# LyricsQt

Linux-first desktop lyrics client inspired by [LyricsX](https://github.com/ddddxxx/LyricsX), built with C++20 and Qt 6 (QWidget).

## Docs

- Decision log: `docs/superpowers/decisions/2026-08-20-decision-log.md`
- Design spec: `docs/superpowers/specs/2026-08-20-lyricsqt-design.md`
- Implementation plan: `docs/superpowers/plans/2026-08-20-lyricsqt-implementation.md`

## Features

- MPRIS (D-Bus) for now-playing
- Multi-source lyrics search (NetEase, QQ, Kugou, LRCLIB)
- Desktop overlay + tray + panel export (unix socket / `--pipe` / D-Bus)
- GNOME and KDE on Wayland as primary targets

## Controls (v1)

Global hotkeys are deferred. Use the system tray context menu for primary controls: toggle desktop lyrics, toggle tray line, show HUD, offset ±, search, wrong lyrics, preferences, and quit.

Optional: Preferences → Launch at login (XDG autostart) and Quit when preferred/active player exits.

## Panel export (Waybar / Plasma / extensions)

Stable IPC for future GNOME Shell extensions and Plasma widgets. Clients must speak only this protocol (do not reimplement MPRIS or providers).

Enable in Preferences → Advanced → “Enable panel export server”, or run with `--pipe`.

### Protocol

| Channel | Details |
|---------|---------|
| Unix socket | `$XDG_RUNTIME_DIR/lyricsqt.sock` — each current line as UTF-8 + `\n` (empty line when idle) |
| Stdout | `lyricsqt --pipe` — same line protocol on stdout |
| D-Bus | Service `org.lyricsqt.Export`, path `/org/lyricsqt/Export`, interface `org.lyricsqt.Export` |

D-Bus properties: `CurrentLine` (s), `Title` (s), `Artist` (s), `Playing` (b). Changes emit `org.freedesktop.DBus.Properties.PropertiesChanged`.

### Waybar custom module (socket)

```json
"custom/lyrics": {
    "exec": "socat -u UNIX-CONNECT:$XDG_RUNTIME_DIR/lyricsqt.sock -",
    "restart-interval": 1,
    "escape": true
}
```

Or with `--pipe` (single process feeds the module):

```json
"custom/lyrics": {
    "exec": "lyricsqt --pipe",
    "restart-interval": 2,
    "escape": true
}
```

Prefer the socket module when LyricsQt already runs as a tray app; use `--pipe` only for dedicated headless/panel feeds.

### Plasma / D-Bus (qdbus example)

```bash
qdbus org.lyricsqt.Export /org/lyricsqt/Export org.freedesktop.DBus.Properties.Get \
  org.lyricsqt.Export CurrentLine

# Watch property changes (requires dbus-monitor):
dbus-monitor "type='signal',interface='org.freedesktop.DBus.Properties',path='/org/lyricsqt/Export'"
```

A thin Plasma plasmoid or GNOME extension should subscribe to `PropertiesChanged` (or read the unix socket) rather than talking to MPRIS directly.

## Build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/src/app/lyricsqt
```

Dependencies: Qt 6 (Core, Gui, Widgets, DBus, Network, Test), CMake ≥ 3.16, C++20 compiler.
