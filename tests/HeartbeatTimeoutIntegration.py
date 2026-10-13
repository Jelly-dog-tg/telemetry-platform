
import csv
import os
import socket
import subprocess
import sys
import tempfile
import time
from pathlib import Path

HOST = "127.0.0.1"
PORT = 5000
DEVICE_ID = 9001


def request(message):
    with socket.create_connection((HOST, PORT), timeout=3) as sock:
        sock.settimeout(3)
        sock.sendall(message.encode("utf-8"))

        data = bytearray()
        while b"\n" not in data:
            chunk = sock.recv(1024)
            if not chunk:
                break
            data.extend(chunk)

        return data.decode(errors="replace").strip()


def read_device(csv_path):
    with csv_path.open(newline="", encoding="utf-8") as file:
        for row in csv.DictReader(file):
            if int(row["id"]) == DEVICE_ID:
                return row
    return None


def main():
    if len(sys.argv) != 2:
        print("Usage: python3 HeartbeatTimeoutIntegration.py SERVER_PATH")
        return 1

    server_path = os.path.abspath(sys.argv[1])

    with socket.socket() as probe:
        if probe.connect_ex((HOST, PORT)) == 0:
            print("FAIL: Port 5000 is already occupied.")
            return 1

    with tempfile.TemporaryDirectory() as temp_dir:
        temp = Path(temp_dir)
        csv_path = temp / "devices.csv"
        log_path = temp / "server.log"

        # Use the legacy CSV format already covered by
        # the existing TCP integration test.
        with csv_path.open("w", newline="", encoding="utf-8") as file:
            writer = csv.writer(file)
            writer.writerow(["id", "temperature", "online"])
            writer.writerow([1, 20.0, 1])

        with log_path.open("w+", encoding="utf-8") as log:
            server = subprocess.Popen(
                [server_path],
                cwd=temp_dir,
                stdout=log,
                stderr=subprocess.STDOUT,
            )

            try:
                ready = False

                for _ in range(50):
                    if server.poll() is not None:
                        break

                    try:
                        with socket.create_connection(
                            (HOST, PORT), timeout=0.2
                        ):
                            ready = True
                            break
                    except OSError:
                        time.sleep(0.1)

                if not ready:
                    print("FAIL: Server did not start.")
                    return 1

                # 1. Register a new online device.
                response = request(f"REGISTER,{DEVICE_ID}\n")

                if response != "ACK: registered":
                    print("FAIL: Device registration:", response)
                    return 1

                row = read_device(csv_path)

                if row is None or int(row["online"]) != 1:
                    print("FAIL: Registered device is not online.")
                    return 1

                print("[PASS] Device registered and online")

                # 2. Wait for 30s timeout + monitor interval.
                deadline = time.monotonic() + 45
                offline = False

                while time.monotonic() < deadline:
                    time.sleep(1)
                    row = read_device(csv_path)

                    if row is not None and int(row["online"]) == 0:
                        offline = True
                        break

                if not offline:
                    print("FAIL: Device did not become offline.")
                    return 1

                print("[PASS] Heartbeat timeout marked device offline")
                print("[PASS] Offline state persisted to CSV")

                # 3. A fresh heartbeat restores online state.
                response = request(
                    f"HEARTBEAT,{DEVICE_ID},1\n"
                )

                if response != "ACK: heartbeat":
                    print("FAIL: Heartbeat recovery:", response)
                    return 1

                row = read_device(csv_path)

                if row is None or int(row["online"]) != 1:
                    print("FAIL: Heartbeat did not restore online state.")
                    return 1

                print("[PASS] Heartbeat restored device online")
                print("\nAll heartbeat timeout tests passed.")
                return 0

            finally:
                server.terminate()

                try:
                    server.wait(timeout=3)
                except subprocess.TimeoutExpired:
                    server.kill()
                    server.wait()

                if server.returncode not in (0, -15):
                    print("Server log:")
                    print(log_path.read_text(
                        encoding="utf-8", errors="replace"
                    ))


if __name__ == "__main__":
    raise SystemExit(main())
