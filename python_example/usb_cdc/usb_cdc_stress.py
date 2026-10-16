"""Read-only command stress test. Requires pyserial; continuously drains replies."""
import argparse
import threading
import time

import serial


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('port', help='USB CDC port, for example COM15')
    parser.add_argument('--seconds', type=float, default=300)
    parser.add_argument('--rate', type=float, default=2000, help='commands per second')
    args = parser.parse_args()
    if args.seconds <= 0 or args.rate <= 0:
        parser.error('--seconds and --rate must be positive')

    # T=603 only queries output settings; it does not move bus devices.
    payload = b'{"T":603}\n'
    stopped = threading.Event()
    errors = []
    received = [0]
    with serial.Serial(args.port, 1000000, timeout=0.1, write_timeout=2) as port:
        def reader():
            try:
                while not stopped.is_set():
                    received[0] += len(port.read(4096))
            except serial.SerialException as exc:
                errors.append(exc)
                stopped.set()

        worker = threading.Thread(target=reader, daemon=True)
        worker.start()
        sent = 0
        started = time.monotonic()
        next_report = started + 5
        try:
            while time.monotonic() - started < args.seconds and not stopped.is_set():
                # Batch writes reduce host overhead. No per-command ACK is expected.
                batch = min(32, max(1, int(args.rate / 100)))
                data = payload * batch
                if port.write(data) != len(data):
                    raise RuntimeError('Incomplete USB write')
                sent += batch
                delay = started + sent / args.rate - time.monotonic()
                if delay > 0:
                    stopped.wait(delay)
                if time.monotonic() >= next_report:
                    print(f'sent={sent}, received_bytes={received[0]}', flush=True)
                    next_report = time.monotonic() + 5
            if errors:
                raise errors[0]
        finally:
            stopped.set()
            worker.join(timeout=3)
        print(f'Finished: sent={sent}, received_bytes={received[0]}')
        print('This measures transport activity, not the number of commands executed.')


if __name__ == '__main__':
    main()
