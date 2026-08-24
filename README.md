# LyricsQt

Linux-first desktop lyrics client inspired by [LyricsX](https://github.com/ddddxxx/LyricsX), built with C++20 and Qt 6 (QWidget).

中文说明见下方 [中文](#中文)。

## Docs

- Decision log: `docs/superpowers/decisions/2026-08-20-decision-log.md`
- Design spec: `docs/superpowers/specs/2026-08-20-lyricsqt-design.md`
- Implementation plan: `docs/superpowers/plans/2026-08-20-lyricsqt-implementation.md`
- Wayland DE checklist: `docs/superpowers/plans/wayland-de-checklist.md`
- QQ Music Linux progress: `docs/qqmusic-linux-position.md`

## Features

- MPRIS (D-Bus) for now-playing
- Multi-source lyrics search (NetEase, QQ, Kugou, LRCLIB)
- Desktop overlay (karaoke wipe, resize, font size, lock) + tray + HUD
- QQ Music Linux progress via Electron CDP when MPRIS `Position` is stuck at 0
- Panel export (unix socket / `--pipe` / D-Bus)
- GNOME and KDE on Wayland as primary targets

## Build dependencies

Debian / Ubuntu (example packages):

```bash
sudo apt install build-essential cmake \
  qt6-base-dev qt6-base-dev-tools \
  qt6-tools-dev qt6-tools-dev-tools
```

Needed Qt 6 modules: Core, Gui, Widgets, DBus, Network, Test (pulled in via `qt6-base-dev` / tools). CMake ≥ 3.16 and a C++20 compiler (`g++` or `clang++`).

Optional runtime helpers for panel examples: `socat`, `qdbus` / `qt6-tools-dev-tools`, a MPRIS player (`playerctl` useful for debugging). QQ Music CDP bridge needs `python3` on `PATH`.

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

Global hotkeys are deferred. Use the **system tray** context menu for primary controls: toggle desktop lyrics, **lock desktop lyrics**, toggle tray line, show HUD, offset ±, search, wrong lyrics, preferences, and quit.

### Desktop lyrics

- **Unlocked**: hover to show centered `A-` / lock / `A+`; drag to move; drag left/right edges to resize width; scroll wheel or `A±` for font size; light gray background only while hovered.
- **Locked**: lyrics stay visible and are click-through (no panel chrome). Unlock from the tray menu item **Lock Desktop Lyrics** (uncheck).
- Karaoke yellow wipe follows the centered glyph width (not the full panel width).
- Font size is also in Preferences → Display (14–72 pt).

Optional: Preferences → Launch at login (XDG autostart) and Quit when preferred/active player exits.

### QQ Music (Linux)

Official Electron QQ Music often exposes Chromium MPRIS with `Position` stuck at `0`. LyricsQt can send `SIGUSR1` and read `<audio>.currentTime` through the Node inspector (`scripts/qqmusic_cdp_position.py`). See `docs/qqmusic-linux-position.md`. Override script path with `LYRICSQT_QQMUSIC_CDP_SCRIPT` if needed.

## Wayland limitations

- Desktop lyrics are a frameless always-on-top `QWidget`, not a compositor “desktop layer” like LyricsX on macOS. GNOME/KDE Wayland may ignore keep-above, restrict positioning, or clip translucency.
- On GNOME Wayland, the app may prefer XWayland (`xcb`) so the overlay can stay on top.
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

---

# 中文

LyricsQt 是面向 Linux 的桌面歌词客户端，灵感来自 [LyricsX](https://github.com/ddddxxx/LyricsX)，使用 C++20 + Qt 6（QWidget）实现。主要目标环境：GNOME / KDE（Wayland）。

## 文档

- 决策记录：`docs/superpowers/decisions/2026-08-20-decision-log.md`
- 设计说明：`docs/superpowers/specs/2026-08-20-lyricsqt-design.md`
- 实现计划：`docs/superpowers/plans/2026-08-20-lyricsqt-implementation.md`
- Wayland 桌面验证清单：`docs/superpowers/plans/wayland-de-checklist.md`
- QQ 音乐 Linux 进度获取：`docs/qqmusic-linux-position.md`

## 功能

- 通过 MPRIS（D-Bus）读取正在播放的曲目
- 多源搜词：网易云、QQ 音乐、酷狗、LRCLIB
- 桌面歌词（卡拉 OK 黄字进度、拖动改宽、字号、锁定）+ 托盘 + HUD
- Linux 版 QQ 音乐在 MPRIS `Position` 恒为 0 时，可用 Electron CDP 读取真实播放进度
- 面板导出：Unix socket / `--pipe` / D-Bus
- 本地歌词缓存、过滤、双语偏好、偏好播放器、开机自启等

## 依赖

Debian / Ubuntu 示例：

```bash
sudo apt install build-essential cmake \
  qt6-base-dev qt6-base-dev-tools \
  qt6-tools-dev qt6-tools-dev-tools
```

需要 Qt 6：Core、Gui、Widgets、DBus、Network、Test。CMake ≥ 3.16，C++20 编译器。  
面板示例可选：`socat`、`qdbus`、`playerctl`。QQ 音乐 CDP 桥接需要系统里有 `python3`。

## 编译与运行

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/src/app/lyricsqt
```

| 参数 | 作用 |
|------|------|
| `--pipe` | 同时把当前歌词行打印到标准输出（给 Waybar 等用） |

测试：

```bash
ctest --test-dir build --output-on-failure
```

## 配置与数据路径

| 内容 | 路径 |
|------|------|
| QSettings | `~/.config/lyricsqt/LyricsQt.conf` |
| 歌词缓存（默认） | `~/.local/share/lyricsqt/LyricsQt/`（可在偏好设置里改） |
| 开机自启 | `~/.config/autostart/lyricsqt.desktop` |
| 面板导出 socket | `$XDG_RUNTIME_DIR/lyricsqt.sock` |

## 操作说明

v1 不做全局热键，主要靠**托盘右键菜单**：开关桌面歌词、**锁定桌面歌词**、托盘行、HUD、偏移 ±100ms、搜索歌词、标记错误歌词、偏好设置、退出。

### 桌面歌词

- **解锁**：鼠标移入显示居中的 `A-` / 锁 / `A+`；拖动移动；左右边缘拖动改宽度；滚轮或 `A±` 改字号；仅悬停时显示淡灰底。
- **锁定**：歌词仍显示，但鼠标穿透（面板上不再出现控制条）。解锁请到托盘菜单取消勾选 **Lock Desktop Lyrics**。
- 卡拉 OK 黄条按**居中文字实际宽度**从左向右扫，而不是整块面板宽度。
- 字号也可在「偏好设置 → Display」里设置（14–72 pt）。

可选：开机自启；首选/当前播放器退出时一并退出 LyricsQt。

### QQ 音乐（Linux 客户端）

官方 Electron 版 QQ 音乐常用 Chromium 的 MPRIS，且 `Position` 常年卡在 `0`。LyricsQt 会对主进程发 `SIGUSR1`，通过 Node Inspector 读主窗口里 `<audio>.currentTime`（脚本：`scripts/qqmusic_cdp_position.py`）。细节见 `docs/qqmusic-linux-position.md`。可用环境变量 `LYRICSQT_QQMUSIC_CDP_SCRIPT` 指定脚本路径。

### Wayland 注意

- 桌面歌词是无边框置顶窗口，不是合成器「桌面层」；GNOME/KDE 可能限制置顶或透明。
- 在 GNOME Wayland 下，程序可能自动走 XWayland（`xcb`）以便置顶。
- 托盘依赖桌面环境；GNOME 上可能需要 AppIndicator / 托盘扩展。
- 无法直接画进 GNOME 顶栏或 Plasma 面板；可用桌面歌词、托盘行或 ExportServer 对接 Waybar / 小部件 / 扩展。

### 面板导出

在「偏好设置 → Advanced」启用导出服务，或使用 `--pipe`。

| 通道 | 说明 |
|------|------|
| Unix socket | `$XDG_RUNTIME_DIR/lyricsqt.sock`，每行当前歌词 UTF-8 + `\n` |
| 标准输出 | `lyricsqt --pipe` |
| D-Bus | `org.lyricsqt.Export`，路径 `/org/lyricsqt/Export` |

属性：`CurrentLine`、`Title`、`Artist`、`Playing`。

## 许可证

MIT
