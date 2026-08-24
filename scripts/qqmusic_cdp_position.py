#!/usr/bin/env python3
"""Poll Linux QQ Music (Electron) playback position via Node inspector CDP.

QQ Music's MPRIS Position is stuck at 0. After SIGUSR1, the main process
exposes an inspector on 127.0.0.1:9229. We executeJavaScript in the main
BrowserWindow to read the playing <audio>.currentTime.

Prints one JSON object per line to stdout.
"""

from __future__ import annotations

import base64
import hashlib
import json
import os
import signal
import socket
import struct
import sys
import time
import urllib.error
import urllib.request
from typing import Any, Optional

INSPECT_HOST = "127.0.0.1"
INSPECT_PORT = 9229
POLL_SEC = 0.25


def find_qqmusic_pid() -> Optional[int]:
    for name in os.listdir("/proc"):
        if not name.isdigit():
            continue
        pid = int(name)
        try:
            cmdline = open(f"/proc/{pid}/cmdline", "rb").read().split(b"\0")
        except OSError:
            continue
        if not cmdline or not cmdline[0]:
            continue
        exe = cmdline[0].decode("utf-8", "replace")
        # Main process only (no --type=renderer/gpu/...)
        joined = b" ".join(cmdline).decode("utf-8", "replace")
        if "qqmusic" not in exe and "/opt/qqmusic/qqmusic" not in joined:
            continue
        if "--type=" in joined:
            continue
        if exe.endswith("qqmusic") or "/opt/qqmusic/qqmusic" in exe:
            return pid
    return None


def ensure_inspector(pid: int) -> None:
    try:
        urllib.request.urlopen(f"http://{INSPECT_HOST}:{INSPECT_PORT}/json/version", timeout=0.5)
        return
    except Exception:
        pass
    os.kill(pid, signal.SIGUSR1)
    for _ in range(40):
        time.sleep(0.1)
        try:
            urllib.request.urlopen(f"http://{INSPECT_HOST}:{INSPECT_PORT}/json/version", timeout=0.5)
            return
        except Exception:
            continue
    raise RuntimeError("failed to open QQ Music inspector on :9229 after SIGUSR1")


def ws_connect(url: str) -> socket.socket:
    # url: ws://127.0.0.1:9229/<id>
    assert url.startswith("ws://")
    hostport, path = url[5:].split("/", 1)
    path = "/" + path
    host, port_s = hostport.split(":")
    port = int(port_s)
    sock = socket.create_connection((host, port), timeout=10)
    key = base64.b64encode(os.urandom(16)).decode("ascii")
    req = (
        f"GET {path} HTTP/1.1\r\n"
        f"Host: {host}:{port}\r\n"
        f"Upgrade: websocket\r\n"
        f"Connection: Upgrade\r\n"
        f"Sec-WebSocket-Key: {key}\r\n"
        f"Sec-WebSocket-Version: 13\r\n"
        f"\r\n"
    )
    sock.sendall(req.encode("ascii"))
    # read HTTP response headers
    data = b""
    while b"\r\n\r\n" not in data:
        chunk = sock.recv(4096)
        if not chunk:
            raise RuntimeError("websocket handshake closed")
        data += chunk
    header, _ = data.split(b"\r\n\r\n", 1)
    if b"101" not in header.split(b"\r\n", 1)[0]:
        raise RuntimeError(f"websocket handshake failed: {header[:200]!r}")
    return sock


def ws_send_text(sock: socket.socket, text: str) -> None:
    payload = text.encode("utf-8")
    mask = os.urandom(4)
    header = bytearray([0x81])  # text, fin
    n = len(payload)
    if n < 126:
        header.append(0x80 | n)
    elif n < 65536:
        header.append(0x80 | 126)
        header.extend(struct.pack("!H", n))
    else:
        header.append(0x80 | 127)
        header.extend(struct.pack("!Q", n))
    header.extend(mask)
    masked = bytes(b ^ mask[i % 4] for i, b in enumerate(payload))
    sock.sendall(header + masked)


def ws_recv_message(sock: socket.socket) -> str:
    # simplified: assume server sends unmasked text frames (CDP does)
    def read_exact(n: int) -> bytes:
        buf = b""
        while len(buf) < n:
            chunk = sock.recv(n - len(buf))
            if not chunk:
                raise RuntimeError("socket closed")
            buf += chunk
        return buf

    while True:
        b0, b1 = read_exact(2)
        opcode = b0 & 0x0F
        masked = (b1 & 0x80) != 0
        length = b1 & 0x7F
        if length == 126:
            length = struct.unpack("!H", read_exact(2))[0]
        elif length == 127:
            length = struct.unpack("!Q", read_exact(8))[0]
        mask = read_exact(4) if masked else b""
        payload = read_exact(length)
        if masked:
            payload = bytes(b ^ mask[i % 4] for i, b in enumerate(payload))
        if opcode == 0x1:  # text
            return payload.decode("utf-8")
        if opcode == 0x8:  # close
            raise RuntimeError("websocket closed by peer")
        if opcode == 0x9:  # ping -> pong
            # ignore for now
            continue
        # skip other opcodes


class CdpClient:
    def __init__(self, sock: socket.socket):
        self.sock = sock
        self._id = 0

    def call(self, method: str, params: Optional[dict] = None) -> Any:
        self._id += 1
        mid = self._id
        msg = {"id": mid, "method": method}
        if params is not None:
            msg["params"] = params
        ws_send_text(self.sock, json.dumps(msg))
        while True:
            data = json.loads(ws_recv_message(self.sock))
            if data.get("id") == mid:
                if "error" in data:
                    raise RuntimeError(data["error"])
                return data.get("result")


PROBE_JS = r"""
(async () => {
  const require = process.mainModule.require;
  const { BrowserWindow } = require('electron');
  const wins = BrowserWindow.getAllWindows();
  const main = wins.find(w => (w.webContents.getURL() || '').includes('index.html'))
            || wins.find(w => w.id === (global.mainWinId || 1))
            || wins[0];
  if (!main) return { ok: false, error: 'no-window' };
  const probe = await main.webContents.executeJavaScript(`(() => {
    const list = [...document.querySelectorAll('audio')];
    const playing = list.find(a => a.src && !a.paused)
                 || list.find(a => a.currentTime > 0 && a.src);
    if (!playing) return { ok: false, error: 'no-audio' };
    return {
      ok: true,
      currentTime: playing.currentTime,
      duration: playing.duration || 0,
      paused: playing.paused
    };
  })()`, true);
  return probe;
})()
"""


def fetch_position(cdp: CdpClient) -> dict:
    result = cdp.call(
        "Runtime.evaluate",
        {"expression": PROBE_JS, "awaitPromise": True, "returnByValue": True},
    )
    value = (result or {}).get("result", {}).get("value")
    if not isinstance(value, dict):
        return {"ok": False, "error": "bad-result", "raw": result}
    return value


def main() -> int:
    pid = find_qqmusic_pid()
    if not pid:
        print(json.dumps({"ok": False, "error": "qqmusic-not-running"}), flush=True)
        return 1
    ensure_inspector(pid)
    targets = json.load(
        urllib.request.urlopen(f"http://{INSPECT_HOST}:{INSPECT_PORT}/json/list", timeout=2)
    )
    if not targets:
        print(json.dumps({"ok": False, "error": "no-inspect-targets"}), flush=True)
        return 1
    ws_url = targets[0]["webSocketDebuggerUrl"]
    sock = ws_connect(ws_url)
    cdp = CdpClient(sock)
    cdp.call("Runtime.enable")
    print(json.dumps({"ok": True, "event": "connected", "pid": pid}), flush=True)
    try:
        while True:
            try:
                pos = fetch_position(cdp)
                pos["pid"] = pid
                pos["ts"] = time.time()
                print(json.dumps(pos, ensure_ascii=False), flush=True)
            except Exception as exc:
                print(json.dumps({"ok": False, "error": str(exc)}), flush=True)
                return 2
            time.sleep(POLL_SEC)
    finally:
        sock.close()
    return 0


if __name__ == "__main__":
    sys.exit(main())
