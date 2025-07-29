# Copilot Instructions for Rack_Controller_Master

## Project Overview
- This project is a PlatformIO-based firmware for a rack controller system, using FreeRTOS and Arduino libraries on an ESP32 (or similar) microcontroller.
- The system manages sensors (temperature, humidity, smoke), door and fan states, and communicates with external systems via UART (Serial0, Serial1, Serial2) using JSON over SLIP and plain serial.
- Major components are in `src/` (main logic, handlers), `include/` (headers), and `lib/` (custom libraries).

## Architecture & Data Flow
- **Sensor Data**: Read in `sensor_handler.cpp` (e.g., DHT11 for temp/humidity, analog for smoke). Thresholds and settings are loaded from EEPROM.
- **Communication**:
  - **Electron App**: Communicates via Serial2 (UART) using JSON messages. See `electron_handler.cpp` for message structure and handshake logic.
  - **Slave Device**: Communicates via Serial1 using SLIP-encoded JSON. See `slave_handler.cpp` for message parsing, device control, and response patterns.
- **Settings**: Device settings (thresholds, rack ID, etc.) are settable via JSON commands and persisted to EEPROM.
- **Task Scheduling**: Uses FreeRTOS tasks for concurrent communication and sensor polling.

## Developer Workflows
- **Build/Upload**: Use PlatformIO (`pio run`, `pio upload`) for building and flashing firmware.
- **Serial Debugging**: Use PlatformIO Monitor or any serial terminal on the relevant UART port (default baud rates: 9600 or 115200).
- **Testing**: No automated tests found; manual testing via serial commands and hardware interaction is standard.

## Project-Specific Patterns
- **JSON Handling**: Uses ArduinoJson. For small, fixed messages use `StaticJsonDocument`; for variable/large, use `DynamicJsonDocument`.
- **SLIP Protocol**: All inter-device messages on Serial1 are SLIP-encoded. Use `slip_send_message` and `slip_process_byte`.
- **EEPROM Defaults**: When reading settings, check for uninitialized values (e.g., `0xFFFF` for `uint16_t`) and set defaults if needed.
- **Password Handling**: Passwords are compared as integers; no hashing or encryption by default.
- **Device State**: Door, fan, and sensor states are tracked in global arrays/variables and updated on message receipt.

## Integration Points
- **External App**: Electron app expects JSON messages with specific keys (see `sendDataToElectron()` in `electron_handler.cpp`).
- **Slave Device**: Expects and sends SLIP-encoded JSON with keys like `type`, `status`, etc. (see `recv_message()` in `slave_handler.cpp`).

## Examples
- To send a new temperature threshold: `{ "type": "device_settings_set", "temperature_threshold": 30 }`
- To trigger door open: `{ "type": "door_status", "front": 1, "back": 0, ... }`

## Key Files
- `src/electron_handler.cpp`: Electron app UART protocol, JSON message structure
- `src/slave_handler.cpp`: SLIP protocol, device command handling, EEPROM logic
- `src/sensor_handler.cpp`: Sensor polling, emergency checks, threshold logic
- `include/`: All public headers for cross-file access

---

If any section is unclear or missing important project knowledge, please provide feedback to improve these instructions.
