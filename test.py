import socket
import threading
import time


def send_resp_command(host: str, port: int, command: list[str], delay: float = 0.0):
    """Send a single RESP command and print the server response."""
    if delay:
        time.sleep(delay)

    # Encode command in RESP format
    resp = f"*{len(command)}\r\n"
    for arg in command:
        resp += f"${len(arg)}\r\n{arg}\r\n"

    try:
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            s.connect((host, port))
            s.sendall(resp.encode())

            # Read response (may block for XREAD BLOCK)
            while True:
                response = s.recv(4096)
                if not response:
                    break
                print(f"[{threading.current_thread().name}] Server response: {response.decode(errors='ignore')}")
    except Exception as e:
        print(f"[{threading.current_thread().name}] Error: {e}")


def run_multithreaded_tests():
    host, port = "127.0.0.1", 6379

    threads = []

    # Example: 1 blocking reader and 2 writers
    # Thread 1: Waits on XREAD BLOCK
    t1 = threading.Thread(
        target=send_resp_command,
        name="ReaderThread",
        args=(host, port, ["XREAD", "BLOCK", "10000", "STREAMS", "mystream", "1"]),
    )
    threads.append(t1)

    # Thread 2: Writes to stream after short delay
    t2 = threading.Thread(
        target=send_resp_command,
        name="Writer1",
        args=(host, port, ["XADD", "mystream", "1-*", "field1", "value1"],),
        kwargs={"delay": 2.0},  # delay so that XREAD blocks first
    )
    threads.append(t2)

    # Thread 3: Another writer
    t3 = threading.Thread(
        target=send_resp_command,
        name="Writer2",
        args=(host, port, ["XADD", "mystream", "1-*", "field2", "value2"],),
        kwargs={"delay": 4.0},
    )
    threads.append(t3)

    for t in threads:
        t.start()

    for t in threads:
        t.join()


if __name__ == "__main__":
    run_multithreaded_tests()
