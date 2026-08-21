# LyricsQt

Linux-first desktop lyrics client inspired by [LyricsX](https://github.com/ddddxxx/LyricsX), built with C++20 and Qt 6 (QWidget).

Status: scaffolding in progress. Docs and Task 1 CMake skeleton are in tree.

## Docs

- Decision log: `docs/superpowers/decisions/2026-08-20-decision-log.md`
- Design spec: `docs/superpowers/specs/2026-08-20-lyricsqt-design.md`
- Implementation plan: `docs/superpowers/plans/2026-08-20-lyricsqt-implementation.md`

## Planned stack

- MPRIS (D-Bus) for now-playing
- Multi-source lyrics search (NetEase, QQ, Kugou, LRCLIB)
- Desktop overlay + tray + panel export
- GNOME and KDE on Wayland as primary targets

## Build (after scaffold lands)

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/src/app/lyricsqt
```
