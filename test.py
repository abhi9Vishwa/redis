import socket
import time

def send_resp_command(host: str, port: int, command: list[str]):
    # time.sleep(1)
    # Encode the command in RESP format
    resp = f"*{len(command)}\r\n"
    for arg in command:
        resp += f"${len(arg)}\r\n{arg}\r\n"

    # Create TCP socket
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.connect((host, port))
        s.sendall(resp.encode())

        # Receive response (up to 1KB)
        response = s.recv(1024)
        print("Server response:", response.decode(errors="ignore"))

if __name__ == "__main__":
    # Example: send ECHO hey
    send_resp_command("127.0.0.1", 6379, ["ECHO", "hey"])
    send_resp_command("127.0.0.1", 6379, ["GET", "hello"])
    send_resp_command("127.0.0.1", 6379, ["SET", "hello", "okay", "EX", "5"])
    send_resp_command("127.0.0.1", 6379, ["GET", "hello"])
    send_resp_command("127.0.0.1", 6379, ["GET", "hello"])
    send_resp_command("127.0.0.1", 6379, ["TYPE", "hello"])
    send_resp_command("127.0.0.1", 6379, ["XADD", "stream_key", "1-*", "foo", "bar"])
    send_resp_command("127.0.0.1", 6379, ["XADD", "stream_key", "1-*", "temperature", "36", "humidity", "95"])
    send_resp_command("127.0.0.1", 6379, ["XADD", "stream_key", "1-*", "foo", "bar"])
    send_resp_command("127.0.0.1", 6379, ["XADD", "stream_key", "1-*", "temperature", "36", "humidity", "95"])
    send_resp_command("127.0.0.1", 6379, ["TYPE", "stream_key"])
    send_resp_command("127.0.0.1", 6379, ["XRANGE", "stream_key", "-", "+"])



