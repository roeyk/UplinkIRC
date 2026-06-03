# IRCv3 Support

Uplink aims to be a first-class IRCv3 client. Below is the full status of every negotiated capability, including bouncer-specific extensions.

---

## How capability negotiation works

Uplink uses **CAP LS 302** — it requests the server's full capability list before deciding what to ask for, and only requests capabilities the server actually advertises. This avoids failures from batching unsupported caps together. Multi-line CAP LS responses (marked with `*`) are fully buffered before any `CAP REQ` is sent, and capabilities advertised in `name=value` form (e.g. `sasl=PLAIN,EXTERNAL`) are matched correctly. After the server acknowledges the requested caps (CAP ACK), Uplink completes registration or starts SASL if needed.

---

## Active capabilities

These capabilities are fully negotiated and actively used.

### `multi-prefix`

Returns all mode prefixes for a nick in NAMES replies — for example, `@+alice` instead of just `@alice`. The nick list correctly displays users who hold more than one status simultaneously (e.g. both op and voice).

### `away-notify`

When a user in a shared channel sets or clears away status, the server sends a real-time AWAY message. Uplink receives and tracks these notifications.

### `server-time`

When the server attaches a `time` tag to a message, Uplink uses that timestamp instead of the local clock. This is essential for bouncer history replay — messages show the time they were originally sent, not the time the client received them. Works across all message types (PRIVMSG, NOTICE, JOIN, etc.).

### `message-tags`

The full IRCv3 message tag extension is negotiated. Tags are parsed as a key-value map with proper IRCv3 escape unescaping. This is the transport layer for `server-time`, `typing`, `batch`, and all other tag-based features.

### `batch`

`BATCH` commands are fully handled. When a batch starts (`BATCH +ref type param`), Uplink buffers all messages tagged with `batch=ref` instead of processing them immediately. When the batch ends (`BATCH -ref`), the buffered messages are delivered as a unit with appropriate handling based on the batch type:

- `chathistory` — messages are delivered as history (dimmed, original timestamps, no unread count)
- `znc.in/batch/playback` — same treatment as chathistory
- `netsplit` — all QUITs in the batch are collapsed into one summary line per affected channel (see [netsplit/netjoin](#netsplit--netjoin-batch-types))
- `netjoin` — all JOINs in the batch are collapsed into one summary line per affected channel (see [netsplit/netjoin](#netsplit--netjoin-batch-types))

Safety limits apply: at most **8 batches** may be open simultaneously, and each batch may contain at most **1 000 messages**. A misbehaving or malicious server that opens a batch and never closes it cannot grow client RAM without bound.

### `chathistory`

When `chathistory` is negotiated, Uplink sends `CHATHISTORY LATEST <channel> * 100` after joining each channel, requesting up to 100 recent messages. The server (or bouncer) responds with a `BATCH` of type `chathistory`.

History messages are visually distinct from live messages:
- Displayed at reduced opacity
- Timestamped with their original send time — previous-day messages show `MM/dd hh:mm` so you always know when they were sent
- Not counted as unread, so they do not badge the channel in the sidebar

This works on any server or bouncer that supports `chathistory`, including soju and modern ZNC.

### `sasl` (PLAIN and EXTERNAL)

Authenticates your account during the CAP handshake — before you appear on the network. Uplink holds `CAP END` until the server confirms success (`903`) or failure (`904`/`906`). On failure the connection continues without authentication.

**PLAIN** — Add `sasl_user` and `sasl_password` to a server block. Uplink sends `AUTHENTICATE PLAIN` followed by the base64-encoded `\0user\0password` payload.

**EXTERNAL** — Add `sasl_external = true`, `client_cert`, and `client_key` to a server block. Uplink loads the PEM certificate and key, presents the certificate during the TLS handshake, then sends `AUTHENTICATE EXTERNAL` followed by an empty `+` response. The server derives your identity from the certificate's fingerprint — no password is transmitted. RSA and EC (ECDSA) PEM keys are both supported.

```
Client → AUTHENTICATE EXTERNAL
Server → AUTHENTICATE +
Client → AUTHENTICATE +
Server → 903 :SASL authentication successful
```

### `draft/typing`

When you start typing in the input box, Uplink immediately sends `TAGMSG` with `+typing=active`. It restarts a 5-second inactivity timer on each keypress — if you stop typing for 5 seconds, it sends `+typing=paused`. When you send or clear the input, it sends `+typing=done`.

Incoming typing notifications from other users appear as "nick is typing…" above the input bar and time out automatically. The feature can be toggled from the **Preferences** dialog (☰).

### `labeled-response`

CAP is negotiated. Labels tie server responses to outgoing commands; used together with `echo-message` to confirm message delivery.

### `echo-message`

When negotiated, the server echoes every message you send back to you as a first-class incoming message — with accurate `server-time` and `msgid` tags. This means your sent messages are timestamped by the server rather than the local clock, and carry a proper message ID for deduplication and reply threading.

Uplink suppresses the local preview echo when `echo-message` is active so messages appear exactly once. Self-echoed private messages route to the correct PM buffer (the conversation partner's buffer, not a buffer named after your own nick).

### `msgid`

Assigns a globally unique ID to every message. Parsed from the `msgid` tag on all incoming `PRIVMSG`, `NOTICE`, and `ACTION` messages, stored on the `Message` struct, and carried through batch delivery. Used as the anchor for `draft/reply`, `draft/react`, and future redaction (`draft/message-redaction`).

### `draft/reply`

A client-only tag (`+draft/reply=<msgid>`) that marks a message as a reply to a specific message by its ID. Uplink supports both sending and receiving:

- **Sending:** right-click any message timestamp in the chat, choose **Reply**, then type and press Enter. The outgoing `PRIVMSG` carries `@+draft/reply=<original-msgid>`.
- **Receiving:** incoming messages with a `+draft/reply` tag display a small **↩ origNick** indicator before the sender nick, showing whose message was being replied to.

Requires `message-tags` (already negotiated). Compatible with any modern IRC server or bouncer — clients that do not support the tag see the message as normal.

### `chghost`

When a user's vhost or ident changes, the server sends a `CHGHOST newuser newhost` command rather than a fake QUIT+JOIN pair. Uplink negotiates this capability and shows a single quiet status line — `nick changed host (newuser@newhost)` — in each channel the user shares, instead of the noisy disconnect/reconnect flood.

### `draft/react`

Emoji reactions attached to specific messages via `TAGMSG` with a `+draft/react` tag and a `+draft/reply` tag identifying the target message ID.

- **Sending:** right-click any message **timestamp** → **React** → emoji picker opens; search by name (`:thumbs`, `fire`, etc.) and click to send. Or use `/react <emoji>` after setting a reply target.
- **Receiving:** reactions are stored per-message and rendered inline below the original message as `emoji(count)` in the chat view.
- Requires `message-tags` and server support for `draft/react`. On servers that do not advertise the capability, reactions are not sent.

### `draft/message-redaction`

Removes a sent message from history and signals other clients to hide it. Requires `msgid` to identify the target message.

- **Sending:** right-click a **timestamp** on your own message → **Delete**. Only available when the server has acknowledged the `draft/message-redaction` CAP.
- **Receiving:** redacted messages are replaced with `[message deleted]` in grey italic. The message ID anchor is preserved so reply threading is not broken.

### `account-notify`

Notifies the client in real time when a user logs in or out of services (NickServ). When a nick's account changes, Uplink updates the account field stored for that nick across all shared channels. The account name is shown as a tooltip when you hover over a nick in the nick list.

### `extended-join`

When this capability is active, `JOIN` messages include the joining user's account name and real name as additional parameters. Uplink uses this to populate the account field for the nick immediately on join, without needing a WHOIS or a subsequent `account-notify`.

### `invite-notify`

When you are a member of a channel, the server notifies you any time someone is invited to that channel (not just when you personally are invited). Invite-from-outside messages post a status line to the server buffer; broadcast invites from inside a shared channel post to the channel buffer.

### `setname`

Allows users to update their real name (GECOS) field after connecting via a `SETNAME` command. When another user in a shared channel changes their real name, Uplink posts a status line in every channel you share with them: `nick changed their realname to "…"`.

### `userhost-in-names`

When active, `NAMES` replies include the full `user@host` for each nick (e.g. `@alice!alice@example.com`). Uplink strips the `!user@host` suffix before displaying nicks, so the nick list looks the same, but the underlying data is richer and ready for future ban/ignore improvements.

### `Monitor`

A standardized server command for watching when specific nicks come online or go offline. Uplink sends `MONITOR + nick1,nick2,…` after the welcome message whenever a monitor list is configured. Status changes post to the server buffer as **Now online: nick** / **Now offline: nick**.

Manage the watch list with `/monitor`:

| Subcommand | Description |
|---|---|
| `/monitor add <nick>` | Add nick to the watch list (persisted to config) |
| `/monitor del <nick>` | Remove nick from the watch list |
| `/monitor list` | Show the current watch list |
| `/monitor clear` | Clear the entire watch list |
| `/monitor status` | Ask the server for current online/offline status of all watched nicks |

The list is stored in `config.toml` under `[monitor] nicks = [...]`.

### `WHOX`

An extended version of the `WHO` command. Uplink requests `WHO <channel> %cnfa,42` after joining each channel — fetching channel, nick, flags, and account in a single query. The server reply (`354 RPL_WHOSPCRPL`) is handled by the same bot-detection logic as the regular `WHO` reply, and any account name returned fires an `accountChanged` update to populate nick list tooltips.

### `sts` (Strict Transport Security)

When a server advertises an STS policy in its `CAP LS` response, Uplink upgrades the connection automatically and caches the policy to disk (`~/.config/uplink/sts.ini`). On future connections to that host, TLS is enforced even if `ssl = false` is set in the config — the stored policy takes precedence and the TLS port from the policy is used.

- **Plain connection:** Uplink receives `sts=port=6697,duration=2678400`, immediately disconnects, and reconnects over TLS on the specified port. The policy is saved with an expiry timestamp based on the `duration` value (in seconds).
- **TLS connection:** Uplink receives `sts=duration=2678400` (no port — already on TLS), refreshes the stored policy expiry.
- **Policy removal:** If the server sends `sts=duration=0` on a TLS connection, the cached policy is deleted.

Policies survive restarts and are applied before each connection attempt. This prevents downgrade attacks where an attacker could intercept a plaintext connection before TLS is established — equivalent to HSTS in web browsers.

### `account-tag`

When negotiated, the server attaches an `account` tag to every message sent by an authenticated user. Uplink uses this to keep per-nick account information current on a per-message basis — complementing `account-notify` (login/logout notifications) and `extended-join` (account on join).

- The account name is stored with each message and shown as a **tooltip when you hover over the sender's nick in the chat view** — the same tooltip as in the nick list.
- The nick list account data is also kept current, so both tooltips always show the same value.
- If a user is not authenticated with services, no `account` tag is sent and no tooltip appears.

### `netsplit` / `netjoin` batch types

When a netsplit occurs, the server wraps the resulting flood of QUITs in a `BATCH` of type `netsplit`. When the servers reconnect, the resulting flood of JOINs arrives in a `BATCH` of type `netjoin`.

Uplink collapses both into a single status line per affected channel instead of flooding the buffer. Nick lists remain accurate — nicks are removed and re-added correctly.

**Example output:**

```
-- Netsplit: 47 users lost (irc1.example.com irc2.example.com)
-- Netjoin: 47 users returned (irc1.example.com irc2.example.com)
```

Without this, a busy network split would print one quit line per user — often hundreds — making the channel unreadable.

### `standard-replies`

A structured format for servers to send machine-readable diagnostics using three commands:

| Command | Meaning | Uplink display |
|---|---|---|
| `FAIL` | A command the client sent failed | Red error line in the relevant buffer |
| `WARN` | A non-fatal warning | Server info line |
| `NOTE` | Informational server message | Server info line |

Format: `FAIL <triggered-by-command> <code> [context...] :<description>`

Uplink routes each reply to the most relevant buffer:
- If a channel name appears in the context parameters, the message goes to that channel's buffer.
- Otherwise it goes to the active channel, or the server buffer if no channel is active.

**Example output:**

```
[FAIL] JOIN CHANNEL_BANNED: You are banned from that channel
[WARN] PRIVMSG RATE_LIMITED: You are sending messages too quickly
[NOTE] * SESSION_EXPIRY: Your session will expire in 5 minutes
```

The `[FAIL]` / `[WARN]` / `[NOTE]` prefix lets you tell them apart at a glance.

### `UTF8ONLY`

An ISUPPORT token (not a CAP) that signals the server only accepts UTF-8 encoded traffic. Uplink detects it in the `005 RPL_ISUPPORT` parameters, sets an internal flag, and posts a status line to the server buffer: `UTF8ONLY: server enforces UTF-8 encoding`. Non-UTF-8 input is not blocked client-side, but the server will reject it.

---

## Bouncer capabilities

These capabilities are negotiated only when `bouncer = "znc"` or `bouncer = "soju"` is set in the server block. They are not requested against regular IRC servers.

### `znc.in/playback` — ZNC only

When available, Uplink sends `PRIVMSG *playback :PLAY * 0` immediately after the welcome message. This tells ZNC's playback module to replay all buffered messages since last seen. Replayed messages arrive with `server-time` tags and are rendered with their original timestamps.

### `znc.in/self-message` — ZNC only

When you send a message from another client connected to the same ZNC, this capability causes ZNC to echo that message to Uplink. Echoed messages are displayed correctly as your own outgoing messages rather than appearing as incoming messages from yourself.

### `znc.in/batch` — ZNC only

Enables ZNC's batch extension for grouping related messages. Used in conjunction with `znc.in/playback` to deliver replayed history as a batch.

### `soju.im/bouncer-networks` — soju only

After CAP negotiation, Uplink sends `BOUNCER LISTNETWORKS`. soju responds with a `BOUNCER NETWORK` line for each attached network, showing the network ID, name, and connection state. Each network entry is listed in the server buffer so you can see which networks your soju instance is managing.

### `soju.im/bouncer-networks-notify` — soju only

Negotiated to receive real-time notifications when a network's connection state changes (e.g. a network goes offline or reconnects). Used to keep the network list current without polling.

### `soju.im/read` — soju only

Synchronizes your read position across all clients connected to the same soju instance. When you read messages in Uplink, it sends `MARKREAD <target> timestamp=<iso8601>` to record your position. When another client advances the read marker, soju forwards the updated marker to Uplink.

### `soju.im/no-implicit-names` — soju only

Tells soju not to send a NAMES list automatically on JOIN. This prevents duplicate nick list entries when soju is already managing channel state.

---

## Planned capabilities

### `cap-notify`

Notifies the client if the server gains or loses capabilities after the initial handshake. Lets the client adapt to capability changes dynamically.

### `draft/multiline`

Allows composing and sending messages that span multiple lines, delivered to clients as a `batch` of type `draft/multiline`. Useful for pastes and structured content.

### WebSocket transport

Conventions for carrying IRC lines over a WebSocket connection (`wss://`). Useful for connecting to servers hosted behind web infrastructure, or in environments where raw TCP is blocked.

### User metadata (`metadata-2`)

A framework for associating key-value metadata with users — display names, avatars, pronouns, homepage, color, status. Values are stored server-side and synced to clients. Negotiated via the `metadata-2` capability.

---

## Reference

Full IRCv3 specification and capability registry: [https://ircv3.net/](https://ircv3.net/)
