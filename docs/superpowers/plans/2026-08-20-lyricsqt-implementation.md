# LyricsQt Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build Linux-first LyricsX-equivalent lyrics client (`LyricsQt`) in C++20 + Qt 6 QWidget with `liblyricsqt` + `lyricsqt` app.

**Architecture:** Shared core library owns MPRIS, lyrics model, providers, store, export; QWidget app owns desktop overlay, HUD, tray, preferences. See `docs/superpowers/specs/2026-08-20-lyricsqt-design.md`.

**Tech Stack:** CMake, C++20, Qt 6 (Core, Gui, Widgets, DBus, Network, Test), MPRIS over session bus. No OpenCC. No global hotkeys in v1 (MIT license).

**Decision log:** `docs/superpowers/decisions/2026-08-20-decision-log.md`

---

## File structure (create over phases)

```text
LyricsQt/
  CMakeLists.txt
  README.md
  LICENSE
  cmake/LyricsQtConfig.cmake.in
  src/liblyricsqt/
    CMakeLists.txt
    include/lyricsqt/*.h          # public headers
    player/MprisPlayerBackend.*
    player/PlayerService.*
    lyrics/LyricsDocument.*
    lyrics/LrcParser.*
    lyrics/LyricsSession.*
    lyrics/LyricsFilter.*
    lyrics/QualityScorer.*
    store/LyricsStore.*
    providers/ILyricsProvider.h
    providers/ProviderHub.*
    providers/LrclibProvider.*
    providers/NetEaseProvider.*
    providers/QQMusicProvider.*
    providers/KugouProvider.*
    export/ExportServer.*
    settings/AppSettings.*
  src/app/
    CMakeLists.txt
    main.cpp
    ui/DesktopLyricsWindow.*
    ui/LyricsHudWindow.*
    ui/TrayController.*
    ui/SearchLyricsDialog.*
    ui/PreferencesDialog.*
  tests/
    CMakeLists.txt
    test_lrc_parser.cpp
    test_quality_scorer.cpp
    test_lyrics_session_sync.cpp
    test_lyrics_filter.cpp
    fixtures/*.lrc
    fixtures/*.lrcx
  resources/
    icons/lyricsqt.svg
    lyricsqt.desktop.in
```

---

### Task 1: Repository skeleton and CMake

**Files:**
- Create: `CMakeLists.txt`
- Create: `src/liblyricsqt/CMakeLists.txt`
- Create: `src/app/CMakeLists.txt`
- Create: `tests/CMakeLists.txt`
- Create: `README.md`
- Create: `LICENSE`
- Create: `src/liblyricsqt/include/lyricsqt/Version.h`
- Create: `src/app/main.cpp`

- [ ] **Step 1: Write root CMakeLists.txt**

```cmake
cmake_minimum_required(VERSION 3.16)
project(LyricsQt VERSION 0.1.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets DBus Network Test)
enable_testing()
add_subdirectory(src/liblyricsqt)
add_subdirectory(src/app)
add_subdirectory(tests)
```

- [ ] **Step 2: Write liblyricsqt CMake with a tiny Version.h and empty static/shared lib target**

```cmake
add_library(lyricsqt
  include/lyricsqt/Version.h
)
add_library(LyricsQt::lyricsqt ALIAS lyricsqt)
target_include_directories(lyricsqt
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(lyricsqt PUBLIC Qt6::Core Qt6::DBus Qt6::Network)
```

```cpp
#pragma once
#define LYRICSQT_VERSION_STRING "0.1.0"
namespace lyricsqt {
inline const char* version() { return LYRICSQT_VERSION_STRING; }
}
```

- [ ] **Step 3: Write app main that links lyricsqt and shows a placeholder QLabel window**

```cpp
#include <QApplication>
#include <QLabel>
#include <lyricsqt/Version.h>

int main(int argc, char* argv[]) {
  QApplication app(argc, argv);
  QApplication::setOrganizationName(QStringLiteral("lyricsqt"));
  QApplication::setApplicationName(QStringLiteral("LyricsQt"));
  QLabel label(QStringLiteral("LyricsQt %1").arg(QLatin1String(lyricsqt::version())));
  label.resize(320, 80);
  label.show();
  return app.exec();
}
```

```cmake
add_executable(lyricsqt-app main.cpp)
set_target_properties(lyricsqt-app PROPERTIES OUTPUT_NAME lyricsqt)
target_link_libraries(lyricsqt-app PRIVATE LyricsQt::lyricsqt Qt6::Widgets)
```

- [ ] **Step 4: Configure and build**

Run:

```bash
cmake -S /home/zxn/Projects/playground/LyricsQt -B /home/zxn/Projects/playground/LyricsQt/build -DCMAKE_BUILD_TYPE=Debug
cmake --build /home/zxn/Projects/playground/LyricsQt/build -j
```

Expected: build succeeds; `./build/src/app/lyricsqt` launches a window.

- [ ] **Step 5: Commit**

```bash
cd /home/zxn/Projects/playground/LyricsQt
git add CMakeLists.txt LICENSE README.md src tests
git commit -m "chore: scaffold LyricsQt CMake project with lib and app"
```

---

### Task 2: AppSettings (QSettings wrapper)

**Files:**
- Create: `src/liblyricsqt/include/lyricsqt/AppSettings.h`
- Create: `src/liblyricsqt/settings/AppSettings.cpp`
- Modify: `src/liblyricsqt/CMakeLists.txt`

- [ ] **Step 1: Write failing test for default desktop lyrics enabled**

```cpp
#include <QtTest>
#include <lyricsqt/AppSettings.h>

class TestAppSettings : public QObject {
  Q_OBJECT
private slots:
  void defaults_desktopLyricsEnabled() {
    lyricsqt::AppSettings s;
    QVERIFY(s.desktopLyricsEnabled());
  }
};
QTEST_MAIN(TestAppSettings)
#include "test_app_settings.moc"
```

- [ ] **Step 2: Run test to verify it fails (header missing / link error)**

Run: `cmake --build build -j && ctest --test-dir build -R AppSettings -V`  
Expected: FAIL or compile error before implementation.

- [ ] **Step 3: Implement AppSettings**

Public API (minimum):

```cpp
class AppSettings : public QObject {
  Q_OBJECT
public:
  explicit AppSettings(QObject* parent = nullptr);
  bool desktopLyricsEnabled() const;
  void setDesktopLyricsEnabled(bool);
  bool menuBarLyricsEnabled() const; // tray line
  bool exportEnabled() const;
  QString preferredPlayerId() const;
  int globalOffsetMs() const;
  void setGlobalOffsetMs(int);
  // add keys as features land; keep names aligned with decision/spec
signals:
  void changed(const QString& key);
private:
  QSettings m_settings;
};
```

Defaults: desktop on, tray line off, export off, preferred player empty (auto), offset 0.

- [ ] **Step 4: Re-run test — PASS**

- [ ] **Step 5: Commit**

```bash
git commit -am "feat: add AppSettings backed by QSettings"
```

---

### Task 3: LRC/LRCX parser (TDD)

**Files:**
- Create: `src/liblyricsqt/include/lyricsqt/LyricsDocument.h`
- Create: `src/liblyricsqt/include/lyricsqt/LrcParser.h`
- Create: `src/liblyricsqt/lyrics/LyricsDocument.cpp`
- Create: `src/liblyricsqt/lyrics/LrcParser.cpp`
- Create: `tests/fixtures/basic.lrc`
- Create: `tests/test_lrc_parser.cpp`

- [ ] **Step 1: Add fixture `tests/fixtures/basic.lrc`**

```lrc
[ti:Test Song]
[ar:Test Artist]
[offset:0]
[00:01.00]First line
[00:05.50]Second line
[00:10.00]Third line
```

- [ ] **Step 2: Write failing parser tests**

```cpp
void parses_three_lines() {
  auto doc = lyricsqt::LrcParser::parseFile(QStringLiteral(":/fixtures/basic.lrc"));
  // or read from disk path relative to source
  QCOMPARE(doc.lines.size(), 3);
  QCOMPARE(doc.lines[0].content, QStringLiteral("First line"));
  QCOMPARE(doc.lines[0].positionSec, 1.0);
  QCOMPARE(doc.lines[1].positionSec, 5.5);
}
void lineIndex_at_time() {
  auto doc = lyricsqt::LrcParser::parseFile(...);
  QCOMPARE(doc.lineIndexAt(0.5, /*offsetMs*/0), -1);
  QCOMPARE(doc.lineIndexAt(1.0, 0), 0);
  QCOMPARE(doc.lineIndexAt(6.0, 0), 1);
}
```

- [ ] **Step 3: Run tests — FAIL**

- [ ] **Step 4: Implement LyricsDocument + LrcParser**

Requirements:

- Parse `[mm:ss.xx]` and `[mm:ss.xxx]`
- Parse metadata tags `ti/ar/al/offset`
- Support multiple timestamps on one text line (expand to multiple `LyricsLine`)
- LRCX: parse translation tags and word time tags into `WordTag` when present; unknown tags ignored safely
- `lineIndexAt(timeSec, offsetMs)` returns last line with `positionSec <= timeSec + offset`

- [ ] **Step 5: Run tests — PASS; commit**

```bash
git commit -am "feat: parse LRC/LRCX into LyricsDocument"
```

---

### Task 4: LyricsSession sync scheduler

**Files:**
- Create: `src/liblyricsqt/include/lyricsqt/LyricsSession.h`
- Create: `src/liblyricsqt/lyrics/LyricsSession.cpp`
- Create: `tests/test_lyrics_session_sync.cpp`

- [ ] **Step 1: Write test that advancing playback time updates currentLineIndex**

```cpp
void updates_line_when_time_advances() {
  lyricsqt::LyricsSession session;
  session.setLyrics(docFromFixture());
  session.setPlayback(true, /*positionSec*/1.0);
  QCOMPARE(session.currentLineIndex(), 0);
  session.setPlayback(true, 6.0);
  QCOMPARE(session.currentLineIndex(), 1);
}
```

- [ ] **Step 2: Implement LyricsSession**

- Holds `std::optional<LyricsDocument> m_lyrics`, `int m_currentLine = -1`
- `setPlayback(playing, positionSec)` recomputes index; if playing, start `QTimer` one-shot to next line boundary (mirror LyricsX schedule)
- Signals: `lyricsChanged()`, `currentLineChanged(int)`

- [ ] **Step 3: Tests PASS; commit**

```bash
git commit -am "feat: sync current lyric line to playback time"
```

---

### Task 5: MPRIS PlayerService

**Files:**
- Create: `src/liblyricsqt/include/lyricsqt/TrackInfo.h`
- Create: `src/liblyricsqt/include/lyricsqt/PlayerService.h`
- Create: `src/liblyricsqt/player/MprisPlayerBackend.h`
- Create: `src/liblyricsqt/player/MprisPlayerBackend.cpp`
- Create: `src/liblyricsqt/player/PlayerService.cpp`
- Modify: app `main.cpp` to log track title on change (temporary debug)

- [ ] **Step 1: Implement MprisPlayerBackend**

Behavior:

- List names under session bus matching `org.mpris.MediaPlayer2.*`
- For selected service, connect to `org.mpris.MediaPlayer2.Player` properties: `Metadata`, `PlaybackStatus`, `Position`
- Subscribe to `PropertiesChanged` and `Seeked`
- Map Metadata keys `xesam:title`, `xesam:artist`, `xesam:album`, `mpris:length`, `mpris:artUrl`, `xesam:url` → `TrackInfo`
- `seekTo(double sec)` calls `SetPosition` with track id object path when available, else `Seek`

- [ ] **Step 2: Implement PlayerService selection policy**

```text
if preferredPlayerId set and present → use it
else prefer PlaybackStatus == Playing
else first available
```

While playing, QTimer 250ms calls `updatePosition()`.

- [ ] **Step 3: Manual verification**

Run Spotify/VLC, start `lyricsqt`, confirm console/HUD logs title/artist/playing.

- [ ] **Step 4: Commit**

```bash
git commit -am "feat: read MPRIS track and playback state"
```

---

### Task 6: Wire track change → local LyricsStore

**Files:**
- Create: `src/liblyricsqt/include/lyricsqt/LyricsStore.h`
- Create: `src/liblyricsqt/store/LyricsStore.cpp`
- Modify: `LyricsSession` / small `AppController`-like `LyricsController` in lib or app

- [ ] **Step 1: LyricsStore API**

```cpp
class LyricsStore {
public:
  explicit LyricsStore(AppSettings*);
  std::optional<LyricsDocument> loadLocal(const TrackInfo&);
  QUrl save(const TrackInfo&, const LyricsDocument&);
  QString cacheFileName(const TrackInfo&) const; // "title - artist.lrcx"
};
```

Search order (spec):

1. Sidecar next to `fileUrl` if `loadLyricsBesideTrack`
2. User saving path / XDG data `lyricsqt/lyrics/`

- [ ] **Step 2: On `PlayerService::trackChanged`, clear session, `loadLocal`, if found `setLyrics`**

- [ ] **Step 3: Manual test with a local `.lrc` named for current track; commit**

```bash
git commit -am "feat: load local LRC/LRCX for current track"
```

---

### Task 7: Desktop lyrics window + tray basics

**Files:**
- Create: `src/app/ui/DesktopLyricsWindow.h/.cpp`
- Create: `src/app/ui/TrayController.h/.cpp`
- Modify: `main.cpp` to own `PlayerService`, `LyricsSession`, windows

- [ ] **Step 1: DesktopLyricsWindow**

- Frameless, tool, stays on top, translucent
- Shows current line (+ optional second line)
- Slot `onCurrentLineChanged`
- Mouse drag moves window; save position factors into AppSettings on release

- [ ] **Step 2: TrayController**

- Icon from resources
- Menu: Show HUD, Toggle desktop lyrics, Offset +100ms / -100ms, Preferences (stub), Quit
- If tray lyrics enabled, `setToolTip` / status text to current line (best-effort)

- [ ] **Step 3: Manual on KDE + GNOME; commit**

```bash
git commit -am "feat: desktop overlay and tray controls"
```

---

### Task 8: Lyrics HUD + seek

**Files:**
- Create: `src/app/ui/LyricsHudWindow.h/.cpp`

- [ ] **Step 1: QListView/QListWidget of lines; highlight current index**

- [ ] **Step 2: Double-click → `player.seekTo(line.positionSec)`**

- [ ] **Step 3: Accept drag-drop of `.lrc`/`.lrcx` → parse → `session.setLyrics` + store save**

- [ ] **Step 4: Commit**

```bash
git commit -am "feat: lyrics HUD with seek and drag-drop import"
```

---

### Task 9: Provider interface + LRCLIB

**Files:**
- Create: `providers/ILyricsProvider.h`
- Create: `providers/ProviderHub.*`
- Create: `providers/LrclibProvider.*`
- Create: `lyrics/QualityScorer.*`
- Create: `tests/test_quality_scorer.cpp`

- [ ] **Step 1: TDD QualityScorer with fixed TrackInfo + candidate docs**

- [ ] **Step 2: LrclibProvider HTTP GET `https://lrclib.net/api/get` / search API; parse synced lyrics into LyricsDocument**

- [ ] **Step 3: ProviderHub parallel search, 10s timeout, cancel on new track; auto-apply best if no local lyrics**

- [ ] **Step 4: Persist chosen lyrics via LyricsStore; commit**

```bash
git commit -am "feat: multi-provider hub with LRCLIB"
```

---

### Task 10: NetEase / QQ / Kugou providers

**Files:**
- Create: `providers/NetEaseProvider.*`
- Create: `providers/QQMusicProvider.*`
- Create: `providers/KugouProvider.*`

- [ ] **Step 1: Implement each adapter behind ILyricsProvider; map to LyricsDocument including translations when API provides them**

- [ ] **Step 2: Enable all four in ProviderHub default set; respect AppSettings enable/order**

- [ ] **Step 3: Manual search quality check on Chinese + English tracks; commit**

```bash
git commit -am "feat: add NetEase QQ Kugou lyrics providers"
```

---

### Task 11: Search dialog + wrong lyrics

**Files:**
- Create: `src/app/ui/SearchLyricsDialog.*`
- Extend: `AppSettings` ignore track ids / album names

- [ ] **Step 1: Dialog shows query fields prefilled from TrackInfo; results table Title/Artist/Source/Quality**

- [ ] **Step 2: Apply selection to session + store**

- [ ] **Step 3: “Wrong lyrics” action: add track id to no-search list, clear current lyrics**

- [ ] **Step 4: Commit**

```bash
git commit -am "feat: manual search UI and wrong-lyrics ignore list"
```

---

### Task 12: Filters + bilingual display

**Files:**
- Create: `lyrics/LyricsFilter.*`
- Modify: desktop + HUD rendering

- [ ] **Step 1: TDD filter keywords and smart filter (drop empty, pure punctuation, metadata-like lines)**

- [ ] **Step 2: Apply filter after load/search before display/persist flag**

- [ ] **Step 3: Prefer bilingual second line when translation present**

- [ ] **Step 4: Commit**

```bash
git commit -am "feat: lyric filters and bilingual lines"
```

Note: Chinese conversion (OpenCC) is explicitly out of v1 (decision D12).

---

### Task 13: Preferences dialog

**Files:**
- Create: `src/app/ui/PreferencesDialog.*` (+ optional `.ui` files)

- [ ] **Step 1: Tabs General / Display / Filter / Advanced wired to AppSettings (no Shortcuts tab)**

- [ ] **Step 2: Live-apply display colors/fonts to DesktopLyricsWindow**

- [ ] **Step 3: Commit**

```bash
git commit -am "feat: preferences dialog"
```

---

### Task 14: Autostart + quit with player (no global shortcuts)

**Files:**
- Create: `resources/lyricsqt.desktop.in`
- Modify: `TrayController` menus (offset ±, search, wrong lyrics, toggles — already primary UX)
- Modify: `PlayerService` / app controller for quit-with-player

- [ ] **Step 1: Ensure tray context menu covers: toggle desktop, toggle tray line, show HUD, offset ±, search, wrong lyrics, prefs, quit**

- [ ] **Step 2: Installable `.desktop` with optional autostart copy into `~/.config/autostart/` from settings**

- [ ] **Step 3: If quit-with-player enabled and designated MPRIS name leaves bus → `QCoreApplication::quit()`**

- [ ] **Step 4: Document in README: global hotkeys deferred; use tray menu**

- [ ] **Step 5: Commit**

```bash
git commit -am "feat: autostart and quit with player; tray menu as primary controls"
```

---

### Task 15: ExportServer (panel integration)

**Files:**
- Create: `src/liblyricsqt/export/ExportServer.*`
- Modify: `main.cpp` for `--pipe`
- Modify: settings + preferences Advanced tab

- [ ] **Step 1: On currentLineChanged, write UTF-8 line + newline to unix socket clients and/or stdout in pipe mode**

- [ ] **Step 2: Optional QDBus adaptor `org.lyricsqt.Export` method/property `CurrentLine`**

- [ ] **Step 3: Document Waybar/Plasma examples in README**

- [ ] **Step 4: Commit**

```bash
git commit -am "feat: export current lyric line for panels"
```

---

### Task 16: Polish, DE checklist, roadmap stubs

**Files:**
- Modify: `README.md`
- Create: `docs/superpowers/plans/wayland-de-checklist.md`
- Optional stub issue list for furigana / Windows SMTC

- [ ] **Step 1: Run DE checklist on GNOME Wayland and KDE Wayland; record results in checklist doc**

Checklist items:

- MPRIS detect Spotify/VLC/browser
- Desktop window visible and draggable
- Tray menu usable
- Export to Waybar custom module
- Search + offline cache after restart
- Offset shortcuts
- Quit with player

- [ ] **Step 2: README: build deps, run, settings paths, limitations on Wayland**

- [ ] **Step 3: Commit**

```bash
git commit -am "docs: README and Wayland DE verification checklist"
```

---

## Spec coverage check

| Spec section | Tasks |
|--------------|-------|
| lib + app architecture | 1 |
| Settings | 2, 13 |
| LRC/LRCX + sync | 3, 4 |
| MPRIS player | 5 |
| Local store | 6 |
| Desktop + tray | 7 |
| HUD + seek + DnD | 8 |
| Providers + quality | 9, 10 |
| Search + wrong lyrics | 11 |
| Filters + bilingual (no OpenCC) | 12 |
| Autostart/quit + tray menu (no hotkeys) | 14 |
| ExportServer | 15 |
| Risks/success/DE | 16 |
| Furigana / Windows | Deferred (decision log D16, D2) |

## Placeholder scan

No TBD steps; deferred items explicitly named in decision log and Task 16 roadmap stubs only as documentation, not as unimplemented required steps.

## Type consistency notes

- Use `lyricsqt::TrackInfo`, `LyricsDocument`, `LyricsSession`, `PlayerService`, `AppSettings` names consistently across tasks
- App executable target `lyricsqt-app` with output name `lyricsqt`
- Library target `lyricsqt` / alias `LyricsQt::lyricsqt`
