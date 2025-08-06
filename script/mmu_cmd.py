#!/usr/bin/env python3
import os
import socket
import sys
import time

SOCKET_PATH = "/tmp/pico_mmu_service.sock"

def send_command(sock, command: str) -> str:
    try:
        print(f"[Socket] --> {command}")
        sock.sendall((command + "\n").encode())

        buffer = ""
        while True:
            chunk = sock.recv(1024).decode(errors="replace")
            if not chunk:
                print("[Socket] Connection closed")
                return "ERROR"

            buffer += chunk
            while "\n" in buffer:
                line, buffer = buffer.split("\n", 1)
                line = line.strip()
                if line:
                    print(f"[Socket] <-- {line}")
                    if line.startswith("OK"):
                        return "OK"
                    elif line.startswith("ERROR"):
                        return "ERROR"
                    
            time.sleep(0.1)
                    
    except (BrokenPipeError, ConnectionResetError) as e:
        print(f"Communication error: {e}")
        return "ERROR"

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: mmu_cmd.py <command>")
        sys.exit(1)

    command_arg = " ".join(sys.argv[1:]).strip()

    while True:
        if not os.path.exists(SOCKET_PATH):
            print(f"Socket not found: {SOCKET_PATH}")
            time.sleep(1)
            continue

        try:
            with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as sock:
                sock.connect(SOCKET_PATH)
                
                if send_command(sock, command_arg) == "OK":
                    sys.exit(0)
                else:
                    sys.exit(1)

        except Exception as e:
            print(f"Communication failed: {e}")
            time.sleep(1)
