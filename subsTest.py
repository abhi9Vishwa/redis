import socket
import threading
import time

HOST = "127.0.0.1"
PORT = 6379
CHANNEL = "mychan1"


def send_resp_array(sock, arr):
    """
    Sends a RESP array: arr = ["SUBSCRIBE", "channel"]
    """
    out = f"*{len(arr)}\r\n"
    for item in arr:
        out += f"${len(item)}\r\n{item}\r\n"
    sock.sendall(out.encode())


def subscriber_worker(name):
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.connect((HOST, PORT))

    send_resp_array(sock, ["SUBSCRIBE", CHANNEL])
    print(f"[{name}] SUBSCRIBED to {CHANNEL}")

    buf = b""

    while True:
        chunk = sock.recv(4096)
        if not chunk:
            print(f"[{name}] Connection closed")
            break

        buf += chunk

        # Process RESP messages line by line (simple parser)
        while b"\r\n" in buf:
            line, buf = buf.split(b"\r\n", 1)
            line = line.decode(errors="ignore")

            # Redis pub/sub messages come in as arrays:
            # *3
            # $7
            # message
            # $<len>
            # <channel>
            # $<len>
            # <payload>

            if line.startswith("*"):
                expected = int(line[1:])
                parts = []

                # Read the next expected *bulk strings* blocks
                for _ in range(expected):
                    # read $len
                    while b"\r\n" not in buf:
                        buf += sock.recv(4096)
                    bulk_len_line, buf = buf.split(b"\r\n", 1)
                    bulk_len_line = bulk_len_line.decode()

                    assert bulk_len_line.startswith("$")
                    strlen = int(bulk_len_line[1:])

                    # read <data>
                    while len(buf) < strlen + 2:
                        buf += sock.recv(4096)

                    data = buf[:strlen].decode()
                    buf = buf[strlen + 2:]  # skip data + CRLF

                    parts.append(data)

                if parts[0] == "message":
                    print(f"[{name}] Received: {parts[2]}")


def main():
    threads = []
    for i in range(3):
        t = threading.Thread(target=subscriber_worker, args=(f"Worker-{i+1}",))
        t.daemon = True
        t.start()
        threads.append(t)

    try:
        while True:
            time.sleep(1)
    except KeyboardInterrupt:
        print("Shutting down")


if __name__ == "__main__":
    main()
