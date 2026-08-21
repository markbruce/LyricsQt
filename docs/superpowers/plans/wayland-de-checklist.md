# Wayland DE verification checklist

Date: 2026-08-21  
Primary targets: **GNOME Wayland**, **KDE Plasma Wayland**  
This machine at checklist time: `XDG_SESSION_TYPE=wayland`, `XDG_CURRENT_DESKTOP=GNOME` (Debian 13).

Legend:

| Tag | Meaning |
|-----|---------|
| **CI/unit** | Covered by `ctest` or other automated tests in this repo |
| **Env-ok** | Can be checked on *this* GNOME Wayland session without a second DE |
| **Manual** | Needs interactive UI / real player / panel; user must confirm |
| **Other-DE** | Requires a KDE Wayland session (not available here) |

## Checklist

| # | Item | GNOME Wayland | KDE Wayland | Notes |
|---|------|---------------|-------------|-------|
| 1 | MPRIS detect Spotify / VLC / browser | Env-ok + Manual | Other-DE + Manual | Session bus already exposes browser/QQ Music MPRIS (`org.mpris.MediaPlayer2.*`). Full Spotify/VLC UX still Manual. Unit coverage: player selection logic only via integration/manual. |
| 2 | Desktop lyrics window visible | Manual | Other-DE + Manual | Frameless always-on-top QWidget (D19). Compositor may ignore keep-on-top or absolute positioning. |
| 3 | Desktop window draggable; position restored | Manual | Other-DE + Manual | Position factors in QSettings; drag/release must be exercised in UI. |
| 4 | Tray menu usable (StatusNotifier) | Manual | Other-DE + Manual | Primary controls in v1 (no global hotkeys). GNOME may need AppIndicator extension for tray icons. |
| 5 | Export → Waybar custom module (socket / `--pipe`) | Env-ok (socket/D-Bus smoke) + Manual (Waybar) | Other-DE + Manual | **CI/unit:** `test_export_server`. Socket path `$XDG_RUNTIME_DIR/lyricsqt.sock`; D-Bus `org.lyricsqt.Export`. Waybar/Plasma widget display is Manual. |
| 6 | Search + offline cache after restart | Manual | Other-DE + Manual | **CI/unit:** store/parser/settings round-trips. End-to-end search + relaunch with same track is Manual. |
| 7 | Offset ± (tray menu) | Manual | Other-DE + Manual | Tray “offset ±”; persists as `GlobalLyricsOffset`. No global hotkeys in v1. |
| 8 | Quit with player | Manual | Other-DE + Manual | Preferences → quit when preferred/active player exits; confirm process exits when MPRIS name leaves the bus. |

## Environment snapshot (2026-08-21)

Verified without launching the full GUI:

- Session is Wayland + GNOME.
- MPRIS names present on the user bus (example at checklist time: Chromium/Edge instances, QQ Music).
- `ctest --test-dir build --output-on-failure` is the automated gate for core/export behavior (see README / latest commit notes).

Not verified in this environment:

- Interactive desktop overlay visibility, stacking, and drag on GNOME.
- Tray icon + full context menu under GNOME (indicator extension dependent).
- Waybar / Plasma panel consuming export live.
- Any KDE Plasma Wayland run.

## How to run manual checks

1. Build and start: `./build/src/app/lyricsqt` (optional `--pipe` for stdout export).
2. Play a track in Spotify, VLC, or a browser tab with MPRIS; confirm HUD/desktop/tray update.
3. Right-click tray: toggle desktop/tray line, offset ±, search, preferences, quit.
4. Enable panel export; point Waybar at the socket (see README) or watch D-Bus `CurrentLine`.
5. Enable “Quit when preferred/active player exits”, quit the player, confirm LyricsQt exits.
6. Repeat on a KDE Wayland session when available; record results below.

## Results log

| Date | DE | Tester | Result | Notes |
|------|----|--------|--------|-------|
| 2026-08-21 | GNOME Wayland | agent (this host) | Partial | Bus/MPRIS presence + unit/export tests only; UI/tray/Waybar/quit-with-player not interactively confirmed |
| — | KDE Wayland | — | Pending | Needs user session |
