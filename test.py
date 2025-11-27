import socket
import threading
import time


def encode_resp_command(command: list[str]) -> bytes:
    """Encode a command list into RESP format."""
    resp = f"*{len(command)}\r\n"
    for arg in command:
        resp += f"${len(arg)}\r\n{arg}\r\n"
    return resp.encode()


def send_multiple_commands(host: str, port: int, commands: list[list[str]], delay_between: float = 0.0):
    """Send multiple commands over one persistent connection."""
    try:
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            s.settimeout(10)
            s.connect((host, port))
            print(f"[{threading.current_thread().name}] Connected to Redis")

            for cmd in commands:
                # Encode and send
                resp = encode_resp_command(cmd)
                s.sendall(resp)
                print(f"[{threading.current_thread().name}] Sent: {' '.join(cmd)}")

                # Wait for response (may block if command is blocking)
                try:
                    response = s.recv(4096)
                    if not response:
                        print(f"[{threading.current_thread().name}] Connection closed by server")
                        break
                    print(f"[{threading.current_thread().name}] Response:\n{response.decode(errors='ignore')}")
                except socket.timeout:
                    print(f"[{threading.current_thread().name}] Timeout waiting for response to {' '.join(cmd)}")

                if delay_between:
                    time.sleep(delay_between)

    except Exception as e:
        print(f"[{threading.current_thread().name}] Error: {e}")


def run_multithreaded_tests():
    host, port = "127.0.0.1", 6379
    threads = []

    # Thread 1: Persistent client that does XREAD BLOCK + XREAD again
    t1_cmds = [
        # ["XREAD", "BLOCK", "10000", "STREAMS", "mystream", "$"],
        # ["PING"],
        # ["ECHO", "done"],
        # ["SUBSCRIBE", "mychan1"],

    ]
    t1 = threading.Thread(
        target=send_multiple_commands,
        name="ReaderClient",
        args=(host, port, t1_cmds),
        kwargs={"delay_between": 2.0},
    )

    # Thread 2: Another client that writes data periodically
    t2_cmds = [
        # ["SUBSCRIBE", "mychan1"],

        # ["XADD", "mystream", "*", "field1", "value1"],
        # ["XADD", "mystream", "*", "field2", "value2"],
        # ["XADD", "mystream", "*", "field3", "value3"]
    ]
    t2 = threading.Thread(
        target=send_multiple_commands,
        name="WriterClient",
        args=(host, port, t2_cmds),
        kwargs={"delay_between": 2.0},
    )

    # Thread 3: Simple client that sends a mix of normal commands
    t3_cmds = [
        # ["SUBSCRIBE", "mychan1"],
        # ["SUBSCRIBE", "mychan2"],
        # ["SET", "foo", "bar", ],

        # ["SUBSCRIBE", "mychan2"],
        # ["SUBSCRIBE", "mychan1"],

        ["ACL", "WHOAMI"],
        ["ACL", "SETUSER", "default", ">mypassword"],
        ["ACL", "GETUSER", "default"],
        ["AUTH", "default", "asdasdsad"],
        ["AUTH", "default", "mypassword"],
        # ["PUBLISH", "mychan2"],
        # ["SET", "num", "12345", "PX", "12343"],
        # # ["CONFIG", "GET", "dir"],
        # # ["CONFIG", "GET", "dbfilename"],
        # # ["REPLCONF", "GETACK", "*"],
        # ["INCR", "counter"],
        # ["INCR", "counter"],
        # ["SAVE"],
        # ["KEYS", "*"],
        # ["GET", "num"],
        # ["GET", "counter"],
        # # ["INCR", "foo"],
        # ["GET", "foo"],
        # ["PING"],
        # ["REPLCONF", "GETACK", "*"],
        # ["ECHO", "test"]

    ]
    t3 = threading.Thread(
        target=send_multiple_commands,
        name="BasicClient",
        args=(host, port, t3_cmds),
        kwargs={"delay_between": 1.0},
    )
    # threads.append(t1)
    # threads.append(t2)
    threads.append(t3)

    for t in threads:
        t.start()

    for t in threads:
        t.join()


if __name__ == "__main__":
    run_multithreaded_tests()
