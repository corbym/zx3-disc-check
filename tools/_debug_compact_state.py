#!/usr/bin/env python3
from pathlib import Path
import time
import sys

sys.path.insert(0, str(Path(__file__).resolve().parent))
import zesarux_smoketest as sm


def main() -> int:
    repo = Path(__file__).resolve().parents[1]
    tap = (repo / "out" / "disk_tester.tap").resolve()
    proc = sm.start_emulator(
        binary=sm.DEFAULT_EMULATOR,
        port=10044,
        machine="P340",
        emulator_speed=200,
        video_driver=None,
        zoom=None,
        headless=True,
    )
    client = None
    try:
        if not sm.wait_for_port("127.0.0.1", 10044, 20.0):
            raise RuntimeError("port did not open")
        client = sm.ZrcpClient("127.0.0.1", 10044, timeout=5.0)
        client.command("close-all-menus")
        client.command("hard-reset-cpu")
        time.sleep(0.2)
        client.command(f"smartload {tap}")
        time.sleep(6.0)

        print("OCR:")
        print(sm.clean_response(client.ocr()))
        for cmd in (
            "get-current-machine",
            "get-registers",
            "get-paging-state",
            "hexdump 5B67 8",
            "hexdump 32768 16",
            "hexdump 33024 16",
        ):
            print(f"\n$ {cmd}")
            print(sm.clean_response(client.command(cmd)))
        return 0
    finally:
        if client is not None:
            try:
                client.command("exit-emulator")
            except Exception:
                pass
        try:
            proc.terminate()
            proc.wait(timeout=5)
        except Exception:
            try:
                proc.kill()
            except Exception:
                pass


if __name__ == "__main__":
    raise SystemExit(main())
