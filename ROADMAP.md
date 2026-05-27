# DojoIRC Roadmap

## Stage 1 — Foundation (current)
- [x] Project scaffold (Go + Wails v2)
- [x] TOML config system (XDG-aware)
- [x] IRC engine (connect, TLS, join, PRIVMSG)
- [x] Basic UI layout (sidebar, messages, nicklist, input bar)
- [x] Default theme (Catppuccin Mocha)
- [x] System tray (hide to tray, show/quit)
- [x] Platform icon sets (Linux, FreeBSD, macOS, Windows)
- [x] MIT license, GitHub repo
- [x] IRC engine wired to UI — real messages flowing
- [x] Slash commands: /nick /whois /join /part /me /msg /query /away /back /topic /kick /mode /invite /raw /quit /help
- [x] Server buffer (click network name to see MOTD and connection output)
- [x] Nick colorization (consistent hash-based color per nick)
- [x] Nick list with op/voice sorting and colors
- [x] Right-click context menu to leave channels / close DM windows
- [x] Multi-server support in UI
- [x] DM (query) windows — click/right-click nick to open, right-click to close
- [x] Typing indicators (IRCv3 draft/typing — outgoing debounced, incoming shown above input)
- [x] Unread dots (accent = unread, yellow = mention) left of channel names
- [x] Draggable sidebar and nicklist resize handles with min/max
- [x] Panel width persistence (localStorage — survives restarts)
- [x] Hamburger menu (theme picker, open config, reload config, quit)
- [x] Topic toggle pill button in buffer header
- [x] Paste in input field (right-click context menu, Wails clipboard API)
- [x] Script aliases — /sysinfo (OS/kernel/CPU/RAM); /exec and /music pending
- [x] Tab completion — nick (cycles, adds `: ` at line start) + slash command completion
- [x] /j alias, /clear command
- [x] Right-click server → Connect / Disconnect
- [x] Auto-reconnect — retries every 10s on unexpected disconnect, quit channel for instant cancel
- [x] Smart auto-scroll — scrolls to bottom on new messages (if at bottom) and channel switch
- [x] In-app documentation panel (hamburger → Documentation)
- [x] Restart button in hamburger menu
- [x] Window position save/restore across hide/show cycles

## Stage 2 — Core IRC Features
- [x] SASL PLAIN authentication
- [x] WHOIS — /whois command + reply display in server buffer
- [x] Nick list with op/voice/halfop indicators — live updates on JOIN/PART/QUIT/NICK
- [x] CTCP — auto-replies to VERSION/PING/TIME; /ctcp command to query others
- [x] Highlight/mention detection — regex match, red row tint, yellow unread dot
- [x] Desktop notifications — Web Notifications API on mention
- [x] Message logging to disk — ~/.config/dojoirc/logs/<server>/<channel>.log
- [x] Full IRCv3 CAP LS 302 negotiation (multiline accumulation, dynamic cap requesting)
- [x] NickServ identify flow
- [x] Channel modes display
- [x] Away status (305/306 numerics + away badge in input bar)
- [x] DCC SEND — file transfer (peer-to-peer TCP); Go backend opens listener, negotiates via CTCP, streams file to recipient's ~/Downloads
- [x] DCC SEND drag-and-drop — drag a file onto a DM/query window to initiate DCC SEND; file lands in recipient's ~/Downloads
- [x] DCC chat (basic) — Accept/Decline UI, `/dcc chat <nick>` initiate, live TCP message routing
- [x] Channel list (/LIST — streaming overlay panel with filter)
- [x] Ignore list (per-server ignore = [...] in config)

## Stage 3 — IRCv3 Capabilities
- [x] `message-tags` — CAP negotiation + inbound tag parsing
- [x] `typing` — outgoing TAGMSG typing indicators (debounced); incoming shown above input bar
- [x] `server-time` — display server-supplied message timestamps
- [ ] `batch` — batched message handling
- [ ] `labeled-response` — correlate responses to requests
- [ ] `multi-prefix` — show all mode prefixes per nick in nicklist
- [ ] `extended-join` — account name on join
- [ ] `account-notify` — track account changes live
- [x] `away-notify` — AWAY messages tracked; 305/306 handled; away badge in input bar
- [ ] `invite-notify` — channel-wide notification on /INVITE
- [ ] `chghost` — host change without reconnect
- [ ] `userhost-in-names` — full user@host in NAMES reply
- [ ] `setname` — live realname changes
- [ ] `chathistory` — bouncer/server history playback
- [ ] `echo-message` — server confirms sent messages
- [ ] `msgid` — unique message IDs for deduplication
- [ ] `Monitor` — notify when a tracked nick comes online/offline
- [ ] `cap-notify` — dynamic capability advertisement after connect
- [ ] `channel-context` — associate messages with channels in batch
- [ ] `multiline` — multi-line message support
- [ ] `react` — emoji reactions to messages
- [ ] `read-marker` — sync read position across clients
- [ ] `Standard Replies` — structured error/info replies from server
- [ ] `sts` — Strict Transport Security; remember TLS-only per server, refuse plaintext downgrade
- [ ] `utf8only` — declare UTF-8 only connection; remove encoding ambiguity
- [ ] `draft/message-redaction` — allow authors/ops to delete a message with a reason (Ergo supported)
- [ ] `draft/account-registration` — in-band account registration via CAP, no NickServ required (Ergo supported)
- [ ] `draft/channel-rename` — rename a channel without part/rejoin (Ergo supported)
- [ ] `WHOX` — extended WHO reply with account names, idle time, real hostname (ISUPPORT token, not a CAP)

## Stage 4 — User Experience
- [x] Theme system (load from themes/*.toml at startup)
- [x] Theme picker (scrollable A-Z panel, active theme highlighted, persists to config)
- [x] URL detection + clickable links
- [x] URL preview cards (og: metadata, inline images)
- [x] Nick colorization (consistent hash-based color per nick)
- [x] Live theme switching (reload without restart)
- [x] Font + font size selection in config (applied via Reload Config)
- [x] Context menu overflow fix (flips upward/leftward at screen edges)
- [x] Emoji support (picker + shortcodes + Tab completion)
- [x] Input history navigation (Up/Down arrows in message input cycle through sent messages)
- [x] Message search in buffer (Ctrl+F, live highlight, Escape to close, prev/next navigation with match counter)
- [x] Buffer scrollback limit (configurable via [behaviour] scrollback, default 5000)
- [x] Keyboard shortcuts (Alt+↑↓ channels, Alt+←→ servers, Ctrl+F search)
- [x] Command autocomplete (Tab)
- [x] Nick autocomplete (Tab)
- [x] Full-height nicklist column (spans full window height alongside input bar)
- [x] Unified input bar (full-width color strip with inline nick prefix + vertical separator)
- [x] Bot icon alignment (inline flex layout — icon sits next to nick text, not at edge)
- [x] Font size manager — Hamburger → Font Sizes opens a panel with live +/− controls for 14 UI zones (sidebar header, hamburger button, server names, channel names, buffer title, channel modes, topic button, topic text, chat messages, timestamps, nick list, typing indicator, input nick prefix, input field). Changes apply instantly and persist via localStorage.
- [x] Hide nick toggle — ‹/› pill button left of the nick in the input bar hides/shows your nick; persists across restarts
- [x] Hide userlist toggle — ◂/▸ pill button in the buffer header hides/shows the nicklist; persists across restarts
- [x] PM Previews toggle — Hamburger → PM Previews: On/Off controls whether URL previews appear in DM buffers; persists via localStorage (default on)

## Stage 5 — Power Features
- [x] Bouncer support (ZNC, soju) — `password` field sends `PASS` before registration
- [ ] MONITOR / WATCH (notify when nick comes online)
- [ ] Proxy support (SOCKS5)
- [ ] mTLS client certificates
- [ ] Split buffer view
- [ ] Drag-to-reorder channels
- [ ] Channel pinning
- [ ] Per-server custom nick/ident/realname
- [ ] Auto-reconnect with backoff
- [ ] Flood protection (message queue + rate limiting)
- [ ] Script aliases (/music, /sysinfo, custom /exec aliases)
- [ ] Plugin/script hooks (shell scripts on events)
- [ ] Memory usage optimization — audit and reduce per-buffer message footprint, DOM node churn, and Go-side allocations
- [x] Nick list cleared on disconnect — Go-side server map entry deleted, JS-side channel nicks cleared; 353 NAMES repopulates on reconnect
- [ ] Explicit nick/channel state cleanup on PART, QUIT, KICK, and channel close (prevent stale accumulation in long-running sessions)
- [ ] `/memstats` debug command — print goroutine count, heap, per-buffer message counts, preview cache size, DCC state counts

## Stage 6 — Platform Polish
- [x] FreeBSD build confirmed working — system tray, WebKit frontend, all features (requires patched Wails v2; see docs/building.md)
- [ ] FreeBSD port skeleton (official ports tree submission)
- [ ] macOS native notifications
- [ ] Windows toast notifications
- [x] .desktop file installer (Linux)
- [ ] macOS .app bundle
- [ ] Windows installer (NSIS, already scaffolded by Wails)
- [ ] Flatpak / AppImage (Linux distribution)
- [x] GitHub Actions CI (build matrix: linux/windows/macos; v* tag triggers release with platform artifacts)
- [ ] Auto-update check

## Security & Reliability

- [x] Write `config.toml` with `0600` permissions (secrets are currently world-readable on multi-user systems)
- [x] Validate URL scheme in `BrowserOpen` backend (defense-in-depth; frontend already validates)
- [x] DCC hardening: quoted filename parsing, port/size range checks, `DialTimeout`, read/write deadlines, stop at advertised size, no-overwrite (rename collisions)
- [x] DCC: add max receive file size setting in config
- [x] Protect `a.quitting` with `sync/atomic.Bool`; guard all `a.clients` and `a.cfg` access consistently under `a.mu`
- [x] Add context cancellation to `App` and IRC clients to coordinate clean shutdown
- [x] Redact secrets in logs: suppress PASS, OPER, NickServ IDENTIFY, AUTHENTICATE, and SASL AUTHENTICATE lines before writing to disk
- [x] Expand private IP blocklist in URL preview: add link-local (169.254.0.0/16, fe80::/10), CGNAT (100.64.0.0/10), and cloud metadata service ranges
- [x] Validate og:image URLs returned by preview fetcher (may point to internal hosts)
- [x] PM Previews toggle — Hamburger → PM Previews: On/Off toggles URL previews in DM buffers; localStorage persistence (default on); config option deferred to a later release
- [x] Add backend input validation for `DCCAccept`, `DCCSend`, `DCCChatAccept` (defence-in-depth, not only `BrowserOpen`)
- [ ] Hide `/raw` and operator commands (`/oper /kill /kline /dline /rehash /wallops`) from default `/help`; warn on first use
- [x] Preview cache: add TTL and max-entry limit (backend `sync.Map` and frontend `Map` are currently unbounded)
- [x] First-run safety: DCC and URL previews off-by-default until enabled in config
- [x] Add DCC security note and URL preview privacy note to docs
- [x] Align scrollback docs and config with actual behavior (memory cap 500, persistence cap 200)
- [x] Add `SECURITY.md`

## Testing & CI

- [x] CI workflow for push/PR (separate from release workflow — go test, go vet, frontend build)
- [x] Pin Wails CLI version in release workflow (currently uses `@latest`)
- [x] Go unit tests: `internal/dcc` (ParseSend, malformed payloads, quoted filenames, IP conversion)
- [x] Go unit tests: `internal/preview` (scheme check, private IP rejection, redirect behavior, read limit)
- [x] Go unit tests: `internal/config` (defaults, missing file, bad TOML, SASL parsing)
- [ ] IRC integration tests with fake server (connect/CAP/SASL/reconnect/malformed messages/no-reconnect-after-quit)
- [x] Run `go test -race ./...` locally and fix reported races
- [x] Add `govulncheck` and Dependabot for Go modules and npm
- [x] Add checksums to release artifacts

## IRCv3 Capabilities

We strive to be a leading IRC client with a rich IRCv3 feature set.
Reference: https://ircv3.net/irc/

| Capability | Status | Notes |
|---|---|---|
| `account-notify` | planned | Track account changes live |
| `away-notify` | planned | Live away status updates in nicklist |
| `batch` | planned | Batched message handling |
| `cap-notify` | planned | Dynamic capability advertisement after connect |
| `channel-context` | planned | Associate messages with channels in batch |
| `chathistory` | planned | Bouncer/server history playback |
| `chghost` | planned | Host change without reconnect |
| `echo-message` | planned | Server echoes sent messages back (confirms delivery) |
| `extended-join` | planned | Account name included in JOIN messages |
| `invite-notify` | planned | Channel-wide notification on /INVITE |
| `labeled-response` | planned | Correlate responses to specific requests |
| `message-tags` | done | CAP negotiation wired; tags parsed on inbound messages |
| `Monitor` | planned | Notify when a nick comes online or goes offline |
| `msgid` | planned | Unique message IDs for deduplication and threading |
| `multi-prefix` | planned | Show all mode prefixes per nick in NAMES/nicklist |
| `multiline` | planned | Multi-line message support |
| `react` | planned | Emoji reactions to messages |
| `read-marker` | planned | Sync read position across clients |
| `sasl` | partial | SASL PLAIN done; EXTERNAL planned |
| `server-time` | done | Display server-supplied message timestamps |
| `setname` | planned | Live REALNAME changes |
| `Standard Replies` | planned | Structured error/info replies from server |
| `typing` | done | Outgoing TAGMSG typing indicators; incoming shown above input bar |
| `userhost-in-names` | planned | Full user@host included in NAMES reply |
| `sts` | planned | Strict Transport Security — remember TLS-only per server, refuse plaintext downgrade |
| `utf8only` | planned | Declare UTF-8 only connection; removes encoding ambiguity |
| `draft/message-redaction` | planned | Delete a message with a reason; Ergo supported |
| `draft/account-registration` | planned | In-band account registration via CAP, no NickServ required; Ergo supported |
| `draft/channel-rename` | planned | Rename a channel server-side without part/rejoin; Ergo supported |
| `WHOX` | planned | Extended WHO reply with account names, idle time, real hostname (ISUPPORT token) |
