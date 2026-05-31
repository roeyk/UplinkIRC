# Slash Commands

Type any of these commands in the message input box and press Enter.

---

## Channel commands

| Command | Description |
|---|---|
| `/join #channel` | Join a channel |
| `/j #channel` | Short alias for `/join` |
| `/part [#channel]` | Leave a channel. Defaults to the current channel if omitted |
| `/part #channel reason` | Leave with a part message |
| `/topic` | Show the current channel topic |
| `/topic <text>` | Set the current channel topic |
| `/topic #channel <text>` | Set the topic on a specific channel |
| `/kick <nick> [reason]` | Kick a user from the current channel (requires op) |
| `/invite <nick>` | Invite a user to the current channel |
| `/invite <nick> #channel` | Invite a user to a specific channel |
| `/mode <target> <flags> [args]` | Set channel or user modes |
| `/op <nick>` | Give op (`+o`) in the current channel |
| `/deop <nick>` | Remove op (`-o`) in the current channel |
| `/voice <nick>` | Give voice (`+v`) in the current channel |
| `/devoice <nick>` | Remove voice (`-v`) in the current channel |
| `/ban <mask>` | Ban a mask (`+b`) in the current channel |
| `/unban <mask>` | Remove a ban (`-b`) in the current channel |
| `/ignore <nick>` | Suppress all messages from a nick (client-side) |
| `/unignore <nick>` | Stop ignoring a nick |
| `/ignored` | List all currently ignored nicks |
| `/monitor add <nick>` | Add a nick to the online/offline watch list (IRCv3 MONITOR) |
| `/monitor del <nick>` | Remove a nick from the watch list |
| `/monitor list` | Show all watched nicks |
| `/monitor clear` | Clear the entire watch list |
| `/monitor status` | Ask the server for current online/offline status |
| `/clear` | Clear the chat buffer |

### Examples

```
/join #linux
/j #uplink
/part
/part #linux see you later
/topic new topic here
/topic #uplink Welcome to the server
/kick baduser spamming
/invite alice
/invite alice #linux
/mode #uplink +m
/op alice
/deop bob
/voice alice
/ban *!*@spammer.host
/unban *!*@spammer.host
/ignore spammer
/unignore spammer
/ignored
/clear
```

---

## Messaging

| Command | Description |
|---|---|
| `/me <text>` | Send an action message. Shown as `* yournick text` in chat |
| `/msg <nick> <text>` | Send a private message — opens a PM buffer in the sidebar for the conversation |
| `/query <nick>` | Open a PM buffer without sending a message |
| `/notice <target> <text>` | Send a NOTICE to a user or channel |

### Examples

```
/me waves hello
/msg alice hey, are you around?
/query alice
/notice alice heads up
```

---

## Services shortcuts

Shortcuts for sending messages to network services. These are equivalent to `/msg <Service> <text>` but shorter to type.

| Command | Equivalent |
|---|---|
| `/ns <text>` | `/msg NickServ <text>` |
| `/cs <text>` | `/msg ChanServ <text>` |
| `/bs <text>` | `/msg BotServ <text>` |
| `/ms <text>` | `/msg MemoServ <text>` |

### Examples

```
/ns identify mypassword
/ns register mypassword email@example.com
/cs op #uplink
/cs invite #uplink
/ms list
```

---

## Nick and identity

| Command | Description |
|---|---|
| `/nick <name>` | Change your nickname |
| `/away [message]` | Set yourself as away with an optional message |
| `/back` | Clear your away status |
| `/whois <nick>` | Look up info about a user |

### Examples

```
/nick coolnick
/away grabbing coffee
/back
/whois alice
```

---

## Connection & IRC operator

| Command | Description |
|---|---|
| `/quit [message]` | Disconnect from the current server with an optional quit message |
| `/motd [server]` | Request the message of the day |
| `/oper <user> <pass>` | IRC operator login — sends `OPER user :pass` to the server |

### Examples

```
/quit
/quit later everyone
/motd
/oper myname mysecretpassword
```

---

## CTCP

| Command | Description |
|---|---|
| `/ping <nick>` | Send a CTCP PING to a user — reply shows round-trip time in the active channel |
| `/time <nick>` | Ask a user for their local time — reply appears in the active channel |
| `/version [nick]` | Request server version, or CTCP VERSION from a nick |
| `/ctcp <nick> <command> [args]` | Send a raw CTCP request to a user |
| `/sysinfo` | Post OS, CPU, RAM, GPU, and uptime info to the current channel |

Incoming CTCP VERSION and PING requests are answered automatically.

> **Note:** `/time` and `/ping` only work if the target client supports those CTCP commands. Most IRC clients do, but bots often do not respond. If you see no reply, the other side simply does not support it.

### Examples

```
/ping alice
/time alice
/version
/version alice
/ctcp alice CLIENTINFO
/sysinfo
```

---

## File transfer (DCC)

DCC Send is initiated from the **nick list right-click menu**, not a slash command.

**Sending a file:**

1. Right-click any nick in the user list on the right side of the window.
2. Choose **Send File**.
3. Pick a file in the file dialog.
4. A progress dialog appears while UplinkIRC waits for the recipient to accept.

**Receiving a file:**

When someone sends you a file, a dialog appears:

```
alice wants to send you:
report.pdf  (2.1 MB)

Accept?   [Yes]  [No]
```

Click **Yes**, choose a save location, and a progress dialog tracks the download. Click **Cancel** at any time to abort.

> **NAT note:** DCC works directly between clients over TCP. It works reliably on the same LAN. Over the internet, it requires the sender's port to be reachable — NAT or a firewall on the sender's side will block the connection. This is a fundamental DCC limitation, not an UplinkIRC bug.

---

## Nick list right-click menu

Right-clicking any nick — in the user list or directly on a nick link in the chat view — opens the same context menu. The title shows the nick in bold.

| Action | What it does |
|---|---|
| **Message** | Opens a private message buffer in the sidebar. Equivalent to `/msg nick`. |
| **Send File** | Opens a file picker and sends the file via DCC SEND. |
| **Whois** | Sends `WHOIS nick`. The response appears in the server window. |
| **Invite** | Opens a dialog pre-filled with the current channel. Edit if needed and click OK to send `INVITE nick #channel`. |
| **Give Op** | Sets `+o` on the nick. Requires op. |
| **Take Op** | Removes `-o` from the nick. Requires op. |
| **Give Voice** | Sets `+v` on the nick. Requires op or half-op. |
| **Take Voice** | Removes `-v` from the nick. Requires op or half-op. |
| **Version** | Sends a CTCP VERSION request. Reply appears in the server window. |
| **Ping** | Sends a CTCP PING. Reply shows RTT in the active buffer: `Ping reply from nick: Xms`. |
| **Copy Nick** | Copies the nickname to the clipboard. |
| **Ignore / Unignore** | Suppresses all messages (PRIVMSG, NOTICE, ACTION) from this nick. The label toggles to **Unignore** if the nick is already ignored. Persists in config. |
| **Kick** | Prompts for an optional reason, then kicks. Requires op. |
| **Ban** | Sets `MODE #channel +b nick!*@*`. Requires op. |
| **Kick & Ban** | Bans first, then kicks (correct order). Prompts for reason. Requires op. |

### Examples

```
# Right-click alice → Invite
# Dialog pre-filled: #uplink   → click OK
# Sends: INVITE alice #uplink

# Right-click spammer → Kick & Ban
# Reason dialog: "flooding"
# Sends: MODE #uplink +b spammer!*@*
#         KICK #uplink spammer :flooding

# Right-click alice → Ping
# Buffer: Ping reply from alice: 42ms
```

---

## Reactions

UplinkIRC supports IRCv3 `draft/react` — emoji reactions attached to specific messages.

**Sending a reaction:**
- Right-click any message **timestamp** in the chat view → **React** → type an emoji → OK
- Or use `/react <emoji>` after right-clicking a timestamp and choosing **Reply** (sets the message target)

**Receiving reactions:**
- Reactions appear inline below the original message as `emoji(count)` whenever they are received

**Requirements:** the server must support the `draft/react` capability. If the server does not advertise it, reactions are silently skipped.

### Examples

```
# Right-click a timestamp → React → type 👍 → OK

# Via slash command (requires a reply target set first):
/react 🔥
/react 👍
```

---

## Message timestamp right-click

Right-clicking a message **timestamp** (the `hh:mm` at the left of each line) opens a context menu for that specific message:

| Action | What it does |
|---|---|
| **Reply** | Sets this message as the reply target. A `↩ nick` bar appears above the input. Press Enter to send; Escape to cancel. |
| **React** | Opens an emoji input dialog. The emoji is sent as an IRCv3 `+draft/react` reaction attached to this message's ID. Requires server support for `draft/react`. |

---

## Raw IRC protocol

| Command | Description |
|---|---|
| `/raw <line>` | Send a raw IRC protocol line directly to the server |
| `/quote <line>` | Alias for `/raw` |
| `/<ANY>` | Any unrecognized slash command is sent as a raw IRC line automatically |

Use these when you need to send a command that UplinkIRC does not have a built-in shortcut for yet. You can also just type the command directly — unrecognized `/CMD` inputs are passed through to the server as-is.

### Examples

```
/raw MODE #uplink +m
/raw INVITE alice #uplink
/quote JOIN #test

/REHASH
/SAMODE #uplink +o alice
/GLOBOPS Hello opers
```

---

## Help

| Command | Description |
|---|---|
| `/help` | Print a list of all available commands in the current chat buffer |

---

---

## Emoji

UplinkIRC has two ways to insert emoji:

### Emoji picker button
Click ☰ to open **Preferences** and check **Show Emoji Button** (or set `show_emoji_button = true` in config). Click the 😊 button next to the input box to open a searchable grid of ~400 emoji.

### :shortcode: typing
Type a colon followed by a keyword and a completion list appears:

| You type | Result |
|---|---|
| `:smi` | autocomplete list: 😄 smile, 😃 smiley, … |
| `:trident:` | auto-replaces to 🔱 as soon as you type the closing `:` |
| `:fire:` + Enter | sends 🔥 in the message |

- Use **Up/Down** arrows to navigate the list, **Enter** or **Tab** to confirm, **Escape** to dismiss.
- Any `:shortcode:` patterns still in the text when you press Enter are resolved before the message is sent.

---

## Notes

- Commands are case-insensitive: `/JOIN` works the same as `/join`.
- Any input that does not start with `/` is sent as a regular message to the current channel or user.
- If you want to send a message that starts with `/` literally, prefix it with a space.
