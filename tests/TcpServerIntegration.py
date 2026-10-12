import csv
import os
import socket
import subprocess
import sys
import tempfile
import time
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

HOST = "127.0.0.1"
PORT = 5000


def send_message(message):
    """Send one TCP message and receive one response."""
    with socket.create_connection(
        (HOST, PORT), timeout=3
    ) as client:
        client.settimeout(3)
        client.sendall(message.encode("utf-8"))

        response = bytearray()

        while b"\n" not in response:
            chunk = client.recv(1024)
            if not chunk:
                break
            response.extend(chunk)

        return response.decode(
            "utf-8", errors="replace"
        ).strip()


def main():
    if len(sys.argv) != 2:
        print("Usage: TcpServerIntegration.py SERVER_PATH")
        return 1

    server_path = os.path.abspath(sys.argv[1])

    # Avoid accidentally testing another running server.
    with socket.socket() as probe:
        if probe.connect_ex((HOST, PORT)) == 0:
            print(
                "FAIL: Port 5000 is already in use. "
                "Stop the existing server first."
            )
            return 1

    with tempfile.TemporaryDirectory() as temp_dir:
        temp_path = Path(temp_dir)
        csv_path = temp_path / "devices.csv"
        log_path = temp_path / "server.log"

        # Create an OLD three-column CSV file.
        with csv_path.open(
            "w", newline="", encoding="utf-8"
        ) as file:
            writer = csv.writer(file)
            writer.writerow(["id", "temperature", "online"])

            for device_id in range(1, 9):
                writer.writerow([device_id, 20.0, 1])

        with log_path.open("w+", encoding="utf-8") as log:
            server = subprocess.Popen(
                [server_path],
                cwd=temp_dir,
                stdout=log,
                stderr=subprocess.STDOUT,
            )

            try:
                # Wait until the server is listening.
                deadline = time.monotonic() + 5
                ready = False

                while time.monotonic() < deadline:
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
                    log.flush()
                    print("FAIL: Server did not start.")
                    print(log_path.read_text(encoding="utf-8"))
                    return 1

                # Send eight messages concurrently.
                messages = [
                    (
                        device_id,
                        30.0 + device_id,
                        1,
                        1,
                    )
                    for device_id in range(1, 9)
                ]

                def build_and_send(item):
                    device_id, temperature, online, sequence = item
                    message = (
                        f"TELEMETRY,{device_id},"
                        f"{temperature:.1f},{online},"
                        f"{sequence}\n"
                    )
                    return send_message(message)

                with ThreadPoolExecutor(max_workers=8) as pool:
                    responses = list(
                        pool.map(build_and_send, messages)
                    )

                if not all(
                    response == "ACK from telemetry server"
                    for response in responses
                ):
                    print("FAIL: Concurrent requests failed.")
                    print(responses)
                    return 1

                print("[PASS] Eight concurrent clients received ACK")

                # Repeat device 1's sequence number.
                stale_response = send_message(
                    "TELEMETRY,1,99.9,1,1\n"
                )

                if not stale_response.startswith("ERROR:"):
                    print("FAIL: Duplicate sequence was accepted.")
                    print(stale_response)
                    return 1

                print("[PASS] Duplicate sequence was rejected")

                # Send an invalid message.
                invalid_response = send_message(
                    "NOT_TELEMETRY\n"
                )

                if not invalid_response.startswith("ERROR:"):
                    print("FAIL: Invalid message was accepted.")
                    print(invalid_response)
                    return 1

                print("[PASS] Invalid protocol message was rejected")

                # Verify data persisted and CSV was upgraded.
                with csv_path.open(
                    newline="", encoding="utf-8"
                ) as file:
                    reader = csv.DictReader(file)
                    rows = {
                        int(row["id"]): row
                        for row in reader
                    }

                if "sequence" not in rows[1]:
                    print("FAIL: CSV sequence column is missing.")
                    return 1

                for device_id, temperature, online, sequence in messages:
                    row = rows[device_id]

                    if (
                        abs(float(row["temperature"]) - temperature) > 1e-9
                        or int(row["online"]) != online
                        or int(row["sequence"]) != sequence
                    ):
                        print(
                            f"FAIL: Incorrect saved data for "
                            f"device {device_id}: {row}"
                        )
                        return 1

                # The duplicate message must not overwrite device 1.
                if float(rows[1]["temperature"]) != 31.0:
                    print("FAIL: Duplicate message changed device 1.")
                    return 1

                print("[PASS] Old CSV loaded and upgraded to four columns")
                print("[PASS] Concurrent device updates were saved")
                print("[PASS] Rejected messages did not corrupt device data")
                print("\nAll TCP integration tests passed.")
                return 0

            finally:
                server.terminate()

                try:
                    server.wait(timeout=3)
                except subprocess.TimeoutExpired:
                    server.kill()
                    server.wait()


if __name__ == "__main__":
    raise SystemExit(main())
