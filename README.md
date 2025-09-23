# Creality Nano MMU

A firmware and tooling stack for the **Creality Nano MMU** (Multi‑Material Upgrade) that runs on an Arduino Nano. The project provides:

* A **C++ firmware** for the Nano that implements the MMU protocol.
* A **Python command‑line interface** (`mmu_cmd.py`) to send commands to the MMU.
* A **daemon** (`mmu_daemon.py`) that keeps the MMU alive and exposes a simple HTTP API.
* Configuration files and scripts for building, flashing and debugging.

---

## Table of Contents

- [Creality Nano MMU](#creality-nano-mmu)
  - [Table of Contents](#table-of-contents)
  - [Hardware](#hardware)
  - [Software](#software)
  - [Getting Started](#getting-started)
  - [Building the Firmware](#building-the-firmware)
  - [Running the Daemon](#running-the-daemon)
  - [Command Line Tool](#command-line-tool)
  - [Configuration](#configuration)
  - [Contributing](#contributing)
  - [License](#license)

---

## Hardware

The firmware targets an **Arduino Nano** (ATmega328P) connected to the MMU via the standard Creality MMU interface. The following pins are used:

| Pin | Function | Notes |
|-----|----------|-------|
| D0 | UART TX | MMU TX |
| D1 | UART RX | MMU RX |
| D2 | GPIO | Optional status LED |

The `pcb/` directory contains schematic and PCB files for a custom board that connects the Nano to the MMU.

## Software

The repository is split into three main parts:

| Directory | Purpose |
|-----------|---------|
| `pico-mmu-controller/` | C++ firmware for the Nano. Built with PlatformIO. |
| `script/` | Python utilities (daemon, CLI, config). |
| `pcb/` | PCB design assets. |

## Getting Started

1. **Clone the repo**
   ```bash
   git clone https://github.com/fernandoglatz/creality-pico-mmu.git
   cd creality-pico-mmu
   ```
2. **Install PlatformIO** (if you don't have it):
   ```bash
   pip install platformio
   ```
3. **Build and flash the firmware** – see the section below.
4. **Run the daemon** – see the section below.
5. **Use the CLI** – see the section below.

## Building the Firmware

The firmware is located in `pico-mmu-controller/` and uses PlatformIO.

```bash
cd pico-mmu-controller
pio run -e nano
pio run -e nano --target upload
```

* `-e nano` selects the Arduino Nano board.
* The `platformio.ini` file contains all required libraries.

After flashing, the Nano will start the MMU protocol and wait for commands.

## Running the Daemon

The daemon keeps the MMU alive and exposes a tiny HTTP API for remote control.

```bash
cd script
python3 mmu_daemon.py
```

The daemon reads `pico-mmu.cfg` for serial port settings. By default it looks for `/dev/ttyACM0`.

## Command Line Tool

The CLI (`mmu_cmd.py`) can send individual commands to the MMU.

```bash
python3 mmu_cmd.py --help
```

Example: request filament status

```bash
python3 mmu_cmd.py status
```

## Configuration

`pico-mmu.cfg` contains serial port and baud‑rate settings.

```ini
[serial]
port = /dev/ttyACM0
baudrate = 115200
```

Feel free to adjust the values for your setup.

## Contributing

Pull requests are welcome! Please open an issue first to discuss major changes.

1. Fork the repo.
2. Create a feature branch (`git checkout -b feature/foo`).
3. Commit your changes.
4. Push to your fork and open a pull request.

Make sure your code passes the existing tests (`pytest` in the `script/` folder).

## License

This project is licensed under the MIT License – see the [LICENSE](LICENSE) file for details.

---

Happy printing!