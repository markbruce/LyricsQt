# LyricsQt Decision Log

Date: 2026-08-20
Source: brainstorming session (LyricsX → Linux/Qt rewrite)
Authority: user answered A/B/C for early items, then authorized remaining decisions to the agent with a decision log.

## Decisions made with user

| ID | Topic | Choice | Meaning |
|----|--------|--------|---------|
| D1 | Scope | A | Full Linux-equivalent feature parity; cut or rewrite macOS-only features |
| D2 | Platforms | B | Linux first (GNOME/KDE Wayland); architecture reserved for Windows later |
| D3 | UI toolkit | A | Qt 6 QWidget only (no QML) |
| D4 | Menu-bar lyrics equivalent | C | System tray lyrics + export API for Waybar/Plasma/etc. |
| D5 | Online lyrics sources | B | Multi-source parallel search + quality ranking from day one |

## Decisions made by agent (user handoff)

| ID | Topic | Choice | Rationale |
|----|--------|--------|-----------|
| D6 | Project name / path | `LyricsQt` at `../LyricsQt` | Clear stack; sibling of LyricsX under playground |
| D7 | Process architecture | Core library `liblyricsqt` + app `lyricsqt` | Same core feeds UI, tray, and panel export; easier Windows later |
| D8 | Build | CMake, C++20, Qt 6 | Machine already has qt6-base-dev / cmake / g++ |
| D9 | Player integration | Session MPRIS via QDBus; auto-pick active player (playerctld-like) | Linux equivalent of MusicPlayer Scriptable + NowPlaying |
| D10 | Lyrics formats | LRC + LRCX (incl. word time tags, translations) | Match LyricsX file compatibility |
| D11 | First provider set | NetEase, QQ, Kugou, LRCLIB; plugin-style interface for more | Closest to LyricsKit; LRCLIB is reliable globally |
| D12 | Chinese conversion | **CUT (user 2026-08-21)** | No OpenCC / no s2t-t2s |
| D13 | Global shortcuts | **CUT for v1 (user 2026-08-21)** | Use tray icon context menu; record as future optional only |
| D14 | Seek from lyrics | MPRIS `SetPosition` / `Seek` | Parity with double-click jump |
| D15 | Follow player life-cycle | Optional: quit when designated player exits; XDG autostart | Parity with launch/quit helper, without macOS helper app |
| D16 | Furigana | Roadmap phase after core parity | High complexity / niche; not block release |
| D17 | Write-to-Apple-Music | Cut | No Linux equivalent; export/copy instead |
| D18 | Touch Bar | Cut | Hardware/OS specific |
| D19 | Desktop overlay model | Frameless always-on-top QWidget (**confirmed user 2026-08-21**) | Wayland GNOME/KDE cannot do LyricsX-style true desktop layer uniformly |
| D20 | License | **MIT (user 2026-08-21: prefer more permissive OSS)** | Was MPL-2.0; switched to MIT |
| D21 | Packaging | CMake install first; Flatpak later | Unblock development |
| D22 | Tests | Qt Test for parser, ranking, sync math; manual DE checklist | Deterministic core; UI verified on both desktops |
| D23 | Spec/plan location | In LyricsQt repo under `docs/superpowers/` | New project owns its docs |
| D24 | Implementation sequencing | Phased delivery inside one program plan | Full design now; build vertical slices that always run |

## Amendments (2026-08-21 user review)

| ID | Change |
|----|--------|
| D12 | No Chinese conversion |
| D13 | No global shortcuts in v1; tray right-click menu covers offset/search/toggle |
| D19 | Confirmed frameless always-on-top window |
| D20 | License → MIT |
| D25 | Panel/top-bar lyrics: not a Qt “clock widget” inside GNOME/KDE chrome; see note below |
| D16 | Furigana deferred — reconfirmed OK |

### D25 Panel / top-bar note

Clock updates because it is **part of the shell/panel**, not because a normal Qt app can draw into the top bar.

Portable options (keep in product):
1. Frameless always-on-top window (desktop lyrics)
2. System tray / StatusNotifier (controls + best-effort line text)
3. ExportServer (socket / `--pipe` / D-Bus) so Waybar, Plasma widget, or a tiny script shows the line

DE-native options (optional later, **not a core rewrite**):
- GNOME: Shell Extension (JS) as a **thin client** of ExportServer / session D-Bus
- KDE: Plasma plasmoid as the same kind of thin client

**Stability guarantee (leave the door open):** `liblyricsqt` owns a versioned export IPC (at minimum `CurrentLine`, ideally also `TrackTitle`/`Artist`/`Playing`). Desktop window, tray, Waybar scripts, and future GNOME/KDE panel plugins are all consumers of that API. Adding true top-bar text later = new small frontend repo or `contrib/` plugin, **not** a redesign of PlayerService / LyricsSession / providers.

v1 ships ExportServer + documents the IPC; does **not** require shipping a GNOME extension or Plasma widget in-tree.

## Explicit non-goals (v1 product)

- Binary compatibility with LyricsX preferences/plist
- Reusing Swift LyricsKit/MusicPlayer at runtime
- GNOME Shell extension or KWin script as **required** primary UI
- Chinese conversion (OpenCC)
- Global hotkeys
- Electron / Tauri / PySide
