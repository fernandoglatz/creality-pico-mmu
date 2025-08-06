#!/usr/bin/env python3
import glob
import http.client
import json
import logging
import os
import queue
import socket
import sys
import threading
import time

import serial

# Configurações
SOCKET_PATH = "/tmp/pico_mmu_service.sock"

BAUDRATE = 9600
RESPONSE_TERMINATORS = ("OK", "ERROR")

FILAMENT_FILE = "/var/lib/filament.txt"

KLIPPER_HOST = "127.0.0.1"
KLIPPER_PORT = 7125

ARDUINO_ALIVE_TIMEOUT_SECONDS = 30

# Variáveis globais
arduino_started = False
arduino_synced = False
arduino_last_alive = time.time()

sync_command = ""

serial_port = None
serial_reader_paused = threading.Event()
command_queue = queue.Queue()
output_conn = None
running = True

# Logging
logger = logging.getLogger()
logger.setLevel(logging.DEBUG)

file_handler = logging.FileHandler('/tmp/mmu_daemon.log', mode='w')
file_handler.setFormatter(logging.Formatter('%(asctime)s %(levelname)s: %(message)s'))
logger.addHandler(file_handler)

console_handler = logging.StreamHandler(sys.stdout)
console_handler.setFormatter(logging.Formatter('%(asctime)s %(levelname)s: %(message)s'))
logger.addHandler(console_handler)

def read_filament_file():
    try:
        with open(FILAMENT_FILE, "r") as f:
            filament = f.read().strip()
            logger.info(f"Read filament from file: {filament}")
            return filament
    except Exception as e:
        logger.error(f"Failed to read filament file: {e}")
        return None

def notify_filament_klipper(filament):
    try:
        conn = http.client.HTTPConnection(KLIPPER_HOST, KLIPPER_PORT, timeout=5)
        payload = {
            "script": f"SET_GCODE_VARIABLE MACRO=MMU_STATE VARIABLE=current_filament VALUE={filament}"
        }
        headers = {"Content-Type": "application/json"}
        conn.request("POST", "/printer/gcode/script", body=json.dumps(payload), headers=headers)
        response = conn.getresponse()
        logger.info(f"Klipper response: {response.status} {response.reason}")
        logger.debug(f"Klipper response body: {response.read().decode()}")
        conn.close()
    except ConnectionRefusedError as e:
        logger.warning(f"Connection refused by Klipper: {e}")
    except Exception as e:
        logger.error(f"Failed to notify Klipper: {e}")

def monitor_printer_status():
    global running
    global arduino_synced

    last_state = None

    while running:
        try:
            conn = http.client.HTTPConnection(KLIPPER_HOST, KLIPPER_PORT, timeout=5)
            conn.request("GET", "/printer/info")
            response = conn.getresponse()
            data = response.read()
            conn.close()

            if response.status == 200:
                info = json.loads(data.decode())
                state = info.get("result", {}).get("state")

                if state != last_state:
                    logger.info(f"Printer state changed: {last_state} -> {state}")

                    if state == "ready":
                        arduino_synced = False
                        filament = read_filament_file()
                        if filament:
                            notify_filament_klipper(filament)

                    last_state = state
            else:
                logger.warning(f"Failed to get printer info: {response.status}")
                last_state = None

        except Exception as e:
            logger.error(f"Error monitoring printer status: {e}")
            last_state = None

        time.sleep(10)

def monitor_arduino_status():
    global arduino_started
    global arduino_last_alive
    global running
    global serial_port

    while running:
        if arduino_started:
            if time.time() - arduino_last_alive > ARDUINO_ALIVE_TIMEOUT_SECONDS:
                logger.warning("Arduino is not alive. Restarting connection...")
                arduino_started = False

                try:
                    serial_port.close()
                except Exception:
                    pass
                serial_port = None

        time.sleep(1)

def scan_serial_ports():
    global serial_port
    global arduino_started
    global arduino_synced
    global running
    global sync_command
    
    logger.info("[Thread] scan_serial_ports started")
    while running:
        if serial_port is None or not serial_port.is_open:
            arduino_started = False

            candidates = glob.glob('/dev/ttyUSB*') + glob.glob('/dev/ttyACM*')
            for dev in candidates:
                try:              
                    port = serial.Serial(dev, BAUDRATE)
                    serial_port = port
                    logger.info(f"Connected to serial device: {dev}")
                    
                    time.sleep(5)

                    command = 'start'
                    response = send_command(command, False, False)

                    if response == 'OK':
                        if sync_command != "":
                            send_command(sync_command, False, False)
                        else:
                            arduino_synced = False

                        filament = read_filament_file()
                        if filament:
                            command = f'filament {filament}'
                            send_command(command, False, False)

                            command = 'filament_release'
                            send_command(command, False, False)
                        
                        arduino_started = True
                    break
                except Exception as e:
                    logger.warning(f"Failed to open {dev}: {e}")
        time.sleep(10)

def read_serial_background():
    global serial_port
    global serial_reader_paused
    global running
    global arduino_last_alive
    global arduino_started

    logger.info("[Thread] read_serial_background started")
    buffer = ""
    while running:
        if serial_port and serial_port.is_open and not serial_reader_paused.is_set():
            try:
                if serial_port.in_waiting:
                    line = serial_port.readline().decode(errors="ignore").strip()
                    if line:
                        logger.info(f"[Arduino] <-- {line}")
                        arduino_last_alive = time.time()

            except Exception as e:
                logger.error(f"Serial read error: {e}")

                if "Input/output error" in str(e):
                    arduino_started = False
                    
                    try:
                        serial_port.close()
                    except Exception:
                        pass
                    serial_port = None
        time.sleep(0.1)

def process_command_queue():
    global serial_port
    global arduino_started
    global arduino_synced
    global command_queue
    global output_conn
    global running
    global sync_command

    logger.info("[Thread] process_command_queue started")
    while running:
        try:
            command = command_queue.queue[0] if command_queue.qsize() > 0 else None

            if command and serial_port and serial_port.is_open and arduino_started:
                if command.lower().startswith("sync"):
                    if not arduino_synced:
                        sync_command = command
                        send_command(command, True, True)
                        arduino_synced = True
                    else:
                        send_socket("OK")
                        remove_command_from_queue()
                        

                elif command.lower().startswith("filament "):
                    filament_value = command[len("filament "):].strip()
                    with open(FILAMENT_FILE, "w") as f:
                        f.write(filament_value)
                    
                    send_command(command, True, True)

                elif command.lower() == "filament_reengage":
                    if not os.path.exists(FILAMENT_FILE):
                        logger.warning("No filament stored to reengage.")

                        send_socket("ERROR")
                        remove_command_from_queue()

                    else:
                        with open(FILAMENT_FILE, "r") as f:
                            filament_value = f.read().strip()

                            if not filament_value:
                                logger.warning("Stored filament value is empty.")
                                send_socket("ERROR")
                                remove_command_from_queue()

                            else:
                                send_command(f"filament {filament_value}", True, True)
                
                else:
                    send_command(command, True, True)

        except Exception as e:
            logger.error(f"Error while processing command queue: {e}")
        
        time.sleep(0.1)

def remove_command_from_queue():
    global command_queue

    if command_queue.qsize() > 0:
        command_queue.get()

def send_socket(response: str):
    global output_conn

    if output_conn:
        try:
            logger.info(f"[Socket] --> {response}")
            output_conn.sendall((response + "\n").encode())
        except Exception as e:
            logger.warning(f"Socket write error: {e}")

def send_command(command: str, send_socket: bool, remove_from_queue: bool) -> str:
    global serial_port
    global serial_reader_paused
    global output_conn
    global running
    global arduino_last_alive
    global arduino_started
    
    try:
        serial_reader_paused.set()

        logger.info(f"[Arduino] --> {command}")
        serial_port.write((command + "\n").encode())
        serial_port.flush()
        logger.info(f"Flushed")

        while running:
            if serial_port.in_waiting:
                line = serial_port.readline().decode(errors="ignore").strip()
                if line:
                    logger.info(f"[Arduino] <-- {line}")
                    arduino_last_alive = time.time()

                    if output_conn and send_socket:
                        try:
                            logger.info(f"[Socket] --> {line}")
                            output_conn.sendall((line + "\n").encode())
                        except Exception as e:
                            logger.warning(f"Socket write error: {e}")
                    if any(line.startswith(term) for term in RESPONSE_TERMINATORS):
                        if remove_from_queue:
                            remove_command_from_queue()

                        return line
            time.sleep(0.1)
    except Exception as e:
        logger.error(f"Error while writing to serial: {e}")
        arduino_started = False

        try:
            serial_port.close()
        except Exception:
            pass
        serial_port = None
    finally:
        serial_reader_paused.clear()

def socket_server():
    global output_conn
    global command_queue
    global running

    if os.path.exists(SOCKET_PATH):
        os.remove(SOCKET_PATH)

    server = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    try:
        server.bind(SOCKET_PATH)
        os.chmod(SOCKET_PATH, 0o666)
        server.listen()
        logger.info(f"Socket server listening on {SOCKET_PATH}")
    except Exception as e:
        logger.error(f"Failed to bind socket: {e}")
        sys.exit(1)

    while running:
        try:
            conn, _ = server.accept()
            output_conn = conn
            logger.info("---------- Client connected ----------")

            buffer = ""
            while running:
                chunk = conn.recv(1024).decode(errors="replace")
                if not chunk:
                    break

                buffer += chunk
                while "\n" in buffer:
                    line, buffer = buffer.split("\n", 1)
                    line = line.strip()
                    if line:
                        logger.info(f"[Socket] <-- {line}")
                        command_queue.put(line)

                        if command_queue.qsize() > 1:
                            logger.debug(f"[Queue] Size: {command_queue.qsize()}")
                
                time.sleep(0.1)
        except Exception as e:
            logger.error(f"Socket error: {e}")
        finally:
            if output_conn:
                output_conn.close()
                output_conn = None
                logger.info("---------- Client disconnected ----------")

if __name__ == "__main__":
    try:
        logger.info("MMU Daemon starting...")
        threading.Thread(target=scan_serial_ports, daemon=True).start()
        threading.Thread(target=read_serial_background, daemon=True).start()
        threading.Thread(target=process_command_queue, daemon=True).start()
        threading.Thread(target=monitor_printer_status, daemon=True).start()
        threading.Thread(target=monitor_arduino_status, daemon=True).start()
        socket_server()
    except KeyboardInterrupt:
        logger.info("Shutting down...")
    finally:
        running = False
        if serial_port and serial_port.is_open:
            serial_port.close()
        logger.info("Daemon terminated.")
