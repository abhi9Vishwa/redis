import socket
import threading
import time

HOST = "127.0.0.1"
PORT = 6379
CHANNEL = "mychan1"


def send_resp_array(sock, arr):
    out = f"*{len(arr)}\r\n"
    for item in arr:
        out += f"${len(item)}\r\n{item}\r\n"
    sock.sendall(out.encode())


def subscriber_worker(name, quit_after=None):
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.connect((HOST, PORT))

    send_resp_array(sock, ["SUBSCRIBE", CHANNEL])
    print(f"[{name}] SUBSCRIBED to {CHANNEL}")

    buf = b""
    msg_count = 0

    while True:
        chunk = sock.recv(4096)
        if not chunk:
            print(f"[{name}] Connection closed by server")
            sock.close()
            return
            break

        buf += chunk

        while b"\r\n" in buf:
            line, buf = buf.split(b"\r\n", 1)
            line = line.decode(errors="ignore")

            if line.startswith("*"):
                expected = int(line[1:])
                parts = []

                for _ in range(expected):
                    while b"\r\n" not in buf:
                        buf += sock.recv(4096)

                    bulk_len_line, buf = buf.split(b"\r\n", 1)
                    strlen = int(bulk_len_line.decode()[1:])

                    while len(buf) < strlen + 2:
                        buf += sock.recv(4096)

                    data = buf[:strlen].decode()
                    buf = buf[strlen + 2:]
                    parts.append(data)

                if parts[0] == "message":
                    msg_count += 1
                    print(f"[{name}] Received: {parts[2]} (count={msg_count})")

                    # If this thread has a quit threshold and has reached it → send QUIT
                    if quit_after is not None and msg_count >= quit_after:
                        print(f"[{name}] Sending QUIT")
                        time.sleep(2)
                        send_resp_array(sock, ["QUIT"])
                        time.sleep(0.1)
                        sock.close()
                        return


def main():
    threads = []

    # Thread-1: normal
    t1 = threading.Thread(target=subscriber_worker, args=("Worker-1",))
    t1.daemon = True
    t1.start()
    threads.append(t1)

    # Thread-2: normal
    t2 = threading.Thread(target=subscriber_worker, args=("Worker-2",))
    t2.daemon = True
    t2.start()
    threads.append(t2)

    # Thread-3: quit after 3 messages
    t3 = threading.Thread(target=subscriber_worker, args=("Worker-3", 1))
    t3.daemon = True
    t3.start()
    threads.append(t3)

    try:
        while True:
            time.sleep(1)
    except KeyboardInterrupt:
        pass


if __name__ == "__main__":
    main()
