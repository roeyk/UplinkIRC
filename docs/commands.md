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

### Examples

```
/join #linux
/j #uplink
/part
/part #linux see you later
```

---

## Messaging

| Command | Description |
|---|---|
| `/me <text>` | Send an action message. Shown as `* yournick text` in chat |
| `/msg <nick> <text>` | Send a private message to a user |

### Examples

```
/me waves hello
/msg alice hey, are you around?
```

---

## Nick and identity

| Command | Description |
|---|---|
| `/nick <name>` | Change your nickname |

### Example

```
/nick coolnick
```

Nick changes take effect immediately. If the nick is already in use on the server you will get an error in the server buffer.

---

## Connection

| Command | Description |
|---|---|
| `/quit [message]` | Disconnect from the current server with an optional quit message |

### Examples

```
/quit
/quit later everyone
```

---

## Raw IRC protocol

| Command | Description |
|---|---|
| `/raw <line>` | Send a raw IRC protocol line directly to the server |
| `/quote <line>` | Alias for `/raw` |

Use these when you need to send a command that UplinkIRC does not have a built-in shortcut for yet.

### Examples

```
/raw PRIVMSG #uplink :hello
/raw MODE #uplink +m
/raw WHOIS alice
/raw TOPIC #uplink New topic here
/quote JOIN #test
```

---

## Notes

- Commands are case-insensitive: `/JOIN` works the same as `/join`.
- Any input that does not start with `/` is sent as a regular message to the current channel or user.
- If you want to send a message that starts with `/` literally, prefix it with a space.
