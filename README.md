# LyricsQt

Linux-first desktop lyrics client inspired by [LyricsX](https://github.com/ddddxxx/LyricsX), built with C++20 and Qt 6 (QWidget).

## Docs

- Decision log: `docs/superpowers/decisions/2026-08-20-decision-log.md`
- Design spec: `docs/superpowers/specs/2026-08-20-lyricsqt-design.md`
- Implementation plan: `docs/superpowers/plans/2026-08-20-lyricsqt-implementation.md`
- Wayland DE checklist: `docs/superpowers/plans/wayland-de-checklist.md`

## Features

- MPRIS (D-Bus) for now-playing
- Multi-source lyrics search (NetEase, QQ, Kugou, LRCLIB)
- Desktop overlay + tray + panel export (unix socket / `--pipe` / D-Bus)
- GNOME and KDE on Wayland as primary targets

## Build dependencies

Debian / Ubuntu (example packages):

```bash
sudo apt install build-essential cmake \
  qt6-base-dev qt6-base-dev-tools \
  qt6-tools-dev qt6-tools-dev-tools
```

Needed Qt 6 modules: Core, Gui, Widgets, DBus, Network, Test (pulled in via `qt6-base-dev` / tools). CMake ≥ 3.16 and a C++20 compiler (`g++` or `clang++`).

Optional runtime helpers for panel examples: `socat`, `qdbus` / `qt6-tools-dev-tools`, a MPRIS player (`playerctl` useful for debugging).

## Build and run

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/src/app/lyricsqt
```

Useful flags:

| Flag | Effect |
|------|--------|
| `--pipe` | Also print current lyric lines to stdout (panel / Waybar `exec`) |

Tests:

```bash
ctest --test-dir build --output-on-failure
```

## Settings paths

| What | Location |
|------|----------|
| QSettings (INI) | `~/.config/lyricsqt/LyricsQt.conf` |
| Cached / saved lyrics (default) | `~/.local/share/lyricsqt/LyricsQt/` (override in Preferences) |
| XDG autostart entry | `~/.config/autostart/lyricsqt.desktop` |
| Panel export socket | `$XDG_RUNTIME_DIR/lyricsqt.sock` |

Organization / application names used by Qt: `lyricsqt` / `LyricsQt`.

## Controls (v1)

Global hotkeys are deferred. Use the system tray context menu for primary controls: toggle desktop lyrics, toggle tray line, show HUD, offset ±, search, wrong lyrics, preferences, and quit.

Optional: Preferences → Launch at login (XDG autostart) and Quit when preferred/active player exits.

## Wayland limitations

- Desktop lyrics are a frameless always-on-top `QWidget`, not a compositor “desktop layer” like LyricsX on macOS. GNOME/KDE Wayland may ignore keep-above, restrict positioning, or clip translucency.
- Tray (StatusNotifier) depends on the DE; on GNOME you may need an AppIndicator / tray extension for the icon and menu.
- LyricsQt cannot draw into the GNOME top bar or Plasma panel chrome itself. Use the desktop window, tray line, or **ExportServer** (socket / `--pipe` / D-Bus) for Waybar, Plasma widgets, or future shell extensions.
- See `docs/superpowers/plans/wayland-de-checklist.md` for GNOME/KDE verification status.

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

## Deferred / out of v1

| Item | Status | Decision |
|------|--------|----------|
| Furigana (ruby annotations) | Deferred roadmap | D16 |
| Global hotkeys | Deferred (tray menu instead) | D13 |
| Chinese conversion (OpenCC s2t/t2s) | Cut for product | D12 |
| Windows SMTC player backend | Architecture reserved; not implemented | D2, design `SmtcBackend` stub |

## License

MIT
