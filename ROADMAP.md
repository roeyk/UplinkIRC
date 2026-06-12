# Uplink Roadmap

A fast, secure, IRCv3-featured IRC client built with Qt6 and C++.  
Default network: **irc.linuxdojo.org:6697** — channel **#uplink**

---

## Completed

- [x] Project scaffold — Qt6/C++17, CMake, directory structure
- [x] Config loader — TOML format via toml++, auto-creates `~/.config/uplink/config.toml` on first run
- [x] First-run nick dialog — prompts for nick if config has placeholder `yournick`
- [x] IRC connection — SSL/TLS via QSslSocket, CAP LS 302 negotiation
- [x] IRCv3 message tag parser — full prefix + tag parsing
- [x] CAP negotiation — requests multi-prefix, away-notify, server-time, message-tags, batch, labeled-response
- [x] Full IRC numerics — 001 welcome, 332/353/366 topic+names, MOTD, ISUPPORT, nick-in-use fallback
- [x] IRC commands — JOIN, PART, QUIT, NICK, KICK, MODE, TOPIC, PRIVMSG, NOTICE, CTCP ACTION
- [x] Data model — Message, Channel, ServerSession, SessionModel
- [x] Message buffer cap — 500 messages per channel (RAM-flat on long sessions)
- [x] Nick list — sorted by prefix rank (~&@%+), live updates on JOIN/PART/QUIT/NICK
- [x] Main window UI — toolbar, sidebar (server/channel tree), chat view, nick list panel, topic bar, input bar
- [x] Nick list panel — embedded on the right side of the chat view in a QSplitter
- [x] Topic bar — shows topic text + channel modes, toggleable from hamburger
- [x] Hamburger menu — About, Documentation (stub), App Icon picker, Theme picker, topic/nick/emoji toggles
- [x] Persistent Preferences dialog — hamburger now opens a non-modal QDialog; stays open while browsing themes/toggles; replaces dismiss-on-click QMenu
- [x] Preferences UI rework (v0.9.2) — Manage Servers + Documentation at top; theme as collapsible stay-open list (arrow keys browse, Enter/click applies); App Icon as radio buttons; About removed; Nick Brackets dropdown compact; Enter-triggers-Font-Config bug fixed
- [x] Hamburger restored as dropdown — ☰ opens About Uplink, Documentation, Preferences, Open Config, Reload Config
- [x] Open Config / Reload Config — Open Config opens config.toml in system editor; Reload Config restarts the app to apply all changes (v0.17.1: changed from partial hot-reload to full restart)
- [x] Input commands — /join, /j, /part, /nick, /me, /msg, /quote, /raw, /quit, /ping, /invite, /mode, /op, /deop, /voice, /devoice, /ban, /unban, /clear
- [x] Raw command passthrough — unrecognized /CMD inputs sent directly as raw IRC lines; /REHASH, /SAMODE, /GLOBOPS etc. work without /quote prefix (v0.16.1)
- [x] System tray — minimize to tray on close (× button or Ctrl+W), left-click shows window, right-click menu (Show/Quit)
- [x] Unread badge — tray icon gets red dot on unread messages; green dot for mention/PM when window not focused
- [x] Theme loader — 55 TOML themes, applies as QSS stylesheet, live switching from hamburger
- [x] App icon — 3 switchable styles (Node N, Tower, Hub Spoke); SVG dark / PNG light variants; auto-selects based on active theme palette brightness; tray icon locked to hub-spoke SVG (v0.16.2)
- [x] About dialog — static Node-Relay-Tower.png logo (128×128), version, description (v0.16.2)
- [x] Signal ordering fix — config loads after MainWindow connects signals
- [x] Version baked into binary — `version.h` generated from CMake `PROJECT_VERSION` at build time
- [x] Full documentation — configuration.md, commands.md, faq.md, ircv3.md, keyboard-shortcuts.md (beginner-friendly, real examples)
- [x] How-To guide — docs/howto.html: left-side nav tree, step-by-step from install to tweaks, platform tabs, callout boxes, scroll-spy; linked from GitHub Pages and README
- [x] GitHub repo — public, branch-protected, invite-only contributions; renamed to uplink/UplinkIRC (v0.16.2)
- [x] GitHub Pages landing page — https://uplink.github.io/Uplink/
- [x] README beautification — badges, icon gallery, feature tables, annotated config, commands table, download buttons
- [x] Full rebrand to Uplink — name, binary, config path, icons, docs, CI, GitHub repo (v0.16.2)
- [x] Downloadable icon set on docs site — 8 PNGs (3 styles × dark/light + avatar + banner) with inline preview (v0.16.2)
- [x] Local directory renamed to Uplink; all paths, hooks, and memory updated (v0.16.2 housekeeping)
- [x] Release CI fixed — all three platform builds produce Uplink-v0.16.2-* artifacts (v0.16.2 housekeeping)
- [x] Nick completion — Tab key completes nicks in input bar
- [x] Slash command tab completion — Tab also completes /commands (e.g. /pi → /ping)
- [x] Input history — Up/Down arrow cycles through sent messages
- [x] Colored nicks — consistent hash color per nick in chat and nick list
- [x] In-app Documentation panel — Hamburger → Documentation opens tabbed help viewer
- [x] Documentation search — live search bar in the tab bar (right of Shortcuts); jumps to first match in active tab; re-runs on tab switch
- [x] About dialog centered on app window — overlay QFrame positioned in parent-local coordinates; works on Wayland and X11
- [x] Save theme + icon choice to config.toml (persists across restarts)
- [x] about.png background normalized — logo visible on both light and dark backgrounds
- [x] IRC color code rendering — mIRC bold, italic, underline, strikethrough, reverse, 16 colors (fg+bg)
- [x] Per-widget font size config — Font Config dialog in hamburger; independent sizes for all UI zones; persists to config.toml
- [x] Typing indicator — IRCv3 draft/typing TAGMSG; debounced send + receive; hamburger toggle
- [x] Extended slash commands — /away, /back, /motd, /whois, /topic, /kick, /notice, /version, /ctcp, /sysinfo, /help
- [x] /sysinfo rewritten (v2) — OS/CPU/MEM/GPU/UP format; GPU via vulkaninfo (deviceName + renderer), lspci fallback; uptime from /proc/uptime
- [x] CTCP auto-replies — incoming VERSION and PING answered automatically
- [x] Channel info bar — shows #channel (modes) ServerName; modes from 324 RPL_CHANNELMODEIS
- [x] Sidebar server item clickable — opens server buffer, shows config short name
- [x] Server buffer unread indicator — server name turns purple in sidebar when unread notices arrive (CTCP replies, NickServ, etc.); clears on focus; CTCP replies routed through noticeReceived so they count as unread
- [x] Colored nicks hamburger toggle — was config-only, now in UI
- [x] GitHub Actions CI — builds on every push across Linux, Windows, macOS
- [x] GitHub Actions release workflow — builds platform binaries on tag push, attaches to GitHub releases
- [x] v0.2.0 released
- [x] PM tabs — /msg opens sidebar buffer; incoming PMs land in sender's buffer
- [x] Nick list right-click menu — Message, Send File, Whois, Invite, Give Op, Take Op, Give Voice, Take Voice, Version, Ping (CTCP RTT), Copy Nick, Kick, Ban, Kick & Ban
- [x] Right-click nick in chat view — same context menu as nick list; nicks rendered as nick: anchors in HTML
- [x] Tray icon left-click toggles window visibility
- [x] Unread dot indicator in sidebar — 🔥 for activity, 💡 red for nick mentions; clears on focus
- [x] Sidebar flat list — Halloy-style: no arrows, servers as section headers, connected icon, dock titles removed
- [x] Font Config: Network Name and Typing Indicator size controls
- [x] Info bar always visible with channel, modes, network, user count
- [x] Topic display — separate drop-down area with chat-window background, toggled by Show Topic
- [x] Panel detach/float — sidebar panel could be detached and re-docked (removed in v0.7.12; sidebar is now a fixed embedded panel)
- [x] Topic bar layout — network/user count next to channel label, not far right
- [x] /topic channel-name parsing — `/topic #channel text` correctly separates channel from topic text
- [x] Topic bar background matches input area — info bar visually joins the input row
- [x] Mode string spacing — gap between `(modes)` and `* Network` fixed
- [x] Clickable URLs in topic display — http/https links open in browser
- [x] Toolbar removed — nothing above the info bar; all panels start flush at top
- [x] Minimal dock title bar — sidebar had a slim bar with a ⧉ float button (replaced in v0.7.12 by gear toggle in the info bar)
- [x] Hamburger menu relocated — now in topic bar, left of #channel label
- [x] Status bar text shrunk — 7pt via QSS
- [x] Clickable URLs in chat messages — http/https links in PRIVMSG, actions, and notices open in browser
- [x] Reconnect with exponential backoff — auto-reconnect on unexpected disconnect; 5s→10s→20s→40s→60s; deliberate quit disables it
- [x] Sidebar right-click context menus — server items: Disconnect/Reconnect; channel items: Rejoin/Leave/Close; PM queries: Close Query
- [x] v0.3.0 released
- [x] macOS release CI fixed — MACOSX_BUNDLE property set; .app bundle produced; macdeployqt succeeds; v0.3.0 re-tagged and all three platform builds pass
- [x] Link preview — hover tooltip shows domain + page title; inline card with og:image thumbnail auto-appears below live URL messages
- [x] Link preview fixed for regular URLs — redirect following, proper UA, 32 KB buffer, multi-line title regex
- [x] Link preview redesigned (v0.12.0) — title+domain on top, image below; 360×220 scale; ChatBrowser subclass decodes data: URIs so images actually render; WhatsApp/2 UA fixes YouTube and heavy sites
- [x] Link right-click menu (v0.12.0) — Copy URL / Open URL / Hide Preview; left-click opens browser; double-menu bug fixed by moving handler to QContextMenu event
- [x] Link preview show/hide toggle (v0.12.1) — right-click a hidden-preview URL shows "Show Preview"; card restores from cache without re-fetch; hiddenPreviews QSet tracks state per channel
- [x] Notification icon-only indicators (v0.12.0) — removed color changes on channel names; 💡/🔥 icons are the sole unread indicators; eliminates layout shift
- [x] Dark banner — uplink-top-banner-dark.svg used in README and About dialog
- [x] `/nick` label update — changing nick now immediately reflects in the input bar nick label
- [x] Direct image URL preview — `.png/.jpg/.jpeg/.gif/.webp` links show a thumbnail card without HTML parsing
- [x] Typing indicator layout — moved from chat-view overlay (covered text) to dedicated layout row; collapses when idle
- [x] FreeBSD download badge in README — links to build-from-source instructions; signals FreeBSD support
- [x] Cross-platform OS characters image on GitHub project page
- [x] TLS certificate verification — certificates are now verified; invalid certs disconnect with an error message instead of being silently accepted
- [x] Credential redaction — `PASS`, `AUTHENTICATE`, and NickServ `IDENTIFY` payloads never appear in the raw log
- [x] Config file hardened — `config.toml` written with owner-only `0600` permissions; save is atomic via `QSaveFile`; all string values properly TOML-escaped
- [x] Link preview LAN protection — private/loopback/link-local addresses blocked from auto-fetch; prevents internal network probing via chat links
- [x] CTCP rate limiting — `VERSION` and `PING` replies limited to once per nick per 5 s; PING echo payload capped at 32 bytes
- [x] Inbound DoS protection — input buffer capped at 64 KB; oversized IRC lines (> 8 KB) dropped; `BATCH` open count (max 8) and message count (max 1 000) bounded
- [x] QTextBrowser block count bounded — chat view kept in sync with message model cap; no unbounded RAM growth on long sessions
- [x] Duplicate disconnect signal fixed — `onErrorOccurred` no longer double-emits or double-schedules reconnect
- [x] Link preview image decode safety — `QImageReader` dimension check before decode; images > 4096×4096 rejected; scaled during decode, not after
- [x] Channel preview hash capped — per-channel link preview store evicts oldest at 100 entries

---

## Planned — Near Term

- [x] /help command — lists available commands in chat view
- [x] SASL authentication — PLAIN mechanism (CAP negotiation, AUTHENTICATE, 903/904/906)
- [x] SASL EXTERNAL — certificate-based auth; sasl_external + client_cert + client_key config keys; RSA and EC PEM keys; Server dialog browse buttons
- [x] NickServ IDENTIFY auto — `nickserv_password` in config; sent to NickServ on RPL_WELCOME
- [x] Server error routing — 482 and other server errors shown in active channel buffer, not just (server)
- [x] Multiple servers — Manage Servers dialog: add, edit, remove with live connect/disconnect
- [x] Reconnect logic — auto-reconnect with backoff on disconnect
- [x] Connection status indicator — signal bars widget in topic bar; latency-based bar count (4=<50ms … 1=>300ms); blue flash=connecting, red flash=disconnected
- [x] Mention notifications — 💡 red sidebar indicator when nick is mentioned in inactive channel; 🔥 for general activity; self-nick highlighted red bold in chat
- [x] Desktop notifications — green dot on tray icon on mention/PM when window not focused; clears on window focus; toggle in Preferences

---

## Planned — Medium Term

- [x] Hanging indent — wrapped message lines align past the timestamp+nick column; toggleable from Preferences and `hanging_indent` config key; uses QTextBlockFormat for correct Qt rendering
- [x] Message search — Ctrl+F opens search bar; Enter/Shift+Enter next/prev with wrap; Escape closes
- [x] Hamburger Close Menu button — explicit dismiss action at bottom of ☰ menu
- [x] Logging — per-channel log files at `~/.config/uplink/logs/<server>/<channel>.log`; `log_messages` config key; Preferences toggle (v0.15.0)
- [x] Ignore list — client-side suppress of incoming PRIVMSGs/NOTICEs/ACTIONs from specific nicks; right-click → Ignore/Unignore; /ignore /unignore /ignored commands; persists as [ignore] nicks in config.toml (v0.14.0)
- [x] msgid — IRCv3 unique message IDs stored on every received message; full signal chain (messageReceived, noticeReceived, actionReceived, batch delivery); prerequisite for reply threading, reactions, and redaction
- [x] echo-message — server echoes sent messages back; duplicate local echo suppressed; PM routing corrected for self-echoes; also fixes ZNC self-message PM routing; typing TAGMSG and DCC CTCP offers filtered by `msg.nick != m_nick` to prevent self-echo of typing indicators and incoming DCC dialogs (v0.25.0)
- [x] draft/reply — right-click any message → Reply; reply bar above input; ↩ origNick shown on received replies; @+draft/reply= tag sent on outgoing messages
- [x] draft/message-redaction — right-click own message timestamp → Delete; REDACT sent; received redactions render as [message deleted] (v0.16.0)
- [x] account-notify + extended-join — track NickServ account per nick; populate from JOIN + ACCOUNT commands; account tooltip in nick list (v0.16.0)
- [x] account-tag — per-message account= tag; account field on Message struct; hover tooltip on nick in chat view; four sources: account-tag, account-notify, extended-join, WHOX (v0.16.3)
- [x] Monitor — /monitor add|del|list|clear|status; watch list persisted in config.toml; MONITOR + sent on connect; 730/731 online/offline posted to server buffer (v0.16.0)
- [x] chghost — CAP negotiated; CHGHOST parsed; quiet "nick changed host" status line in shared channels; no fake QUIT+JOIN noise (v0.15.0)
- [x] invite-notify — CAP negotiated; channel invite broadcasts post to channel buffer; direct invites post to server buffer (v0.16.0)
- [x] setname — CAP negotiated; SETNAME parsed; "nick changed their realname" posted to shared channels (v0.16.0); /setname send side added (v0.25.2)
- [x] WHOX — WHO <channel> %cnfa,42; 354 RPL_WHOSPCRPL parsed; account names populated on join; bot flags preserved (v0.16.0); parser corrected for Ergo's 5-field format (no token) — was silently dropping all WHO replies (v0.25.0)
- [x] userhost-in-names — CAP negotiated; !user@host stripped from NAMES entries before display (v0.16.0)
- [x] STS (Strict Transport Security) — auto-upgrade plaintext connections to TLS; policy cached to ~/.config/uplink/sts.ini; stsstore.h/cpp; m_stsUpgrade flag; downgrade prevention (v0.16.3)
- [x] netsplit/netjoin batch types — collapse netsplit noise into a single folded entry (v0.16.4)
- [x] Standard Replies — structured error/warning/note messages from server (v0.16.5)
- [x] UTF8ONLY — detect server UTF-8-only signal, enforce encoding, warn on violations (v0.16.8)
- [x] URL detection + click to open — http/https links in chat open in browser (v0.3.0)
- [x] Link preview persistence — cards survive channel switches (stored in Channel struct)
- [x] Link preview entity decoding — QTextDocument decodes &amp; &#39; &lt; etc. in og:title and &lt;title&gt; fallback paths
- [x] DCC Send File — right-click nick → Send File; DccSend TCP listener + ACK protocol; DccReceive connect + ACK send; progress dialogs; 60s/30s timeouts. NAT limitation: local IP advertised; works on LAN, blocked by NAT on WAN.
- [x] DCC passive / NAT traversal — passive DCC (port=0 + token); DccSend::initPassive/connectOut + DccReceive::listenPassive; new signals dccPassiveOfferReceived / dccPassiveSendReply routed through SessionModel; "Send File (Passive)" in nick right-click menu
- [x] Detachable channel panes (v0.17.0) — right-click sidebar channel → Open in Pane; each pane has its own chat view, nick list, per-pane topic bar (toggle), and input bar; auto layout (2=side-by-side, 3=primary+two stacked, 4=2×2 grid); max 4 total; primary column header + close button when panes are open
- [x] Pane drag-to-rearrange — drag header bar to swap any pane with another pane or the primary panel (v0.18.0)
- [x] Pane layout persistence — save/restore pane arrangement across sessions
- [x] Message reactions — IRCv3 draft/react; receive + store per-msgid; render inline below messages; right-click timestamp → React; /react command; IrcClient::sendReact (v0.15.0)
- [x] Multiline messages — IRCv3 draft/multiline; compose and render multi-line message blocks
- [x] /list command — sends LIST to server; RPL_LIST (322) and RPL_LISTEND (323) displayed in server buffer with user count and topic (v0.25.2)
- [x] /list dialog — QDialog channel browser; streams RPL_LIST (322) results into a sortable QTableWidget (channel, users, topic); live filter box; double-click to join; batched UI inserts to stay responsive on large networks; guard against concurrent LIST requests
- [x] /whowas — RPL_WHOWASUSER (314) and RPL_ENDOFWHOWAS (369); results in active buffer alongside WHOIS (v0.25.2)
- [x] /stats — routes 211–219 and 241–244 to active buffer; /stats u=uptime, o=opers, m=commands (v0.25.2)
- [x] /time (server) — bare /time sends IRC TIME command; RPL_TIME (391) handled; /time <nick> CTCP already existed (v0.25.2)
- [x] Soju bouncer network selection — SASL AUTHENTICATE payload now sends user/network when bouncerNetwork is set; LISTNETWORKS END handled; initial network list shown as formatted summary (v0.25.2)
- [x] IRCv3 WebSocket transport — connect to servers over wss:// in addition to plain TCP+TLS
- [x] User metadata — IRCv3 draft/metadata-2: display-name and avatar keys; avatar image rendered inline in nick list tooltip (fetched async, cached, no upscale); local file paths supported; Preferences Profile section + /displayname /avatar commands; auto-publish on connect; avatar updates in Preferences now reflected immediately without reconnect
- [x] cap-notify — server can notify client of capability changes mid-session; CAP NEW triggers REQ for wanted caps, CAP DEL removes from active set; m_registered flag prevents spurious CAP END on mid-session ACK (v0.25.4)
- [x] draft/chathistory compatibility — Ergo IRCd uses draft cap name; Uplink now requests both names and handles either on history delivery and batch recognition (v0.25.5)

---

## Planned — Polish + Distribution

- [x] Material Design sidebar indicators — lightbulb_2 (yellow) for mentions, forum for unread; drawn after channel name by SidebarDelegate; text position stable
- [x] Server globe icon — Material Symbols "public" replaces hand-drawn dot; stored in UserRole+2 so SidebarDelegate renders it; turns red on server status pane unread
- [x] METADATA SUBSCRIBE removed — Ergo does not support the subcommand; metadata is pushed automatically; fixes [FAIL] SUBCOMMAND_INVALID on ergo.chat
- [x] Mention detection fix — emit selfNickChanged on RPL_WELCOME so mentionRe is built on connect, not only on mid-session NICK change
- [x] Send button — paper-plane SVG in input bar right of emoji button; calls onInputSubmit
- [x] Send button floats inside input area — child widget of input, right-edge centred, repositions on resize; QTextFrameFormat right margin keeps text clear (2026-06-11)
- [x] Tab completion cycling fixed — repeated Tab now cycles all alphabetical matches; stored m_tabWordStart prevents empty-prefix bail-out after first completion (2026-06-11)
- [x] GitHub Pages site redesigned — Gruvbox Light + IBM Plex Mono; Quick Start how-to section embedded on landing page; dark/light mode toggle on both index.html and howto.html (2026-06-11)
- [x] Fade scrollbars — appear on scroll, fade out after 3.5 s; applied to chat view, nick list, sidebar, all channel panes (2026-06-11)
- [x] TAGMSG cap guard — sendTyping/sendReact no longer fire on servers that haven't ACKed message-tags; fixes Unknown command spam on Undernet (2026-06-11)
- [x] FadeScrollBar hardened — unconditional enterEvent wake; sliderReleased connected; timer polling loop removed; leaveEvent guards on opacity>=kVisible; sendTyping cap corrected to draft/typing; sendReact target validated (2026-06-11)
- [x] Long-session memory/fd leaks fixed — log fd leak on closeBuffer; nickMeta unbounded growth; ctcpTimestamps clear-all on overflow; channelpane doc cap misaligned (2026-06-11)
- [x] v0.25.16 released (2026-06-11)
- [x] FadeScrollBar post-release regression fixed — reverted to QGraphicsOpacityEffect (QSS painter path bypasses QPainter::setOpacity; effect composites after all painting); reverted to valueChanged (actionTriggered never fires for wheel events on parent QAbstractScrollArea) (2026-06-11)
- [x] AppImage self-integrates on first run — writes .desktop entry + copies icon to ~/.local/share/; Uplink appears in launchers without manual setup (2026-06-11)
- [x] v0.25.17 released (2026-06-11)
- [x] WHOX ISUPPORT guard — WHO #channel %cnfa,42 now gated behind WHOX in RPL_ISUPPORT; plain WHO fallback for servers that don't support it (e.g. Rizon); fixes spurious "Unknown command" on connect (v0.25.18, 2026-06-12)
- [x] v0.25.18 released (2026-06-12)
- [x] Send button disabled when input empty — re-enables on non-whitespace text; matches onInputSubmit guard (2026-06-12)
- [x] NickDelegate::sizeHint hotspot fixed — returns fixed QSize(width, 16) directly; eliminates ~17K QTextEngine::itemize calls per session found via heaptrack audit (2026-06-12)
- [x] Virtual scrolling — refreshChatView renders last 150 messages (kRenderWindow); grey divider shows older count; scroll-to-top loads 50 more (kRenderChunk) with position restore; ~3x faster channel switches (2026-06-12)
- [x] v0.25.19 released (2026-06-12)
- [ ] Nick completion button — person_search icon triggers Tab completion for non-keyboard users (was added then reverted as pointless mid-message)
- [ ] Preferences toggle for send button — same pattern as emoji button toggle
- [x] Window state persistence — sidebar width and nick panel width saved via QSettings on quit, restored on launch; sidebar is drag-resizable; geometry clamped to available screen on startup (v0.25.1); settings path corrected to ~/.config/uplink/uplink.conf (v0.25.1)
- [x] Nick panel width persistence — QSplitter position saved via QSettings on quit, restored on launch
- [x] Config editor UI — Manage Servers dialog covers server-level editing
- [x] Emoji picker — searchable popup grid with :shortcode: autocomplete and auto-substitution
- [x] Bot nick indicators — SVG robot/alien icon drawn inline to the right of +B nicks in the user list; randomly assigned per nick per session, stable for the connection duration; colorized to theme accent (v0.25.0; replaces emoji font approach which was unreliable across fontconfig configurations)
- [x] Configurable nick brackets — `nick_brackets` in `[ui]`; `"<>"` `"[]"` `"::::"` `""` supported; also configurable live from Hamburger → Nick Brackets
- [x] Autojoin regression fix — editing a server in the GUI no longer wipes auto-join channels; Auto-join field added to Add/Edit Server dialog
- [x] Autojoin Windows fix — config loading now uses Qt file I/O instead of toml::parse_file; paths with non-ASCII characters load correctly on Windows
- [x] Simplified channel config — `channels = "#uplink, #linux"` replaces `[[server.channels]]` array-of-tables; Reload Config syncs changes live via SessionModel::syncServers()
- [x] Channel key persistence — channels saved as `[[server.channel]]` tables with `name` + `key`; backward-compatible load of old comma-string format (v0.13.0)
- [x] Close/Close Query in sidebar — right-clicking a channel shows Close (PARTs + removes buffer); right-clicking a PM query shows Close Query (removes buffer)
- [x] Channel focus on join — joining a channel now always switches focus to it
- [x] Nick panel redesign — replaced detachable QDockWidget with embedded panel in QSplitter; gear button (⚙) in header animates a full spin before toggling the user list; gear and user count remain visible when collapsed; panel background matches chat buffer color across all themes
- [x] Sidebar gear toggle — ⚙ button in the topic bar collapses the server/channel list; hamburger and gear stay pinned in the topic bar at all times (kBtnZoneMinW=48px floor); only the list collapses beneath them
- [x] Native Windows style — windows11 Qt style by default; no alien dark theme on fresh installs
- [x] FreeBSD port skeleton — `packaging/freebsd/`: Makefile, pkg-descr, pkg-plist; uses GH source tarball; needs `make makesum` + qt6-keychain port verification before submission
- [x] AppImage packaging for Linux — build-appimage.sh; linuxdeploy + Qt plugin; zsync metadata embedded; release.yml uploads .AppImage + .AppImage.zsync; plugin strip patched for .relr.dyn (v0.15.0)
- [x] Auto-bump release docs — `scripts/sync-site.sh` replaces all version strings in README.md and docs/index.html; `update-docs` job in release.yml runs it automatically on every tag push; also run manually at session close
- [x] Service shortcuts — /ns /cs /bs /ms aliases for NickServ/ChanServ/BotServ/MemoServ; /query opens PM without sending; /oper for IRC operator login (v0.15.0)
- [x] Right-click copy in chat view — selecting text and right-clicking shows Copy (v0.14.0)
- [x] Theme-aware icons — hamburger menu and gear buttons use m_theme.text; link preview card hardcoded dark (v0.14.0)
- [x] In-app update check UI — "Check for Updates" in hamburger menu; fetches GitHub releases API, parses tag_name, compares UPLINK_VERSION_MAJOR/MINOR/PATCH; shows "Update Available" or "Up to Date" message box
- [x] Event message visual polish — join/part/quit/nick lines render at 82% font size; part/quit/kick color softened to #e06b6b for dark backgrounds (v0.18.2)
- [x] QSS visual polish overhaul — rounded inputs with focus rings, themed checkboxes/radio/tabs/tooltips, floating scrollbars, pill-shaped sidebar selection, menu border-radius and separators (v0.19.0)
- [x] Sidebar fitted pill highlight — custom SidebarDelegate draws rounded rect sized to text width; no full-row highlight; hover shift eliminated; selected text always readable (v0.19.0)
- [x] Nick list pill highlight — NickDelegate mirrors SidebarDelegate; selected/hovered nicks get pill-shaped background sized to text width; QSS transparent-background fix so delegate shows through; vPad=1 radius=6 for proper oblong pill (v0.25.1)
- [x] Topic toggle button theme integration — both primary panel and channel pane topic buttons use #topicToggle QSS rule; removed palette(mid)/palette(highlight) inline stylesheets; always visible in single-window mode (v0.19.0)
- [x] Topic button speech bubble icon — custom QPainter icon replaces text label; muted when collapsed, accent when open; ChannelPane::setTopicIcon() API; updates on theme change (v0.19.1)
- [x] Floating rounded input bar — separator line removed; bar blends into chat area via bufferBg; QLineEdit gets border-radius pill shape (v0.19.1)
- [x] Rounded chat panel with padding — RoundedPane widget clips all chat content (panes splitter + children) to a 10 px rounded rect via setMask(); 8 px right/bottom padding shows sidebarBg for contrast; sidebar toggle balances left margin to match (v0.19.2)
- [x] MD3-inspired UI pass — pill buttons (20px radius via PillButton QPainter subclass), rounder inputs/menus/tooltips, Material Symbols SVG icons for hamburger/gear/all menu items, sidebar pill highlight padding fixed, row heights via sizeHint delegates (v0.20.0)
- [x] Self-signed cert fingerprint pinning — per-server accept/reject/pin dialog on first connect; SHA-256 fingerprint saved to config; mismatch disconnects with warning; IrcClient::abort() for clean reject (v0.20.0)

---

## Security Backlog

- [x] Config file permissions — 0600 enforced via QSaveFile (done v0.8.0)
- [x] Outbound IRC injection prevention — stripCrlf/validIrcToken/ircv3TagEscape helpers; all send paths validated (v0.13.0)
- [x] openUrl scheme guard — only http/https links passed to QDesktopServices::openUrl (v0.13.0)
- [x] Link preview SSRF — og:image and direct image fetches blocked for private/loopback addresses (v0.13.0)
- [x] DCC receive write cap — reads bounded to remaining advertised byte count (v0.13.0)
- [x] DCC send 4 GiB guard — files over UINT32_MAX rejected; ACK comparison uses qint64 (v0.13.0)
- [x] DCC local bind — listener binds to IRC socket's local interface, not all interfaces (v0.13.0)
- [x] DCC cancel cleanup — cancel() methods added; partial files removed on user cancel (v0.13.0)
- [x] DCC offer validation — zero port / non-positive filesize rejected before accept dialog (v0.13.0)
- [x] Reconnect socket abort — socket abort()ed before reconnect if not already unconnected (v0.13.0)
- [x] Password field encryption — don't store plaintext passwords (OS keychain integration) (v0.16.6)
- [x] Password field show/hide toggle — eye icon (Material Symbols) inside each password field in Add/Edit Server dialog; toggles echo mode (v0.24.2)
- [x] Keychain sentinel UX — fields with keychain-stored passwords show placeholder text instead of sentinel as dots; empty-on-save preserves the keychain entry instead of deleting it (v0.24.2)
- [x] Keychain missing-entry warning — if config has `<keychain>` but no OS keychain entry exists, a red error in the server buffer directs the user to re-enter the password in Edit Server (v0.24.2+)
- [x] Memory optimizations — message buffer cap 2000→500, link preview cap 100→20, emoji picker lazy-loaded, QTextDocument block count aligned to buffer cap (v0.24.2)
- [x] Qt6Keychain CI integration — FetchContent fallback in CMakeLists.txt; all three platform CI + release builds passing (v0.16.6 follow-up)
- [x] Theme-aware link preview cards — bg/border/text/timestamp colours from active theme (v0.16.7)
- [x] Right-click Copy + Reply together — both available from message body right-click; no need to aim at the timestamp (v0.16.7)
- [x] NickServ credential redaction extended — IDENTIFY/REGISTER/GHOST/RECOVER/RELEASE/REGAIN/SETPASS all redacted in local echo (v0.16.7)
- [x] Service NOTICE routing — ChanServ/NickServ/BotServ/MemoServ replies route to their PM tab when open (v0.16.7)
- [x] Contextual command replies — /whois, /version, and /ping responses appear in the active channel/PM window instead of the server buffer; implemented via contextualMessage signal from IrcClient through SessionModel (v0.25.0)
- [x] howto.html full-text search — live search box in nav sidebar, match count, keyboard navigation (v0.16.7)
- [x] /leave and /close commands — close query buffer or part channel (v0.16.8)
- [x] NickServ credential redaction complete — echo-message server echo path also redacted (v0.16.8)
- [x] Self-signed cert option — per-server accept/reject + fingerprint-pin UI (v0.20.0)
- [x] SOCKS5 proxy support — per-server proxy_host/port/user/pass; GUI in server dialog (v0.16.8)
- [x] OPER password redaction — `redactRawForLog` now covers OPER credentials same as PASS (2026-06-05)
- [x] DCC passive filename sanitization — strip all control chars from remote-supplied DCC filenames to prevent CTCP envelope injection (2026-06-05)
- [x] DCC passive receive: peer validation skipped when expectedIP is private/zero — first inbound TCP connection accepted without identity check; add per-transfer write timeout and document the limitation (`dccreceive.cpp:98-103`)
- [x] DCC send: no receiver IP validation — first peer to connect gets the file; add expected-peer check to `DccSend::listen()`, reject others (`dccsend.cpp:102-111`)
- [x] Link preview SSRF: DNS rebinding gap — QNAM re-resolves after pre-check; extend `addresscheck.h` to cover CG-NAT (`100.64.0.0/10`), documentation ranges (`192.0.2.0/24`, `198.51.100.0/24`, `203.0.113.0/24`), unspecified (`0.0.0.0/8`), IPv4-mapped private (`::ffff:0:0/96`) (`linkpreview.cpp:169-200`)
- [x] `sendRaw`: no 512-byte IRC line cap — add `.left(510)` before write; oversized lines are partially delivered or rejected by server (`ircclient.cpp`)
- [x] `onConnected`: `NICK`/`USER` only call `stripCrlf`, not `validIrcToken` — malformed nick from bad config produces a broken `USER` line (`ircclient.cpp`)
- [x] DCC send filename: normal send path only replaces spaces, not control chars — create shared `safeDccFilename()` helper used by all DCC SEND paths (`dccsend.cpp:92-95` vs `mainwindow.cpp:1469-1477`)
- [x] Unknown slash commands sent as raw IRC by default — change to "unknown command" message; keep `/raw` and `/quote` for power users; add opt-in `advanced_raw_passthrough` config key (`commanddispatcher.cpp:572-575`)
- [x] `linkpreview`: apply `isBlockedBySchemeOrLiteral` in `extractImageUrl` before returning URL — defense-in-depth; currently only checked by the caller (`linkpreview.cpp`)
- [x] `Channel::previews`: cap stored HTML string length per entry — `kPreviewCap=100` limits count but not per-entry size (`channel.h`)
- [x] KWallet/keychain popup on every preference change — `Config::save()` called `storePassword()` unconditionally; now guarded by `migratePasswords` flag, only true for server dialog saves (v0.24.3)
- [x] README vs code: README says "plaintext not supported" but `connectToHost()` is reachable when `ssl=false` — enforce TLS-only in code or update README (`ircclient.cpp:103-107`)

---

## Performance & Maintainability Backlog

Items from the lightweight code review (2026-06-04). Ordered roughly by value / effort.

### Near term
- [x] `/sysinfo` async — run process collection off the UI thread; cache result for the session; add hard global timeout
- [x] DCC receive hardening — configurable max receive size; write to `.part` file and rename on success; delete partial on failure/cancel; check available disk space before starting
- [x] Link preview queue — replace abort-on-new-fetch with a small queue (max concurrency 1–2); cap `m_previewChannels`; add per-channel/per-minute throttling
- [x] Compiler warning cleanup — fix `-Wconversion` narrowing (qsizetype→int), `-Wshadow` locals, `-Wold-style-cast` casts flagged by the new warning flags
- [x] `m_buffer` in `onReadyRead`: keep as `QByteArray` through line-split; call `QString::fromUtf8` per-line only — eliminates per-event UTF-16 conversion and O(n) string shift (`ircclient.cpp`)
- [x] `logMessage`: cache one `QFile*` per active log target in a `QHash`; flush periodically — open/close per message is the dominant bottleneck during history replay (`sessionmodel.cpp:156-180`)
- [x] `m_ctcpTimestamps`: grows unbounded on long sessions with many unique nicks — cap at ~500 entries or evict on insert (`ircclient.cpp`)
- [x] `removeNick`: patch `nickIndex` in-place (decrement indices > removed position) instead of full `rebuildNickIndex()` — O(n²) during netsplit QUIT storms (`channel.h`)

### Medium term
- [x] Incremental nick-list updates — emit specific `nickAdded`/`nickRemoved`/`nickRenamed`/`nickModeChanged` signals; replace `clear()`/repopulate with targeted updates; batch during NAMES/netsplit bursts
- [x] Per-channel nick index — `QHash<QString, int> nickIndex` (lower nick → index) to replace O(n) scans in `onMessage`, `onNotice`, `onAction`, `onAccountChanged`
- [x] Batch chat/nick refreshes — debounce `refreshChatView()` and `refreshNickList()` with a short single-shot timer for burst updates
- [x] Split `ChatRenderer` out of `MainWindow` — `formatMessage`, `ircToHtml`, `linkifyHtml`, emoji rendering as a separate class
- [x] Split `CommandDispatcher` out of `MainWindow` — `/join`, `/msg`, `/dcc`, `/sysinfo`, etc. as a separate class
- [x] Parser and model unit tests — `IrcParserTest` (prefixes/tags/CTCP/malformed), `ChatFormatTest` (HTML escaping/linkification); `SessionModelTest` deferred (needs mock IrcClient)

### Bigger / architectural
- [x] Keychain operations async — remove nested `QEventLoop` in `KeychainHelper`; load non-secret config first, then resolve secrets before connecting; avoids reentrancy hazard
- [x] Targeted chat block updates — `BlockMsgid` userData tags each QTextBlock; onReactionsChanged and onMessageRedacted do targeted insert/replace/remove instead of full rebuild (v0.23.3)
- [x] IrcParser fuzz target — libFuzzer harness around `IrcParser::parseLine()` for parser regression coverage
- [ ] `ServerId` / `BufferId` strong types — replace stringly-typed host/channel routing; prerequisite for robust multi-network and bouncer support
- [x] `selfNickRe` (`QRegularExpression`): verify compiled once per nick change at call site, not reconstructed per `formatMessage` call (`chatrenderer.cpp`) — already correct
- [x] DCC ACK coalescing: send ACK every 64 KB and on completion, not every `readyRead` — reduces write syscalls on fast links (`dccreceive.cpp`)
- [x] CI: add Linux ASan/UBSan sanitizer job (`-DUPLINK_ENABLE_SANITIZERS=ON` Debug build); add `clang-tidy` or CodeQL static analysis (`.github/workflows/ci.yml`, `CMakeLists.txt:145-149`)

---

## Stability Backlog

- [x] Ping watchdog timeout: `sendPing` ignores stale pending pings indefinitely — abort + reconnect after ~90s; improves recovery from dead Wi-Fi, VPN changes, NAT timeouts (`ircclient.cpp:431-437`)
- [x] Early PING during SASL registration — removed `singleShot(2000, sendPing)` from `onConnected()`; was causing 2-minute disconnect cycle on solanum servers (Libera.Chat) where pre-registration PINGs are silently ignored
- [x] `sendRaw` disconnected: silently drops messages during reconnect — show local "Not connected; message not sent" error for user-visible commands (`ircclient.cpp:276-281`)
- [x] DCC write failure: `onReadyRead` emits error but doesn't call `cancel()` — call cancel directly on write failure; guard against multiple error/finished emissions with `m_done` flag (`dccreceive.cpp:151-154`)
- [x] DCC progress divide-by-zero: `checkTransferPrecon` does not reject `total <= 0` — malformed DCC offer with size=0 can divide-by-zero in UI progress callback (`dccreceive.cpp:21-47`, `mainwindow.cpp:1484-1485`)
- [x] `DccSend::sendNextChunk`: no socket state check after `cancel()` — `m_socket` non-null in `ClosingState` is currently safe but fragile; check state explicitly or null `m_socket` in `cancel()` (`dccsend.cpp`)
- [x] `IrcParser::isValid()`: add explicit `!command.isEmpty()` check — empty-command lines fall through to numeric default handler silently (`ircparser.cpp`) — already implemented
- [x] STS upgrade race: add comment explaining ordering assumption between `abort()`, synchronous `onDisconnected`, and the `singleShot` lambda (`ircclient.cpp`)

---

## Known Issues

- DCC over internet (active mode) — active DCC advertises your local IP; if both sides are behind NAT the connection still fails. Use **Send File (Passive)** so the receiver opens the port instead.
- DCC passive receive over NAT — the receiver's port must be reachable from outside. If the receiver is also behind NAT, passive DCC will not work either (both sides blocked). No relay mechanism implemented.
