# LyricsQt Design Spec

Date: 2026-08-20  
Status: Approved via user handoff (see decision log)  
Reference product: [LyricsX](https://github.com/ddddxxx/LyricsX) (macOS)  
Target repo: `/home/zxn/Projects/playground/LyricsQt`

## 1. Goal

Build a Linux-first desktop lyrics client in **C++20 + Qt 6 (QWidget)** that reproduces LyricsX’s user-visible capabilities wherever Linux allows: follow the playing track, fetch/rank lyrics from multiple sources, show synchronized desktop + tray lyrics, search/import/export, offset, filters, and player life-cycle helpers. No Chinese conversion and no global hotkeys in v1 (tray context menu instead). Architecture must not block a later Windows port. License: MIT.

## 2. Non-goals

- Porting AppKit/Swift code
- True compositor-level desktop overlay identical to macOS
- Touch Bar, write-into-Apple-Music
- Shipping Flatpak in the first milestone

## 3. Approaches considered

| Approach | Summary | Verdict |
|----------|---------|---------|
| A. Monolithic Qt app | One binary owns D-Bus, HTTP, all UI | Simple, weaker reuse for panel export / Windows |
| B. Core library + GUI | `liblyricsqt` + `lyricsqt` | **Selected** |
| C. Daemon + many clients | Always-on service + thin UIs | Overkill for v1 |

## 4. Architecture

```
┌─────────────────────────────────────────────────────────┐
│ lyricsqt (QWidget app)                                  │
│  DesktopLyricsWindow │ LyricsHudWindow │ Tray │ Prefs │
└───────────────┬─────────────────────────────────────────┘
                │ Qt signals / public API
┌───────────────▼─────────────────────────────────────────┐
│ liblyricsqt                                             │
│  PlayerService (MPRIS)                                  │
│  LyricsSession (current lyrics + line index + offset)   │
│  LyricsStore (local files + cache)                      │
│  ProviderHub (parallel search + quality)                │
│  ExportServer (stdout / unix socket / optional D-Bus)   │
│  Settings (QSettings)                                   │
└─────────────────────────────────────────────────────────┘
        │ QDBus              │ QNetworkAccessManager
        ▼                    ▼
   MPRIS players         NetEase/QQ/Kugou/LRCLIB
```

### 4.1 liblyricsqt responsibilities

- Discover MPRIS players on `org.mpris.MediaPlayer2.*`
- Select player: preferred identity, else “now playing” (PlaybackStatus=Playing), else last active
- Expose `TrackInfo { id, title, artist, album, lengthUs, artUrl }` and `PlaybackState { playing, positionUs }`
- Poll position while playing (MPRIS Position is often stale without Seeked signals); interval ~200–500 ms configurable
- Own `Lyrics` model: lines with `positionMs`, `content`, optional translation, optional word-level tags
- On track change: load sidecar/cache → else multi-source search → pick best by quality → persist
- Schedule current-line updates from position + offset (same idea as LyricsX `scheduleCurrentLineCheck`)
- Emit: `trackChanged`, `playbackChanged`, `lyricsChanged`, `currentLineChanged`

### 4.2 lyricsqt responsibilities

- Frameless, translucent, always-on-top desktop lyrics window (draggable, position factors saved)
- Full lyrics HUD (scrollable list, double-click seek)
- Tray icon: menu (offset ±, search, wrong lyrics, prefs, quit); optional tray title/tooltip for current line
- Preferences dialog mirroring LyricsX groups: General / Display / Filter / Lab (no Shortcuts tab in v1)
- Wire ExportServer start/stop from settings
- Tray context menu for offset / search / toggles (no global hotkeys in v1)

### 4.3 Windows reserve (not implemented now)

- Abstract `IPlayerBackend` with `MprisBackend` now; later `SmtcBackend`
- No Linux paths hardcoded in lyrics/provider layers

## 5. Feature mapping (LyricsX → LyricsQt)

| LyricsX | LyricsQt |
|---------|----------|
| MusicPlayers.Scriptable / NowPlaying / SystemMedia | MPRIS preferred + auto now-playing |
| Desktop karaoke lyrics | Always-on-top frameless window |
| Menu bar lyrics | Tray title/tooltip + ExportServer for panels |
| Touch Bar | Cut |
| Lyrics HUD | QWidget scroll lyrics window |
| Search lyrics UI | Search dialog + provider results table |
| Offset in status menu | Tray menu + shortcuts |
| Drag-drop import/export | Same on windows |
| Load lyrics beside track | Sidecar `.lrc`/`.lrcx` next to file URL if MPRIS provides `xdg-scheme` path |
| Lyrics saving path | XDG data dir + custom path setting |
| Strict search / quality | Port quality scoring concepts |
| Bilingual lyrics | Translation attachment display |
| Chinese conversion | Cut (v1) |
| Filters | Keyword + smart empty/metadata filters |
| Launch/quit with player | Autostart + quit when designated player vanishes |
| Write to iTunes | Cut → Export / copy |
| Furigana | Deferred roadmap |
| Global shortcuts | Cut (v1); tray menu instead; optional later |
| Wrong lyrics / no search ids | Same ignore lists in settings |
| Menu bar / panel line | Tray best-effort + ExportServer for Waybar/Plasma/extensions |

## 6. Core data model

```text
TrackInfo
  QString id, title, artist, album
  qint64 lengthUs
  QUrl fileUrl   // optional
  QUrl artUrl    // optional

LyricsLine
  double positionSec
  QString content
  QString translation  // optional
  QVector<WordTag> words  // optional {double timeSec, int index}

LyricsDocument
  QVector<LyricsLine> lines
  int offsetMs
  QString sourceId
  double quality
  QUrl localUrl
  QString title, artist, language
```

Quality score (initial heuristic, tunable):

- Title/artist match boost
- Duration proximity boost
- Presence of word-level tags boost
- Translation presence small boost
- Source priority weight (user-configurable order)

## 7. Provider hub

Interface:

```text
class ILyricsProvider {
  virtual QString id() const = 0;
  virtual void search(const LyricsSearchRequest& req) = 0; // async results via signal
};
```

`ProviderHub` runs enabled providers in parallel, merges results, cancels in-flight on track change, applies timeout (default 10s), picks highest quality unless user picks manually in Search UI.

Initial providers: NetEase, QQ Music, Kugou, LRCLIB.  
Dead sources (Xiami, ViewLyrics) are not implemented.

## 8. UI design (QWidget)

### 8.1 Desktop lyrics window

- `Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool`
- Translucent background; user colors for text / progress / shadow / background
- One-line or two-line (next line or translation)
- Vertical mode flag reserved (layout flip)
- Hide when paused if setting enabled
- Drag to reposition; store x/y position factors relative to available geometry
- Wayland note: some compositors ignore always-on-top or positioning; document limitations; still ship best-effort

### 8.2 Lyrics HUD

- List of lines; highlight current
- Double-click → `PlayerService::seek(positionSec)`
- Drag-drop `.lrc`/`.lrcx` import

### 8.3 Tray + export

- Tray menu actions parity with LyricsX status item essentials (primary control surface; no global hotkeys in v1)
- ExportServer modes (settings) — **stable extension point for future top-bar/panel UIs**:
  - Unix socket line protocol: each current line as UTF-8 + `\n`
  - Optional stdout when run with `--pipe`
  - Session D-Bus interface `org.lyricsqt.Export` with at least `CurrentLine` (and preferably track fields + playing flag)

Future GNOME Shell extension / Plasma plasmoid / Waybar module should speak only this IPC. They must not reimplement MPRIS or providers. No liblyricsqt redesign required to add them.

### 8.4 Preferences

Tabs: General, Display, Filter, Advanced (Lab):

- Preferred player id / auto
- Autostart, quit with player
- Saving path, load beside track
- Strict search, bilingual
- Desktop/tray/export toggles
- Fonts and colors
- Filter keywords
- Provider enable list and order

## 9. Settings storage

`QSettings` under org `lyricsqt`, app `LyricsQt`.  
Keys named after LyricsX semantics where sensible (e.g. `DesktopLyricsEnabled`) to ease mental mapping—not binary compatible.

## 10. Error handling

- No player: clear lyrics UI, idle tray
- Search failure: keep last local if any; otherwise empty with HUD hint
- Provider HTTP errors: isolate per provider; others continue
- Seek unsupported by player: ignore with status message

## 11. Testing strategy

- Unit (Qt Test): LRC/LRCX parser, quality ranking, line-index given time+offset, filter rules
- Integration (manual script): mock MPRIS via a test player or `playerctl` against real Spotify/VLC
- DE checklist: GNOME Wayland + KDE Wayland — top window, tray, shortcuts, export to Waybar/Plasma

## 12. Repository layout

```text
LyricsQt/
  CMakeLists.txt
  README.md
  LICENSE
  docs/superpowers/...
  src/
    liblyricsqt/
      player/
      lyrics/
      providers/
      store/
      export/
      settings/
    app/
      main.cpp
      ui/
  tests/
  resources/
```

## 13. Delivery phases (implementation order)

1. **Skeleton** — CMake, lib+app, empty window, settings stub  
2. **MPRIS** — track/state/position, player picker  
3. **LRC parser + sync** — local file load, current line, HUD highlight  
4. **Desktop + tray UI** — overlay, tray menu, offset  
5. **ProviderHub** — LRCLIB first then NetEase/QQ/Kugou, cache persist  
6. **Search UI + import/export + filters**  
7. **Autostart/quit + ExportServer** (no global shortcuts)  
8. **Polish** — bilingual/vertical, DE docs, packaging notes; furigana / hotkeys later

Each phase must leave a runnable app.

## 14. Risks

| Risk | Mitigation |
|------|------------|
| Wayland overlay limits | Frameless top window; document; panel export as equal-class feature |
| Provider API breakage | Provider interface + per-source adapters; LRCLIB as reliable baseline |
| MPRIS Position drift | Periodic refresh + Seeked listener |
| GNOME tray weakness | ExportServer for panel; tray still for controls |

## 15. Success criteria

- With a common MPRIS player, track changes update lyrics automatically from cache or network  
- Desktop window shows correct synchronized line; offset works  
- Tray controls work on KDE; on GNOME at least menu actions work  
- Waybar/Plasma can display current line via export  
- Manual search, wrong-lyrics ignore, drag-drop import work  
- Same codebase builds; player backend is swappable for future Windows
