# QQ Music Linux：播放进度获取

官方 Linux QQ 音乐（Electron）通过 Chromium MPRIS 暴露元数据，但 `Position` 恒为 `0`，`CanSeek/CanControl` 为 false。桌面歌词窗口「歌词」只消费内部状态，不对外提供进度 IPC。

## 可行方案（已验证）

对主进程发送 `SIGUSR1` 打开 Node Inspector（`127.0.0.1:9229`），经 CDP 在主窗口执行：

```js
document.querySelectorAll('audio')… → currentTime / duration / paused
```

实测 `currentTime` 随播放约每秒递增。

辅助脚本：`scripts/qqmusic_cdp_position.py`  
LyricsQt 在激活 `org.mpris.MediaPlayer2.chromium.*` 时自动拉起该脚本，用 CDP 时钟覆盖卡住的 MPRIS Position。

环境变量 `LYRICSQT_QQMUSIC_CDP_SCRIPT` 可指定脚本路径。

## 已尝试且不可用 / 不充分

| 路线 | 结果 |
|------|------|
| MPRIS Position | 恒 0 |
| asar 明文翻 IPC | `.js` 为自定义加密（魔数 `e7a3d286`），asarfix 未解出 |
| 未公开 unix socket | 仅 Chromium singleton socket |
| PulseAudio | 只能判断在播，无进度 |
| 桌面歌词窗口 DOM | 无 `<audio>`，只有歌词文本 |

## 注意

Inspector 仅本机；`SIGUSR1` 由脚本在需要时发送。勿把 `~/.config/qqmusic/settings.json` 里的登录 token 写入日志或提交仓库。
