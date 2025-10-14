# Creality Pico MMU

A firmware and tooling stack for a custom **8-line Multi-Material Unit (MMU)** for the **Creality Ender 3 V3 KE** 3D printer. The project provides:

* A **C++ firmware** for ATmega328P (Arduino Pro Mini/Nano) that controls the MMU hardware via serial communication.
* A **Python daemon** (`mmu_daemon.py`) that manages serial communication, provides automatic reconnection, and integrates with Klipper via socket interface.
* A **Python command-line interface** (`mmu_cmd.py`) to send commands to the MMU daemon.
* **Klipper configuration** (`pico-mmu.cfg`) with macros for seamless integration with Klipper firmware.
* Hardware designs for filament cutter and filament hub components.

## Features

✨ **8-Line Multi-Material System**
- Supports up to 8 different filament colors/materials simultaneously
- Individual filament presence sensors for each line
- Real-time filament state monitoring

🎯 **Automatic Filament Management**
- Servo-driven filament selector with precise positioning
- Automatic filament cutting between material changes
- Smart extrude/retract with acceleration control
- Stuck filament detection and error handling

🔌 **Seamless Klipper Integration**
- Native G-code macros (T0-T7 for tool changes)
- Daemon-based architecture with automatic reconnection
- HTTP API integration with Klipper
- State persistence across restarts

💡 **Visual & Audio Feedback**
- 16 RGB LEDs (WS2812B) showing filament status
- Color-coded status indicators per filament
- Musical notifications for events (startup, errors, success)
- Action button for manual control

🛠️ **Hardware Design Included**
- Custom filament cutter with dual 5015 fan cooling
- 9-line filament hub with integrated runout sensor
- Protoboard wiring diagram for easy assembly
- 3D printable STL files available on Printables

⚙️ **Highly Configurable**
- Adjustable servo positions, speeds, and distances
- Configurable acceleration curves
- Per-setup calibration via Klipper macros
- Runtime parameter synchronization

## System Requirements

### Hardware
- Creality Ender 3 V3 KE (or compatible 3D printer)
- ATmega328P microcontroller (Arduino Pro Mini 16MHz or Arduino Nano)
- MCP23017 I2C I/O expander
- 2x Servo motors (selector + cutter)
- Stepper motor with driver for filament feeding
- WS2812B NeoPixel LED strip (16 LEDs)
- 8x Filament sensors + 1x Hub sensor
- Piezo buzzer
- Push button
- Various electronics components (see Hardware section)

### Software
- Klipper firmware on printer
- Python 3.x
- PlatformIO (for firmware compilation)
- pyserial Python library

---

## Table of Contents

- [Creality Pico MMU](#creality-pico-mmu)
  - [Features](#features)
  - [System Requirements](#system-requirements)
  - [Table of Contents](#table-of-contents)
  - [Hardware](#hardware)
  - [Filament Cutter](#filament-cutter)
  - [Filament Hub](#filament-hub)
  - [Software Architecture](#software-architecture)
  - [Getting Started](#getting-started)
  - [Building the Firmware](#building-the-firmware)
  - [Running the Daemon](#running-the-daemon)
  - [Command Line Tool](#command-line-tool)
  - [Klipper Integration](#klipper-integration)
  - [Troubleshooting](#troubleshooting)
  - [Development](#development)
  - [Contributing](#contributing)
  - [License](#license)

---

## Hardware

The firmware targets an **ATmega328P** microcontroller (Arduino Pro Mini or Arduino Nano) that controls an 8-line MMU system with integrated sensors, servos, and LED feedback. 

### Core Components

- **Microcontroller**: ATmega328P (Arduino Pro Mini 16MHz or Arduino Nano)
- **I/O Expander**: MCP23017 (I2C) - provides 16 additional GPIO pins for sensors and control
- **LED System**: WS2812B NeoPixel strip (16 LEDs arranged as 2 bars of 8 LEDs each)
- **Servos**: 
  - MMU selector servo (positions filament selector wheel)
  - Filament cutter servo (operates cutting mechanism)
- **Stepper Motor**: Controls filament feeding/retracting with configurable acceleration/deceleration
- **Audio Feedback**: Piezo buzzer for status notifications
- **Sensors**:
  - 8x filament presence sensors (one per filament line)
  - 1x filament hub sensor (detects filament movement)
  - 1x action button (manual control)

### Pin Configuration

#### Direct ATmega328P Pins
| Pin | Function | Description |
|-----|----------|-------------|
| D2 | Filament Hub Sensor | Interrupt-driven sensor for filament movement detection |
| D3 | Cutter Servo PWM | Controls filament cutting servo |
| D5 | NeoPixel Data | WS2812B LED strip data line |
| D6 | Buzzer | Piezo buzzer for audio feedback |
| D7 | MMU Direction | Stepper motor direction control |
| D8 | MMU Step | Stepper motor step signal |
| D9 | MMU Enable | Stepper motor enable/disable |
| D10 | MMU Servo PWM | Controls MMU selector servo |
| SDA/SCL | I2C Bus | Communication with MCP23017 |

#### MCP23017 I/O Expander Pins
| Pin | Function | Description |
|-----|----------|-------------|
| 0 | Action Button | Manual control button (INPUT_PULLUP) |
| 1 | Creality Filament Sensor | Output signal to printer's filament runout sensor |
| 8-15 | Filament Sensors 1-8 | Individual filament presence detection (INPUT_PULLUP) |

### Hardware Features

- **Stepper Motor Control**: Microstepping (64 microsteps), configurable RPM (50-500), acceleration/deceleration curves
- **LED Status Indicators**: Per-filament status (idle, active, loaded, error states)
- **Audio Notifications**: Startup, error, filament inserted/removed, success melodies
- **Sensor Monitoring**: Real-time filament presence and movement detection
- **Servo Control**: Precise positioning for filament selection and cutting

The `pcb/` directory contains a protoboard diagram (`protoboard-diagram.fzz`) for wiring the components.

## Filament Cutter

The **Filament Cutter** is a custom toolhead modification designed specifically for use with the 8-line Pico MMU on the **Creality Ender 3 V3 KE**. This component enables automatic filament cutting between material changes, eliminating the need for manual intervention.

The design is based on the FatBurner cooling system and integrates a servo-driven blade mechanism that cuts the filament between the extruder motor and the hotend.

**Model files:** [Printables - Filament Cutter](https://www.printables.com/model/1433552-filament-cutter-creality-ender-3-v3-ke)

### Features

- Servo-driven cutting mechanism using TD-8120MG (20Kg) or MG995 (10Kg) servo
- Integrated dual 5015 fan cooling system (FatBurner-based)
- 4020 hotend fan adapter
- Compact design that fits between the extruder motor and hotend
- Uses standard box cutter blade for reliable cutting

### Required Parts

- **8x** Brass insert nut M3 (4mm OD x 4mm length)
- **4x** Superb screws (for the fan adapter)
- **4x** M3 flat head Allen screws (20mm)
- **2x** M3 flat head Allen screws (8mm)
- **1x** M3 flat head Allen screw (6mm)
- **4x** M3 Allen screws (8mm)
- **2x** M3 Allen screws (25mm)
- **2x** Fan 5015 (24V)
- **1x** Fan 4020 (24V)
- **3x** JST SH1 mini 1.25mm connector
- **6x** Heat shrink tubes
- **1x** Servo TD-8120MG (20Kg) or MG995 (10Kg)
- **1x** PTFE tube (1m, Creality 2mm ID)
- **1x** Box cutter blade

### Print Settings

- **Infill:** 100%
- **Support:** Yes
- **Support Type:** Snug and Tree Organic (for the FatBurner frame)
- **Raft Layers:** 10 (only for the FatBurner frame)

**Recommendations:**
- Print parts close to the hotend in **ABS** to avoid PLA deformation
- Use the **TD-8120MG 20Kg servo** for better robustness
- Verify your MMU board supports the servo current requirements
- Check camera mount clearance to avoid collisions

### Assembly Instructions

1. **Print all parts** using the provided 3MF file in `models/filament-cutter/`
2. **Insert brass insert nuts** into all designated holes
3. **Modify fan connectors:**
   - Replace 5015 and 4020 fan connectors with JST SH1 mini
   - May need to invert GND/24V pins in the connector
4. **Assemble cooling system:**
   - Attach 5015 fans to the FatBurner frame
   - Attach 40mm fan adapter to the hotend
   - Attach 4020 fan to the adapter
5. **Prepare hotend:**
   - Disassemble the toolhead (detach radiator from rear panel)
   - Cut PTFE tube, leaving only 1mm protruding from hotend radiator
   - Make an internal cone-shaped cut in the PTFE to facilitate filament entry
6. **Install cutting mechanism:**
   - Attach cutting base to hotend radiator using 25mm screws
   - Attach extruder motor
   - Insert box cutter blade into blade support using pliers (use cyanoacrylate glue to secure)
7. **Test cutting:**
   - Insert filament for testing
   - Manually push blade support into cutting base to verify cutting action
8. **Final assembly:**
   - Connect fans to extruder plate
   - Attach FatBurner frame with 6mm screw
   - Position servo holder with 8mm screws
   - Insert gear into servo (requires considerable force)
9. **Servo setup:**
   - Connect servo to power and set to initial position (0°)
   - Insert blade support, lightly touch filament, then back off 1mm
   - Insert servo into holder (mark blade support position for future maintenance)
   - Tighten all servo screws
   - Test cutting angle electronically (typically ~90°, always return to 0° between tests)
   - Apply petroleum jelly to moving parts

### Design Notes

This filament cutter is a remix that combines elements from several existing designs:

- Based on [FatBurner cooling system](https://www.printables.com/model/887597-fatburner-ender-3-v3-ke-dual-5015-fan-cooling-syst)
- Adapted cutting mechanism from [Ender 3 V3 SE filament cutter](https://www.printables.com/model/1277295-filament-cutter-for-ender-3-v3-se-servo-motor-edit)
- Uses [40mm fan adapter](https://www.printables.com/model/718283-ender-3-v3-se-25-40mm-fan-bracket-low-profile)

**Key modifications:**
- Custom cutout in FatBurner frame to accommodate filament cutter
- Shortened right-side duct for improved airflow
- Relocated mounting holes and reinforced structure on cutting base

## Filament Hub

The **Filament Hub** is a 9-line filament distribution system designed for the custom 8-line Pico MMU on the **Creality Ender 3 V3 KE**. It manages multiple filament feeds and includes an integrated filament runout sensor to detect when filament runs out during printing.

The hub uses ECAS04 push-to-connect fittings for quick and secure PTFE tube connections, and features a KW11-3Z micro switch for runout detection.

**Model files:** [Printables - Filament Hub 9 Lines](https://www.printables.com/model/1432682-filament-hub-9-lines-creality-ender-3-v3-ke)

### Features

- 9-line capacity (supports 8-line MMU plus one spare)
- Integrated filament runout sensor with micro switch
- ECAS04 quick-connect fittings for all filament lines
- Compact design suitable for mounting on printer frame
- Compatible with standard 1.75mm filament

### Required Parts

- **2x** M3 Allen screws (8mm)
- **2x** M3 Allen screws (10mm)
- **9x** ECAS04 connector (push-to-connect PTFE fittings)
- **1x** KW11-3Z micro switch (filament runout sensor)
- **1x** Zip tie (for cable management)

### Print Settings

- **Infill:** 100%
- **Support:** Yes
- **Support Type:** Tree Organic
- **Raft Layers:** 10

**Important Notes:**
- Do **NOT** add support on top of the hub - supports must only be on the bottom/sides
- Adding supports on top will make them impossible to remove and may damage the hub
- All models are oriented correctly for printing in the 3MF file

### Assembly and Maintenance

1. **Print the hub** using the provided 3MF file in `models/filament-hub/`
2. **Install ECAS04 connectors:**
   - Insert connectors into all 9 ports
   - Use cyanoacrylate glue to secure them permanently
3. **Install micro switch:**
   - Mount the KW11-3Z micro switch in the designated position
   - Secure with appropriate screws
4. **Mount the hub:**
   - Use M3 screws (8mm and 10mm) to mount hub to printer frame
   - Use zip tie for cable management
5. **Lubrication:**
   - Apply petroleum jelly to the inside of the hub
   - Apply petroleum jelly to the switch end cap
   - This ensures smooth filament movement and prevents binding
6. **Calibration:**
   - Test with different filament types
   - Adjust micro switch end-of-travel angle if needed for reliable detection
   - Fine-tune based on filament diameter variations

### Design Notes

This hub is a remix based on the [Filament Runout Sensor Enclosure](https://www.printables.com/model/11416-filament-runout-sensor-enclosure) by stevehlau, customized specifically for the Creality Ender 3 V3 KE and integrated with the 8-line Pico MMU system.

**Key features of the remix:**
- Expanded to 9 lines for full MMU support
- Optimized geometry for Ender 3 V3 KE mounting
- Integrated runout sensor in a compact form factor

## Software Architecture

The software stack consists of three main layers that work together to provide seamless multi-material printing:

### Repository Structure

| Directory | Purpose |
|-----------|---------|
| `pico-mmu-controller/` | C++ firmware for ATmega328P. Built with PlatformIO. Handles low-level hardware control. |
| `script/` | Python daemon and utilities for system integration and communication. |
| `pcb/` | Hardware design files (Fritzing protoboard diagram). |
| `models/` | 3D printable components (filament cutter, filament hub). |

### Software Components

#### 1. Firmware (`main.cpp`)
The ATmega328P firmware provides:
- **Serial Protocol**: Command-based communication at 9600 baud
- **State Machine**: Manages filament states, sensor monitoring, and LED feedback
- **Motor Control**: Precise stepper control with acceleration/deceleration
- **Servo Control**: Position management for selector and cutter servos
- **Sensor Integration**: Real-time monitoring of 8 filament sensors and hub sensor
- **LED Feedback**: Visual status indication for each filament line
- **Audio Notifications**: Musical feedback for events (startup, errors, filament changes)

**Key Commands**:
- `START` - Initialize system
- `SYNC` - Synchronize configuration parameters
- `FILAMENT <n>` - Select filament line (0-7)
- `EXTRUDE <mm> <rpm>` - Feed filament
- `RETRACT <mm> <rpm>` - Retract filament
- `CUTTER_POSITION <angle>` - Control cutter servo
- `MMU_POSITION <angle>` - Control selector servo

#### 2. Daemon (`mmu_daemon.py`)
The Python daemon acts as a bridge between Klipper and the MMU firmware:
- **Serial Management**: Automatic port detection and reconnection
- **Socket Interface**: Unix socket server at `/tmp/pico_mmu_service.sock`
- **Command Queue**: Thread-safe command queuing and processing
- **State Persistence**: Saves current filament selection to `/var/lib/filament.txt`
- **Klipper Integration**: HTTP API calls to update Klipper state variables
- **Health Monitoring**: Tracks Arduino heartbeat (ALIVE messages every 5 seconds)
- **Auto-Recovery**: Restarts connection on communication failures

**Daemon Threads**:
- `scan_serial_ports`: Detects and connects to Arduino on USB/ACM ports
- `read_serial_background`: Continuous serial port monitoring
- `process_command_queue`: Sequential command execution
- `monitor_printer_status`: Watches Klipper state changes
- `monitor_arduino_status`: Detects communication timeouts
- `socket_server`: Handles client connections

#### 3. Command-Line Interface (`mmu_cmd.py`)
Simple utility to send commands directly to the daemon:
```bash
python3 mmu_cmd.py <command>
```
Communicates via Unix socket, waits for OK/ERROR response.

#### 4. Klipper Configuration (`pico-mmu.cfg`)
Provides G-code macros for integration:
- **Tool Selection**: `T0` through `T7` macros for filament selection
- **MMU Control**: Direct access to all MMU functions
- **Configuration Variables**: Adjustable parameters (positions, distances, speeds)
- **Automatic Filament Change**: `MMU_SWITCH_FILAMENT` macro handles complete swap sequence

**Configuration Parameters** (adjustable in `MMU_SWITCH_FILAMENT` macro):
- `filament_positions`: Servo angles for each filament (default: 170,148,126,104,80,56,32,10)
- `extrude_distance`: Filament feed distance in mm (default: 32)
- `retract_distance`: Filament retract distance in mm (default: 60)
- `mm_per_rotation`: Filament distance per motor rotation (default: 18.28571429)
- `mm_to_stuck`: Distance threshold for stuck detection (default: 80)
- `cutter_position_opened`: Cutter servo open angle (default: 0)
- `cutter_position_closed`: Cutter servo closed angle (default: 120)

## Getting Started

### Prerequisites

- **Hardware**: Assembled MMU system with ATmega328P, MCP23017, servos, stepper motor, sensors, and LEDs
- **3D Printer**: Creality Ender 3 V3 KE (or compatible) running Klipper firmware
- **Python**: Python 3.x installed on the printer's system
- **PlatformIO**: For compiling and flashing firmware

### Installation Steps

1. **Clone the repository**
   ```bash
   git clone https://github.com/fernandoglatz/creality-pico-mmu.git
   cd creality-pico-mmu
   ```

2. **Install PlatformIO** (if not already installed):
   ```bash
   pip install platformio
   ```

3. **Build and flash the firmware** (see [Building the Firmware](#building-the-firmware))

4. **Install Python dependencies** on your printer:
   ```bash
   pip install pyserial
   ```

5. **Copy scripts to printer**:
   ```bash
   # Copy to Klipper config directory
   cp script/mmu_daemon.py /usr/data/printer_data/config/pico-mmu/
   cp script/mmu_cmd.py /usr/data/printer_data/config/pico-mmu/
   cp script/pico-mmu.cfg /usr/data/printer_data/config/pico-mmu/
   ```

6. **Set up daemon service** (copy `script/S60mmu_daemon` to `/etc/init.d/` or create a systemd service)

7. **Include configuration in Klipper**:
   Add to your `printer.cfg`:
   ```ini
   [include pico-mmu/pico-mmu.cfg]
   ```

8. **Start the daemon**:
   ```bash
   /etc/init.d/S60mmu_daemon start
   ```
   Or manually:
   ```bash
   python3 /usr/data/printer_data/config/pico-mmu/mmu_daemon.py
   ```

## Building the Firmware

The firmware is located in `pico-mmu-controller/` and uses PlatformIO with the Arduino framework.

### Supported Boards

Two board configurations are available in `platformio.ini`:
- `pro16MHzatmega328` - Arduino Pro Mini 16MHz
- `nanoatmega328` - Arduino Nano (recommended)

### Build and Flash

```bash
cd pico-mmu-controller

# For Arduino Nano (recommended)
pio run -e nanoatmega328
pio run -e nanoatmega328 --target upload

# For Arduino Pro Mini
pio run -e pro16MHzatmega328
pio run -e pro16MHzatmega328 --target upload
```

### Dependencies

The firmware requires the following libraries (automatically installed by PlatformIO):
- `Servo` (v1.2.2) - Servo motor control
- `Adafruit NeoPixel` (v1.15.1) - WS2812B LED strip control
- `Adafruit MCP23017 Arduino Library` (v2.3.2) - I2C I/O expander

### Serial Monitor

After flashing, you can monitor the serial output:
```bash
pio device monitor -e nanoatmega328
```

The firmware will output:
```
Starting...
READY
```

Send `START` command to initialize the system.

## Running the Daemon

The daemon (`mmu_daemon.py`) manages communication between Klipper and the MMU firmware. It provides:
- Automatic serial port detection (`/dev/ttyUSB*`, `/dev/ttyACM*`)
- Auto-reconnection on communication failures
- Command queuing and processing
- Unix socket server for client communication
- State persistence and Klipper integration

### Manual Start

```bash
cd script
python3 mmu_daemon.py
```

The daemon logs to `/tmp/mmu_daemon.log` and console output.

### Service Installation

For automatic startup, use the provided init script:

```bash
# Copy the service script
sudo cp script/S60mmu_daemon /etc/init.d/
sudo chmod +x /etc/init.d/S60mmu_daemon

# Start the service
/etc/init.d/S60mmu_daemon start

# Enable on boot (depends on your system)
```

### Daemon Status

The daemon creates a Unix socket at `/tmp/pico_mmu_service.sock` for client connections.

Check if it's running:
```bash
ls -l /tmp/pico_mmu_service.sock
```

View logs:
```bash
tail -f /tmp/mmu_daemon.log
```

## Command Line Tool

The CLI (`mmu_cmd.py`) sends commands directly to the MMU daemon via Unix socket.

### Usage

```bash
python3 mmu_cmd.py <command>
```

### Available Commands

**System Commands:**
```bash
python3 mmu_cmd.py start              # Initialize MMU system
python3 mmu_cmd.py "sync FILAMENT_POSITIONS (...)"  # Sync configuration
```

**Filament Control:**
```bash
python3 mmu_cmd.py "filament 0"       # Select filament T0
python3 mmu_cmd.py filament_release   # Release filament selector
python3 mmu_cmd.py filament_reengage  # Re-engage last filament
```

**Motor Control:**
```bash
python3 mmu_cmd.py "extrude 32 500"   # Extrude 32mm at 500 RPM
python3 mmu_cmd.py "retract 60 210"   # Retract 60mm at 210 RPM
python3 mmu_cmd.py "mmu_rotate 360 500"  # Rotate motor 360° at 500 RPM
```

**Servo Control:**
```bash
python3 mmu_cmd.py "mmu_position 90"      # Set selector servo to 90°
python3 mmu_cmd.py "cutter_position 120"  # Set cutter servo to 120°
```

**Testing:**
```bash
python3 mmu_cmd.py test_leds          # Test all LEDs
python3 mmu_cmd.py "test_led 1"       # Test LED for filament 1
python3 mmu_cmd.py "midi 0"           # Play startup melody
```

### Exit Codes
- `0` - Command successful (received OK)
- `1` - Command failed (received ERROR or communication error)

## Klipper Integration

### Configuration

The `pico-mmu.cfg` file contains Klipper-compatible G-code macros and configuration. Key settings are in the `MMU_SWITCH_FILAMENT` macro:

```gcode
[gcode_macro MMU_SWITCH_FILAMENT]
variable_filament_positions: 170,148,126,104,80,56,32,10
variable_extrude_distance: 32
variable_retract_distance: 60
variable_mm_per_rotation: 18.28571429
variable_mm_to_stuck: 80
variable_cutter_position_opened: 0
variable_cutter_position_closed: 120
```

### Using in G-code

**Tool Selection:**
```gcode
T0  ; Select filament 0
T1  ; Select filament 1
T7  ; Select filament 7
```

**Manual Commands:**
```gcode
MMU_FILAMENT INDEX=3                    ; Select filament 3
MMU_EXTRUDE DISTANCE=50                 ; Extrude 50mm
MMU_RETRACT DISTANCE=60                 ; Retract 60mm
MMU_CUTTER_POSITION POSITION=120        ; Close cutter
MMU_FILAMENT_RELEASE                    ; Release selector
MMU_MIDI SLOT=4                         ; Play victory sound
```

### Filament Change Sequence

When you use `T0`-`T7`, the `MMU_SWITCH_FILAMENT` macro executes:
1. Syncs configuration with firmware
2. Heats extruder if needed (minimum 190°C)
3. Turns on cooling fan
4. Moves toolhead to safe position (X=-10)
5. Retracts current filament
6. Cuts filament
7. Retracts to MMU selector
8. Selects new filament
9. Feeds new filament to hotend
10. Purges filament (150mm on first change)
11. Verifies filament loaded correctly

## Troubleshooting

### Arduino Not Detected
- Check USB connection
- Verify the device appears: `ls -l /dev/ttyUSB* /dev/ttyACM*`
- Check daemon logs: `tail -f /tmp/mmu_daemon.log`
- Restart daemon: `/etc/init.d/S60mmu_daemon restart`

### Filament Not Loading
- Check sensor states in firmware output
- Verify filament is properly inserted in filament hub
- Check servo positions are calibrated correctly
- Verify stepper motor is powered and functioning

### Communication Timeout
- Daemon sends ALIVE heartbeat every 5 seconds
- If no response for 30 seconds, connection is reset
- Check serial baud rate matches (9600)
- Verify firmware is running: `pio device monitor`

### LED Status Meanings
- **Black**: No filament detected
- **Cyan**: Filament detected but not selected
- **White**: Currently selected/active
- **Green**: Filament loaded and ready
- **Dark Green**: Filament loaded, waiting at sensor
- **Orange**: Hub sensor stuck/not responding correctly
- **Red**: Error - filament missing or failed to load

### Configuration Issues
- Adjust `filament_positions` servo angles in `pico-mmu.cfg`
- Calibrate `mm_per_rotation` based on your extruder gear ratio
- Tune `extrude_distance` and `retract_distance` for your setup
- Set `cutter_position_opened` and `cutter_position_closed` for your servo

## Development

### Code Structure

**Firmware** (`pico-mmu-controller/src/main.cpp`):
- Setup functions for hardware initialization
- Serial command parser (`processSerialInput()`)
- Motor control with acceleration (`rotateMmu()`, `rotateMmuToSensor()`)
- Sensor monitoring (`readSensors()`, `readHubState()`)
- LED and audio feedback functions
- State management for filament selection

**Daemon** (`script/mmu_daemon.py`):
- Multi-threaded architecture for concurrent operations
- Serial port auto-detection and recovery
- Command queue processing
- Socket server for client communication
- Klipper HTTP API integration
- State persistence for filament selection

### Compile-Time Checks

The firmware includes compile-time assertions (`compile_checks.h`) to validate configuration:
- Array sizes match `NUMBER_OF_FILAMENTS`
- Configuration is consistent across all components

### Serial Communication Protocol

The firmware implements a text-based command protocol over serial (9600 baud):

**Command Format**: `COMMAND [ARGS]\n`

**Response Format**: `OK\n`, `ERROR\n`, or `ALIVE\n`

**Command Reference**:
| Command | Arguments | Description | Response |
|---------|-----------|-------------|----------|
| `START` | None | Initialize system, read sensors, display startup | `OK` |
| `SYNC` | `FILAMENT_POSITIONS (...) EXTRUDE_MM n RETRACT_MM n MM_PER_ROTATION f MM_TO_STUCK n CUTTER_OPENED n CUTTER_CLOSED n` | Synchronize configuration | `OK` |
| `FILAMENT` | `<0-7>` | Select filament line | `OK`/`ERROR` |
| `FILAMENT_RELEASE` | None | Release selector to neutral position | `OK` (async) |
| `EXTRUDE` | `<mm> [rpm]` | Feed filament (wait for hub sensor) | `OK` |
| `RETRACT` | `<mm> [rpm]` | Retract filament (wait for hub sensor) | `OK` (async) |
| `SWAP_FINISH` | None | Verify filament swap completed successfully | `OK`/`ERROR` |
| `MMU_POSITION` | `<angle>` | Set selector servo position | `OK` |
| `CUTTER_POSITION` | `<angle>` | Set cutter servo position | `OK` |
| `MMU_ROTATE` | `<degrees> [rpm]` | Rotate stepper motor | `OK` |
| `MIDI` | `<0-4>` | Play melody (0=startup, 1=error, 2=inserted, 3=removed, 4=victory) | `OK`/`ERROR` |
| `TEST_LEDS` | None | Test all LED strips | `OK` |
| `TEST_LED` | `<1-8>` | Test specific LED | `OK` |

**Logging Format**: `[<millis>] <LEVEL> - <message>`

**Example Session**:
```
[Arduino] --> START
[Arduino] <-- [123] INFO - Starting up...
[Arduino] <-- [456] INFO - Started
[Arduino] <-- OK
[Arduino] --> FILAMENT 0
[Arduino] <-- [789] INFO - Setting filament T0
[Arduino] <-- [1234] INFO - Filament set
[Arduino] <-- OK
```

### Debugging

Enable verbose logging:
```python
# In mmu_daemon.py
logger.setLevel(logging.DEBUG)
```

Monitor serial communication:
```bash
pio device monitor -e nanoatmega328 -b 9600
```

Test individual components:
```bash
python3 mmu_cmd.py test_leds
python3 mmu_cmd.py "test_led 1"
python3 mmu_cmd.py "midi 0"
```

Check daemon status:
```bash
# View real-time logs
tail -f /tmp/mmu_daemon.log

# Check socket exists
ls -l /tmp/pico_mmu_service.sock

# Test socket communication
echo "test_leds" | nc -U /tmp/pico_mmu_service.sock
```

## Contributing

Contributions are welcome! Please follow these guidelines:

1. **Open an issue** first to discuss major changes
2. **Fork the repository** and create a feature branch
3. **Follow the existing code style**:
   - C++ firmware: Arduino conventions
   - Python: PEP 8 style guide
4. **Test thoroughly**:
   - Verify firmware compiles without warnings
   - Test daemon with actual hardware
   - Ensure Klipper integration works correctly
5. **Document your changes**:
   - Update README if adding features
   - Add comments for complex logic
   - Update configuration examples if needed
6. **Submit a pull request** with clear description

### Areas for Improvement
- Add support for more than 8 filament lines
- Implement filament runout recovery
- Add web interface for configuration
- Support for different printer models
- Improved error recovery mechanisms

## License

This project is licensed under the Apache License 2.0 – see the [LICENSE](LICENSE) file for details.

### Third-Party Attributions

- **Filament Cutter**: Based on designs from:
  - [FatBurner cooling system](https://www.printables.com/model/887597-fatburner-ender-3-v3-ke-dual-5015-fan-cooling-syst)
  - [Ender 3 V3 SE filament cutter](https://www.printables.com/model/1277295-filament-cutter-for-ender-3-v3-se-servo-motor-edit)
  - [40mm fan adapter](https://www.printables.com/model/718283-ender-3-v3-se-25-40mm-fan-bracket-low-profile)

- **Filament Hub**: Remix based on:
  - [Filament Runout Sensor Enclosure](https://www.printables.com/model/11416-filament-runout-sensor-enclosure) by stevehlau

### Libraries Used

- Arduino Servo Library (v1.2.2) - LGPL 2.1
- Adafruit NeoPixel (v1.15.1) - LGPL 3.0
- Adafruit MCP23017 (v2.3.2) - BSD License
- Python pyserial - BSD License

---

Happy printing!