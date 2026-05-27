# Security Policy

## Supported versions

Only the latest release is supported with security fixes.

## Reporting a vulnerability

Please do **not** open a public GitHub issue for security vulnerabilities.

Report security issues privately to the maintainer:

- GitHub: [@joehonkey](https://github.com/joehonkey)
- Email: joseph.d.harris78@gmail.com

Include:
- A clear description of the vulnerability
- Steps to reproduce or a proof-of-concept
- The version of DojoIRC affected
- Any suggested mitigations

You can expect an acknowledgement within a few days and a fix or status update within 30 days.

## Scope

In-scope:
- Remote code execution or command injection via IRC messages or DCC
- SSRF via URL preview fetching
- Path traversal in DCC file receive
- Config file exposure (passwords stored in plaintext)
- Any vulnerability allowing an IRC peer to read local files or execute commands

Out of scope:
- Issues requiring physical access to the machine
- Social engineering
- Vulnerabilities in upstream dependencies (report those to the upstream project)

## Security notes

- Config files (`config.toml`) may contain IRC server passwords, NickServ passwords, and SASL credentials stored in plaintext. Protect this file accordingly.
- DCC file transfers connect directly to the peer's IP address. Only accept DCC offers from trusted users.
- URL previews fetch metadata from linked sites, which receives your IP address. Disable with `previews_enabled = false` in `[behaviour]`.
