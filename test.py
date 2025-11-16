import socket
import threading
import time


def send_resp_command(host: str, port: int, command: list[str], delay: float = 0.0):
    """Send a single RESP command and exit once a response is received."""
    if delay:
        time.sleep(delay)

    # Encode command in RESP format
    resp = f"*{len(command)}\r\n"
    for arg in command:
        resp += f"${len(arg)}\r\n{arg}\r\n"

    try:
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            s.settimeout(15)  # Prevent infinite blocking if server never responds
            s.connect((host, port))
            s.sendall(resp.encode())

            # Wait for the first full response (blocking read)
            response = b""
            try:
                chunk = s.recv(4096)
                response += chunk
            except socket.timeout:
                print(f"[{threading.current_thread().name}] Timeout waiting for response")
                return

            if response:
                print(f"[{threading.current_thread().name}] Server response:\n{response.decode(errors='ignore')}")
            else:
                print(f"[{threading.current_thread().name}] No response received")

    except Exception as e:
        print(f"[{threading.current_thread().name}] Error: {e}")


def run_multithreaded_tests():
    host, port = "127.0.0.1", 6379
    threads = []

    # Thread 1: XREAD BLOCK waits for messages
    t1 = threading.Thread(
        target=send_resp_command,
        name="ReaderThread",
        args=(host, port, ["XREAD", "BLOCK", "10000", "STREAMS", "mystream11", "1"]),
    )
    threads.append(t1)

    # Thread 2: XADD after 2 seconds
    t2 = threading.Thread(
        target=send_resp_command,
        name="Writer1",
        args=(host, port, ["XADD", "mystream11", "1-*", "field1", "value1"]),
        kwargs={"delay": 2.0},
    )
    threads.append(t2)

    # Thread 3: Another XADD after 4 seconds
    t3 = threading.Thread(
        target=send_resp_command,
        name="Writer2",
        args=(host, port, ["XADD", "mystream11", "1-*", "field2", "value2"]),
        kwargs={"delay": 8.0},
    )
    threads.append(t3)

    for t in threads:
        t.start()

    for t in threads:
        t.join()


if __name__ == "__main__":
    run_multithreaded_tests()
