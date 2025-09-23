# Copilot Instructions for Creality‑Pico‑MMU

## 1. Project Overview

The repository implements the firmware for a **Creality Pico MMU** (Multi‑Material Upgrade) controller. It runs on an ATmega328‑based board (Nano or Pro‑16 MHz) and communicates with the host printer via a serial USB interface. The firmware controls:

- 8 filament sensors (via MCP23X17 I/O expander)
- 2 servos – one for the MMU drive, one for the cutter
- 16‑LED NeoPixel bar (two 8‑LED strips)
- A buzzer for simple melodies

The code is written in C++ for the Arduino framework and is built with **PlatformIO**.

## 2. Build & Upload

```bash
# Build for the Nano board
platformio run --environment nanoatmega328

# Build for the Pro‑16 MHz board
platformio run --environment pro16MHzatmega328
```

Upload the firmware to the board:

```bash
platformio run --target upload --environment nanoatmega328
```

> **Tip** – The `platformio.ini` file already lists the required libraries:
>
> - `Servo`
> - `Adafruit NeoPixel`
> - `Adafruit MCP23017 Arduino Library`

## 3. Serial Monitor & Debugging

The firmware uses the Arduino `Serial` object for logging. The log format is:

```
[<millis>] <LEVEL> - <message><extra>
```

Levels: `INFO`, `WARN`, `ERROR`. Use the following command to view the log:

```bash
platformio device monitor --environment nanoatmega328
```

The monitor speed is set to **9600 baud** in `platformio.ini`.

### Common Log Messages

| Message | Meaning                         |
| ------- | ------------------------------- |
| `ALIVE` | Heartbeat sent by the firmware. |
| `OK`    | Command processed successfully. |
| `ERROR` | Command failed.                 |

## 4. Code Conventions

| Area                | Convention                                                                                                                                        |
| ------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------- |
| **Pin definitions** | All pins are defined at the top of `main.cpp` using `#define`. Keep them grouped by function (LED, buzzer, MMU, servos, sensors).                 |
| **Constants**       | Use `const int` or `const long` for fixed values. Musical notes are defined as `NOTE_A4`, `NOTE_B5`, etc.                                         |
| **LED handling**    | `changeLED(index, color)` updates a single LED. `blinkLED` and `blinkErrorLEDs` provide visual feedback.                                          |
| **MIDI**            | `playMIDI` plays a melody array. The function accepts a `bool ledEnabled` to optionally animate LEDs while playing.                               |
| **Serial commands** | The firmware responds to simple text commands (`OK`, `ERROR`, `ALIVE`). Implementations for other commands are in the omitted part of `main.cpp`. |
| **State variables** | Global state (e.g., `filamentStates`, `mmuServo`, `cutterServo`) is kept in the file scope for simplicity.                                        |

## 5. External Dependencies

- **Adafruit MCP23X17** – I/O expander used for filament sensors.
- **Adafruit NeoPixel** – Controls the 16‑LED bar.
- **Servo** – Drives the MMU and cutter servos.

All libraries are pulled automatically by PlatformIO via the `lib_deps` section.

## 6. Testing

The `test/` directory is set up for PlatformIO Unit Testing, but no tests are currently provided. To add tests, create a `test/test_main.cpp` file and use the PlatformIO Test Runner.

## 7. Common Tasks

| Task           | Command                          |
| -------------- | -------------------------------- |
| Clean build    | `platformio run --target clean`  |
| Flash firmware | `platformio run --target upload` |
| Monitor serial | `platformio device monitor`      |
| Run tests      | `platformio test`                |

---

**Feedback** – If any section is unclear or you need more detail on a specific part of the code, let me know and I’ll expand the instructions.
