import socket
import time
import threading

def connect_with_retry(ip, port):
    while True:
        try:
            s = socket.socket()
            s.connect((ip, port))
            print(f"Connected to {ip}:{port}")
            return s
        except Exception as e:
            print(f"Connection failed: {e}. Retrying in 1s...")
            time.sleep(1)

# Select mode
ip = input("Input ip: ").strip()

while True:
    choice = input("Emulate Client(a) or CLI(b)\n").strip().lower()
    if choice == "a":
        port = 9000
        break
    elif choice == "b":
        port = 9001
        break

s = connect_with_retry(ip, port)
buffer = ""

def reader():
    global buffer, s
    while True:
        try:
            chunk = s.recv(4096)
            if not chunk:
                print("Disconnected by server.")
                return
            buffer += chunk.decode()

            # parse all packets
            while True:
                start = buffer.find("type:")
                if start == -1:
                    buffer = ""
                    break

                if start > 0:
                    buffer = buffer[start:]
                    start = 0

                sep = buffer.find("%%%")
                if sep == -1:
                    break

                type_name = buffer[start+5:sep]
                payload_start = sep + 3

                next_packet = buffer.find("type:", payload_start)
                if next_packet == -1:
                    payload = buffer[payload_start:].strip()
                    print(f"Server ({type_name}): {payload}")
                    buffer = ""
                    break
                else:
                    payload = buffer[payload_start:next_packet].strip()
                    print(f"Server ({type_name}): {payload}")
                    buffer = buffer[next_packet:]

        except Exception as e:
            print("Reader error:", e)
            return

# start reader thread
threading.Thread(target=reader, daemon=True).start()

# writer loop
while True:
    try:
        msg = input("")
        s.sendall((msg + "\n").encode())
    except Exception as e:
        print("Writer error:", e)
        break
