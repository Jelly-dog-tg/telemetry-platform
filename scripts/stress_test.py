
import argparse
import secrets
import socket
import statistics
import time
from concurrent.futures import ThreadPoolExecutor

HOST = "127.0.0.1"
PORT = 5000


def run_device(device_id, duration, interval):
    """Simulate one device using a persistent TCP connection."""
    latencies = []
    success = 0
    failed = 0

    try:
        with socket.create_connection((HOST, PORT), timeout=5) as sock:
            sock.settimeout(5)

            with sock.makefile("rb") as reader:

                def request(message):
                    start = time.perf_counter()
                    sock.sendall(message.encode("utf-8"))
                    response = reader.readline()

                    latency = (
                        time.perf_counter() - start
                    ) * 1000

                    if not response:
                        raise ConnectionError("Server disconnected")

                    return response.decode().strip(), latency

                response, _ = request(
                    f"REGISTER,{device_id}\n"
                )

                if response not in (
                    "ACK: registered",
                    "ACK: already registered",
                ):
                    raise RuntimeError(response)

                sequence = 1
                end_time = time.monotonic() + duration
                next_send = time.monotonic()

                while time.monotonic() < end_time:
                    temperature = 35.0 + (sequence % 10)

                    message = (
                        f"TELEMETRY,{device_id},"
                        f"{temperature},1,{sequence}\n"
                    )

                    try:
                        response, latency = request(message)

                        if response == "ACK from telemetry server":
                            success += 1
                            latencies.append(latency)
                        else:
                            failed += 1
                            print(f"Device {device_id}: {response}")

                    except (OSError, ConnectionError) as error:
                        failed += 1
                        print(f"Device {device_id}: {error}")
                        break

                    sequence += 1
                    next_send += interval

                    sleep_time = next_send - time.monotonic()
                    if sleep_time > 0:
                        time.sleep(sleep_time)

    except (OSError, RuntimeError) as error:
        failed += 1
        print(f"Device {device_id} failed: {error}")

    return success, failed, latencies


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--clients", type=int, default=8)
    parser.add_argument("--duration", type=int, default=10)
    parser.add_argument("--rate", type=float, default=5)
    args = parser.parse_args()

    if args.clients <= 0 or args.duration <= 0 or args.rate <= 0:
        parser.error("clients, duration and rate must be positive")

    interval = 1.0 / args.rate

    # Random device IDs reduce conflicts with earlier tests.
    base_id = 100000 + secrets.randbelow(800000)

    print("=== Telemetry Stress Test ===")
    print(f"Clients: {args.clients}")
    print(f"Duration per client: {args.duration}s")
    print(f"Target rate per client: {args.rate} msg/s")

    start = time.perf_counter()

    with ThreadPoolExecutor(max_workers=args.clients) as executor:
        futures = [
            executor.submit(
                run_device,
                base_id + i,
                args.duration,
                interval,
            )
            for i in range(args.clients)
        ]

        results = [future.result() for future in futures]

    elapsed = time.perf_counter() - start

    success = sum(result[0] for result in results)
    failed = sum(result[1] for result in results)
    latencies = [
        value
        for result in results
        for value in result[2]
    ]

    print("\n=== Results ===")
    print(f"Elapsed: {elapsed:.2f}s")
    print(f"Successful telemetry: {success}")
    print(f"Failed operations: {failed}")
    print(f"Successful throughput: {success / elapsed:.2f} msg/s")

    if latencies:
        values = sorted(latencies)
        p95_index = max(0, int(0.95 * len(values) + 0.999999) - 1)

        print(f"Average latency: {statistics.mean(values):.2f} ms")
        print(f"P95 latency: {values[p95_index]:.2f} ms")

    total = success + failed
    if total:
        print(f"Observed operation success rate: {success / total * 100:.2f}%")

    return 0 if failed == 0 and success > 0 else 1


if __name__ == "__main__":
    raise SystemExit(main())
