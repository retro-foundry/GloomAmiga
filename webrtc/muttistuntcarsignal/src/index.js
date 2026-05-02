import { DurableObject } from "cloudflare:workers";

const MAX_ROOM_SIZE = 2;

function json(data, status = 200, extraHeaders = {}) {
  return new Response(JSON.stringify(data), {
    status,
    headers: {
      "content-type": "application/json; charset=utf-8",
      ...extraHeaders,
    },
  });
}

function text(body, status = 200, extraHeaders = {}) {
  return new Response(body, {
    status,
    headers: {
      "content-type": "text/plain; charset=utf-8",
      ...extraHeaders,
    },
  });
}

function getAllowedOrigins(env) {
  if (env.APP_ORIGINS) {
    return env.APP_ORIGINS.split(",").map((o) => o.trim()).filter(Boolean);
  }
  if (env.APP_ORIGIN) return [env.APP_ORIGIN];
  return [];
}

function getCorsHeaders(env, origin) {
  const allowed = getAllowedOrigins(env);
  if (origin && allowed.includes(origin)) {
    return {
      "Access-Control-Allow-Origin": origin,
      "Access-Control-Allow-Methods": "GET,POST,OPTIONS",
      "Access-Control-Allow-Headers": "Content-Type",
      "Access-Control-Max-Age": "86400",
      "Vary": "Origin",
    };
  }

  return {
    "Access-Control-Allow-Methods": "GET,POST,OPTIONS",
    "Access-Control-Allow-Headers": "Content-Type",
    "Access-Control-Max-Age": "86400",
    "Vary": "Origin",
  };
}

function isAllowedOrigin(env, request) {
  const origin = request.headers.get("Origin");
  if (!origin) return false;
  return getAllowedOrigins(env).includes(origin);
}

function isAllowedHost(env, request) {
  return new URL(request.url).hostname === env.SIGNAL_HOST;
}

function validateRoom(room) {
  return typeof room === "string" && room.length >= 1 && room.length <= 64 && /^[a-zA-Z0-9_-]+$/.test(room);
}

function validatePeer(peer) {
  return typeof peer === "string" && peer.length >= 1 && peer.length <= 128 && /^[a-zA-Z0-9._:-]+$/.test(peer);
}

function base64UrlEncodeBytes(bytes) {
  let binary = "";
  for (let i = 0; i < bytes.length; i++) binary += String.fromCharCode(bytes[i]);
  return btoa(binary).replace(/\+/g, "-").replace(/\//g, "_").replace(/=+$/g, "");
}

function base64UrlEncodeString(str) {
  return base64UrlEncodeBytes(new TextEncoder().encode(str));
}

function base64UrlDecodeToBytes(str) {
  const padded = str.replace(/-/g, "+").replace(/_/g, "/") + "=".repeat((4 - (str.length % 4 || 4)) % 4);
  const binary = atob(padded);
  const bytes = new Uint8Array(binary.length);
  for (let i = 0; i < binary.length; i++) bytes[i] = binary.charCodeAt(i);
  return bytes;
}

function timingSafeEqual(a, b) {
  if (a.length !== b.length) return false;
  let out = 0;
  for (let i = 0; i < a.length; i++) out |= a[i] ^ b[i];
  return out === 0;
}

async function importHmacKey(secret) {
  return crypto.subtle.importKey(
    "raw",
    new TextEncoder().encode(secret),
    { name: "HMAC", hash: "SHA-256" },
    false,
    ["sign", "verify"]
  );
}

async function signTokenPayload(payloadObj, secret) {
  const payloadJson = JSON.stringify(payloadObj);
  const payloadB64 = base64UrlEncodeString(payloadJson);
  const key = await importHmacKey(secret);

  const sig = await crypto.subtle.sign(
    "HMAC",
    key,
    new TextEncoder().encode(payloadB64)
  );

  const sigB64 = base64UrlEncodeBytes(new Uint8Array(sig));
  return `${payloadB64}.${sigB64}`;
}

async function verifyToken(token, secret) {
  if (!token || typeof token !== "string") return null;

  const parts = token.split(".");
  if (parts.length !== 2) return null;

  const [payloadB64, sigB64] = parts;

  let payloadBytes;
  let sigBytes;
  try {
    payloadBytes = base64UrlDecodeToBytes(payloadB64);
    sigBytes = base64UrlDecodeToBytes(sigB64);
  } catch {
    return null;
  }

  const key = await importHmacKey(secret);
  const expectedSig = await crypto.subtle.sign(
    "HMAC",
    key,
    new TextEncoder().encode(payloadB64)
  );

  const expectedBytes = new Uint8Array(expectedSig);
  if (!timingSafeEqual(sigBytes, expectedBytes)) return null;

  let payload;
  try {
    payload = JSON.parse(new TextDecoder().decode(payloadBytes));
  } catch {
    return null;
  }

  if (!payload || typeof payload !== "object") return null;
  if (typeof payload.exp !== "number") return null;
  if (Date.now() > payload.exp) return null;

  return payload;
}

export default {
  async fetch(request, env) {
    const url = new URL(request.url);
    const corsHeaders = getCorsHeaders(env, request.headers.get("Origin"));

    try {
      if (request.method === "OPTIONS") {
        return new Response(null, { status: 204, headers: corsHeaders });
      }

      if (url.pathname === "/health") {
        return text("ok", 200, corsHeaders);
      }

      if (!isAllowedHost(env, request)) {
        return text("forbidden host", 403, corsHeaders);
      }

      if (url.pathname === "/session" && request.method === "POST") {
        try {
          if (!isAllowedOrigin(env, request)) {
            return json({ error: "forbidden origin" }, 403, corsHeaders);
          }

          let body;
          try {
            body = await request.json();
          } catch {
            return json({ error: "invalid json" }, 400, corsHeaders);
          }

          const room = body?.room;
          const peer = body?.peer;

          if (!validateRoom(room)) {
            return json({ error: "invalid room" }, 400, corsHeaders);
          }

          if (!validatePeer(peer)) {
            return json({ error: "invalid peer" }, 400, corsHeaders);
          }

          if (!env.SIGNAL_SHARED_SECRET) {
            return json({ error: "missing SIGNAL_SHARED_SECRET" }, 500, corsHeaders);
          }

          if (!env.SIGNAL_HOST) {
            return json({ error: "missing SIGNAL_HOST" }, 500, corsHeaders);
          }

          const expiresAt = Date.now() + 60_000;
          const payload = { room, peer, exp: expiresAt };
          const token = await signTokenPayload(payload, env.SIGNAL_SHARED_SECRET);

          return json({
            token,
            expiresAt,
            wsUrl: `wss://${env.SIGNAL_HOST}/ws/${encodeURIComponent(room)}?peer=${encodeURIComponent(peer)}&token=${encodeURIComponent(token)}`
          }, 200, corsHeaders);
        } catch (err) {
          return json({
            error: "session_failed",
            detail: String(err && err.message ? err.message : err),
          }, 500, corsHeaders);
        }
      }

      if (url.pathname.startsWith("/ws/")) {
        if (request.headers.get("Upgrade") !== "websocket") {
          return text("Expected WebSocket", 426, corsHeaders);
        }

        if (!isAllowedOrigin(env, request)) {
          return text("forbidden origin", 403, corsHeaders);
        }

        const room = decodeURIComponent(url.pathname.slice("/ws/".length));
        const peer = url.searchParams.get("peer");
        const token = url.searchParams.get("token");

        if (!validateRoom(room)) return text("invalid room", 400, corsHeaders);
        if (!validatePeer(peer)) return text("invalid peer", 400, corsHeaders);

        const claims = await verifyToken(token, env.SIGNAL_SHARED_SECRET);
        if (!claims) return text("bad token", 403, corsHeaders);
        if (claims.room !== room || claims.peer !== peer) return text("token mismatch", 403, corsHeaders);

        const id = env.ROOM.idFromName(room);
        return env.ROOM.get(id).fetch(request);
      }

      return text("Not found", 404, corsHeaders);
    } catch (err) {
      return json({
        error: "worker_failed",
        detail: String(err && err.message ? err.message : err),
      }, 500, corsHeaders);
    }
  },
};

export class Room extends DurableObject {
  constructor(ctx, env) {
    super(ctx, env);
    this.ctx = ctx;
    this.env = env;
    this.peers = new Map();

    for (const ws of this.ctx.getWebSockets()) {
      const meta = ws.deserializeAttachment();
      if (meta?.peer) this.peers.set(meta.peer, ws);
    }
  }

  async fetch(request) {
    const url = new URL(request.url);
    const room = decodeURIComponent(url.pathname.slice("/ws/".length));
    const peer = url.searchParams.get("peer");

    if (!validateRoom(room)) return text("invalid room", 400);
    if (!validatePeer(peer)) return text("invalid peer", 400);
    if (request.headers.get("Upgrade") !== "websocket") return text("Expected WebSocket", 426);

    const existing = this.peers.get(peer);
    if (existing) {
      try { existing.close(1012, "replaced"); } catch {}
      this.peers.delete(peer);
    }

    if (this.peers.size >= MAX_ROOM_SIZE) {
      return text("room full", 403);
    }

    const role = this.peers.size === 0 ? "host" : "guest";
    const pair = new WebSocketPair();
    const [client, server] = Object.values(pair);

    this.ctx.acceptWebSocket(server);
    server.serializeAttachment({ room, peer, role });
    this.peers.set(peer, server);

    this.send(server, {
      type: "welcome",
      peer,
      role,
      peers: [...this.peers.keys()],
    });

    this.broadcast({
      type: "peers",
      peers: [...this.peers.keys()],
      count: this.peers.size,
      max: MAX_ROOM_SIZE,
    });

    return new Response(null, { status: 101, webSocket: client });
  }

  webSocketMessage(ws, message) {
    let msg;
    try {
      msg = JSON.parse(typeof message === "string" ? message : new TextDecoder().decode(message));
    } catch {
      this.send(ws, { type: "error", error: "bad json" });
      return;
    }

    const from = ws.deserializeAttachment()?.peer;
    if (!from) {
      this.send(ws, { type: "error", error: "unknown peer" });
      return;
    }

    switch (msg.type) {
      case "offer":
      case "answer":
      case "ice": {
        if (!validatePeer(msg.to)) {
          this.send(ws, { type: "error", error: "invalid target peer" });
          return;
        }

        const target = this.peers.get(msg.to);
        if (!target) {
          this.send(ws, { type: "error", error: "peer not found" });
          return;
        }

        this.send(target, {
          type: msg.type,
          from,
          payload: msg.payload ?? null,
        });
        return;
      }

      case "ping":
        this.send(ws, { type: "pong", ts: Date.now() });
        return;

      default:
        this.send(ws, { type: "error", error: "unknown message type" });
    }
  }

  webSocketClose(ws) {
    this.remove(ws);
  }

  webSocketError(ws) {
    this.remove(ws);
  }

  remove(ws) {
    const peer = ws.deserializeAttachment()?.peer;
    if (!peer) return;

    this.peers.delete(peer);

    this.broadcast({
      type: "peers",
      peers: [...this.peers.keys()],
      count: this.peers.size,
      max: MAX_ROOM_SIZE,
    });

    try { ws.close(); } catch {}
  }

  send(ws, obj) {
    try {
      ws.send(JSON.stringify(obj));
    } catch {}
  }

  broadcast(obj) {
    const data = JSON.stringify(obj);
    for (const ws of this.peers.values()) {
      try { ws.send(data); } catch {}
    }
  }
}
