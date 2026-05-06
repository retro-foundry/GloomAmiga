# Verifying Gloom PC Releases

Release binaries are built by GitHub Actions from the tagged source revision. A release tag such as `v0.1.0` creates the Windows, DOS, Linux, and macOS archives, a `SHA256SUMS.txt` checksum file, and GitHub artifact attestations where GitHub supports them.

## Verify Checksums

Download the archive for your platform and `SHA256SUMS.txt` from the same GitHub Release.

On Linux:

```bash
sha256sum -c SHA256SUMS.txt
```

On macOS:

```bash
shasum -a 256 -c SHA256SUMS.txt
```

On Windows PowerShell:

```powershell
Get-FileHash .\gloom-pc-v0.1.0-windows-x64.zip -Algorithm SHA256
```

Compare the printed hash with the matching line in `SHA256SUMS.txt`.

## Verify Provenance

Install the GitHub CLI, authenticate with `gh auth login`, then verify an archive against this repository:

```bash
gh attestation verify gloom-pc-v0.1.0-linux-x64.tar.gz --repo OWNER/REPO --signer-workflow .github/workflows/release.yml
```

Replace `OWNER/REPO` with the GitHub repository name and replace the archive name with the file you downloaded. A successful verification means GitHub has a provenance record tying that file digest to this repository's release workflow.

Checksums prove the file you downloaded matches the release asset. Attestations prove the matching file was produced by GitHub Actions from the tagged repository workflow.
