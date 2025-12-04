import socket
import threading
import time
from pathlib import Path


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


def reader():
    global s
    buf = b""  # buffer for incoming data

    downloads = Path(r"C:\Users\oscar\Downloads")

    while True:
        try:
            chunk = s.recv(4096)
            if not chunk:
                print("Disconnected.")
                break

            buf += chunk

            # Process as many full headers as we have
            while True:
                header_end = buf.find(b"\n")
                if header_end == -1:
                    break  # need more data

                # Extract header line
                header = buf[:header_end].decode()
                buf = buf[header_end + 1:]

                if not header.startswith("type:"):
                    print(f"Server (string): {header}")
                    continue

                parts = header.split("%%%")
                type_name = parts[0][5:]

                # STRING PACKET
                if type_name != "file":
                    payload = "%%%".join(parts[1:])
                    print(f"Server ({type_name}): {payload}")
                    continue

                # FILE PACKET
                filename = parts[1]
                filesize = int(parts[2])

                # Wait until we have EXACT file bytes
                while len(buf) < filesize:
                    more = s.recv(4096)
                    if not more:
                        raise ConnectionError("Disconnected during file transfer")
                    buf += more

                # Extract file bytes
                filedata = buf[:filesize]
                buf = buf[filesize:]

                save_path = downloads / filename
                with open(save_path, "wb") as f:
                    f.write(filedata)
                print("DEBUG header:", header)
                print("DEBUG filesize:", filesize)
                print("DEBUG first 200 bytes of filedata:")
                print(filedata[:200].decode(errors="replace"))
                print(f"Saved file: {save_path}")

        except Exception as e:
            print("Reader error:", e)
            break


# ------------------ MAIN ------------------

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

# Start reader thread
threading.Thread(target=reader, daemon=True).start()

# Writer loop
while True:
    try:
        msg = input("> ")
        s.sendall((msg + "\n").encode())
    except Exception as e:
        print("Writer error:", e)
        s = connect_with_retry(ip, port)
