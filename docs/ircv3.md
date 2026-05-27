# IRCv3 Support

DojoIRC aims to be a first-class IRCv3 client. Below is the current status of every capability, what it does, and what's planned.

---

## Done

### `message-tags`
CAP negotiation is wired. Tags are parsed on all inbound messages and stored with each message object. This is the foundation that `typing` and other tag-based features build on.

### `draft/typing`
Outgoing typing indicators are sent automatically while you type (debounced — sends `active`, then `paused` after a pause, then `done` on send or clear). Incoming typing indicators from other users appear above the input bar, e.g. `alice is typing…`. Both sides require the server and the other client to support `draft/typing`.

### `sasl` (PLAIN)
SASL PLAIN authentication is supported per server via `[server.sasl]` in config. The full CAP LS 302 handshake is used. Authentication happens before JOIN so your nick is identified before entering channels. SASL EXTERNAL (certificate-based) is planned.

DojoIRC uses CAP LS 302 with multiline accumulation: it waits for the full capability list before deciding what to request, and only requests capabilities the server actually advertises. This avoids all-or-nothing NAK failures when batching multiple caps.

### `server-time`
CAP negotiated. When the server supplies a `time` tag on incoming messages, DojoIRC uses that timestamp instead of the local clock. Message timestamps in the chat window reflect server time on supporting servers. Important for accurate logs and bouncer history replay.

### `away-notify`
CAP negotiated. When a user in a shared channel sets or clears their away status, the server sends an AWAY message with or without a reason. DojoIRC tracks this and can surface it in future nick list UI. Numerics 305 (RPL_UNAWAY) and 306 (RPL_NOWAWAY) are also handled — when you `/away` or `/back`, the away badge next to your nick in the input bar updates immediately.

---

## Planned

### `batch`
Groups related messages together so the client can handle them atomically. Required for `chathistory` replay and other bulk operations. Low-level plumbing that other features depend on.

### `labeled-response`
Lets the client tag an outgoing command with a label and match the server's response to it. Needed for reliable echo-message and other request/response flows.

### `multi-prefix`
Returns all mode prefixes for a nick in NAMES replies (e.g. `@+alice` instead of just `@alice`). Required to accurately show users who have both op and voice in the nick list.

### `extended-join`
Includes the joining user's account name in JOIN messages. Makes it possible to show account names in join notifications without a separate WHOIS.

### `account-notify`
Notifies the client when a user's account association changes (logs in or out of services). Useful for showing account status in the nick list.

### `invite-notify`
Broadcasts INVITE messages to all members of a channel (not just the inviter), so the channel can see who is being invited.

### `chghost`
Allows a user's hostname to change without disconnecting and reconnecting. Displayed as a status message in shared channels.

### `userhost-in-names`
Includes the full `user@host` in NAMES replies. Lets the client build a full picture of connected users without additional WHO queries.

### `setname`
Allows a user to change their real name (GECOS) without reconnecting.

### `chathistory`
Replays message history from the server or bouncer. When implemented, switching to a channel will load recent history automatically. Depends on `batch`.

### `echo-message`
The server echoes sent messages back to the sender as confirmation of delivery. Enables proper sent-message timestamps and deduplication with `msgid`.

### `msgid`
Assigns a unique ID to every message. Enables deduplication, threading, and reliable reference to specific messages (required by `react` and `message-redaction`).

### `Monitor`
Efficiently tracks whether specific nicks are online. More scalable than ISON polling. Will be used to implement a friends/watch list feature.

### `cap-notify`
Notifies the client if the server gains or loses capabilities after the initial handshake. Allows the client to react to capability changes on the fly.

### `multiline`
Allows a single logical message to span multiple IRC lines. The client would present them as one message block.

### `react`
Emoji reactions to specific messages (identified by `msgid`). Requires `msgid` to be implemented first.

### `read-marker`
Syncs the read position for a channel across multiple clients. When you read a channel on one device, other devices know where you left off.

### `Standard Replies`
Structured machine-readable error and info replies from the server. Replaces inconsistent numeric replies with well-defined response objects.

### `sts` (Strict Transport Security)
If a server advertises STS, the client remembers to always use TLS for that server and refuses plaintext connections. Prevents downgrade attacks. Similar to HSTS in web browsers.

### `utf8only`
Client and server both declare UTF-8 as the exclusive encoding. Removes all encoding ambiguity on modern servers.

### `draft/message-redaction`
Allows message authors or channel operators to delete a message with a reason. Requires `msgid`. Supported by Ergo.

### `draft/account-registration`
In-band account registration directly from the IRC client via CAP — no NickServ commands required. Supported by Ergo. Will make onboarding new users much simpler.

### `draft/channel-rename`
Allows a channel to be renamed server-side without all users having to part and rejoin. Supported by Ergo.

### `WHOX`
Extended WHO command (an ISUPPORT feature, not a CAP) that returns account names, idle time, and real hostname in a single request. Will make the nick list and WHOIS output considerably richer.

---

## Reference

Full IRCv3 specification: [https://ircv3.net/irc/](https://ircv3.net/)
