# Releasing Gloom PC

This repository releases native desktop and DOS builds through GitHub Actions. Pushing a version tag matching `v*.*.*`, for example `v0.1.0`, builds clean Linux, Windows, macOS, and DOS archives from the tagged source, runs the available self-tests, creates SHA256 checksums, records artifact attestations, and publishes the files to a GitHub Release using `GITHUB_TOKEN`.

## Create a Release

1. Make sure `main` is green in CI.
2. Update `project(GloomPC VERSION ...)` in `CMakeLists.txt` if the in-game `--version` output should match the release tag.
3. Create and push an annotated tag:

```bash
git tag -a v0.1.0 -m "v0.1.0"
git push origin v0.1.0
```

The release workflow creates or updates the GitHub Release for that tag. The release assets appear on the repository's Releases page after all platform jobs finish.

## Release Assets

Each release contains:

- `gloom-pc-<tag>-windows-x64.zip`
- `gloom-pc-<tag>-dos-386.zip`
- `gloom-pc-<tag>-linux-x64.tar.gz`
- `gloom-pc-<tag>-macos-x64.zip`
- `SHA256SUMS.txt`
- `VERIFY_RELEASE.md`

The platform archives contain the executable, `README.TXT`, `GLOOM.INI`, this verification guide, and the required Amiga runtime assets copied by the build. The DOS archive also contains `GLOOM.BAT` and `CWSDPMI.EXE`, which DJGPP protected-mode DOS programs need on real DOS systems and some emulators. Source trees, CMake caches, intermediate objects, debug symbols, and temporary files are not packaged.

The DOS job bootstraps the DJGPP GCC 12.2.0 cross-compiler, CWSDPMI, and the pinned SDL3 DOS source used by this port before running `tools/build_dos.ps1`.

## Verification

Players can verify release downloads with `SHA256SUMS.txt` and GitHub artifact attestations. See `VERIFY_RELEASE.md` for commands.

The release notes state that binaries were built by GitHub Actions from the tagged source. GitHub's attestation records can be checked with:

```bash
gh attestation verify <archive> --repo OWNER/REPO --signer-workflow .github/workflows/release.yml
```

## Optional Signing

Windows signing is wired into the release workflow but only runs when both secrets are present:

- `WINDOWS_SIGNING_CERT_BASE64`: base64 encoded `.pfx` code-signing certificate
- `WINDOWS_SIGNING_CERT_PASSWORD`: password for the `.pfx` certificate

To create the base64 value locally in PowerShell:

```powershell
[Convert]::ToBase64String([IO.File]::ReadAllBytes("C:\path\to\certificate.pfx"))
```

macOS signing and notarization are not implemented yet. TODO: add a Developer ID certificate, a temporary keychain setup, `codesign`, `notarytool`, and stapling. Expected future secrets are:

- `MACOS_DEVELOPER_ID_CERT_BASE64`
- `MACOS_DEVELOPER_ID_CERT_PASSWORD`
- `MACOS_NOTARY_APPLE_ID`
- `MACOS_NOTARY_TEAM_ID`
- `MACOS_NOTARY_PASSWORD`

No signing secret is required for ordinary unsigned releases.

## Security Notes

The CI workflow runs on pushes and pull requests with read-only repository permissions. The release workflow runs only for pushed version tags and grants release/provenance permissions only to release jobs.

Recommended repository settings:

- Protect tags matching `v*`.
- Allow only maintainers to create release tags.
- Prefer signed annotated tags.
- Require CI to pass before tagging a release commit.
