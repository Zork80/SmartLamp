# SmartLamp

A smart, 3D-printable bedside lamp with a customizable lithophane shade. It features sleep and wake-up lights, various color themes, and is controlled by an ESP32 (or Arduino Nano).

The lamp offers a modern web interface for control and configuration and integrates seamlessly with Home Assistant via MQTT.

## Features

* **Web Interface:** Control all functions via a responsive web interface, accessible from any device on the network.
* **Dynamic Themes:** 15 built-in themes (e.g., Fire, Wave, Rainbow, Twinkle, Confetti, Sinelon, Police) plus custom color picker.
* **Wake-up & Sleep Timer (Dawn/Dusk):** Simulates a sunrise to wake up gently or a sunset to fall asleep.
* **Smart Wi-Fi Setup & Auto-Recovery:** Starts in Access Point mode if not configured, and automatically reconnects to home Wi-Fi when the router comes back online.
* **Flash Wear Protection:** Debounced EEPROM writes to protect the micro-controller flash memory against excessive write cycles.
* **Home Assistant Integration:** Automatic detection and integration into Home Assistant via MQTT Discovery.
* **Multiple Hardware Targets:** Support for ESP32, ESP32-C6, and Arduino Nano.

## Required Hardware

* 3D-printed parts (see below)
* ESP32 Dev Board (e.g., ESP32 DevKit, ESP32-C6 SuperMini) or Arduino Nano
* WS2812B / WS2811 RGB LED Strip or Ring (e.g. 12, 24, or 36 LEDs)
* 5x M2 self-tapping screws
* USB cable and power supply (5V)

## 3D Printing

The files for 3D printing and instructions can be found here: [SmartLamp on Printables](https://www.printables.com/model/237208-smartlamp)

## PlatformIO Environments

The project includes pre-configured environments in `platformio.ini`:

| Environment | Target Board | Description | Default Upload |
| :--- | :--- | :--- | :--- |
| **`[env:SmartLamp-24]`** | ESP32 DevKit | Standard lamp with 24 WS2812 LEDs | OTA |
| **`[env:SmartLamp-12]`** | ESP32 DevKit | Compact lamp with 12 WS2812 LEDs | OTA |
| **`[env:RoofLight]`** | ESP32 DevKit | Ceiling lamp with relay control, spotlight & PIR sensor | OTA |
| **`[env:SmartLamp-C6]`** | ESP32-C6 SuperMini | RISC-V ESP32-C6 dev board | USB (`/dev/ttyACM0`) |
| **`[env:SmartLamp-Nano]`** | Arduino Nano (ATmega328) | Offline version with physical push buttons | USB Serial |

## Build Flags & Customization

You can customize each environment in `platformio.ini` using `build_flags`:

* **`-D RESTORE_THEME_ON_BOOT`**: If set, the lamp restores its previous theme and brightness after a reboot. If omitted, the lamp powers up safely in the `Theme_Off` ("Aus") state. (Default active on `SmartLamp-Nano`).
* **`-D PIXELSPERSTAGE=xx`**: Number of LEDs in a single ring or stage (e.g. `12`, `24`, `36`).
* **`-D STAGES=x`**: Number of stacked stages/rings (e.g. `1` or `6`).
* **`-D LED_PIN=x`**: Output GPIO pin for the WS2812/WS2811 data line.
* **`-D LED_ORDER=GRB`**: Color order of the LEDs (`GRB`, `BRG`, `RGB`).
* **`-D HAS_WIFI`**: Enables Wi-Fi, WebServer, OTA, and MQTT functionality.
* **`-D HAS_BUTTONS`**: Enables physical push button controls (Pins 2 & 3).
* **`-D SKIP_THEME_OFF`**: When cycling through themes with physical buttons, skips the "Off" state.

## Software Installation

1. Open the project folder in VS Code with the PlatformIO extension installed.
2. Select the target environment (e.g., `env:SmartLamp-C6` or `env:SmartLamp-24`) from the bottom status bar.
3. Connect the board to your computer via USB.
4. Execute PlatformIO commands:
   * **`Upload`** (compiles and flashes the firmware).
   * **`Upload Filesystem Image`** (uploads the web interface files in `data/` to SPIFFS).

## Wi-Fi & Configuration

### 1. Initial Wi-Fi Setup
If no Wi-Fi credentials are saved, or if the network is unavailable at boot:
1. Connect your phone or computer to the Wi-Fi network:
   * **SSID:** `SmartLampConfig`
   * **Password:** `12345678`
2. Open your browser and navigate to `192.168.4.1`.
3. Enter your Wi-Fi SSID and password under "Network & Device Settings".
4. Click **Save and Reboot**.

> **Note on Router Reboots:** The firmware features automatic AP-mode recovery. If the router reboots or is temporarily unreachable, the lamp periodically checks the connection in the background and returns to normal Wi-Fi operation automatically.

### 2. Home Assistant Integration (MQTT)
1. In the SmartLamp web interface under "Network & Device Settings", configure the IP address of your MQTT broker.
2. Save and restart.
3. The lamp announces itself via MQTT Discovery and will automatically appear under **Settings -> Devices & Services -> MQTT** in Home Assistant with full RGB, brightness, effect, and timer controls.

## Stability & Architecture Details

* **Non-blocking NTP Synchronization:** `getLocalTime` runs with `0ms` timeout to eliminate 5-second main-loop freezes when waiting for network time.
* **Debounced Flash Persistence:** Changes to colors, brightness, or timers wait 2.5 seconds before committing to EEPROM/Flash, reducing flash sector wear by up to 95%.
* **MQTT Client Timeout Protection:** The TCP client timeout is bounded to 2 seconds to avoid freezing the animation loop if the broker is unreachable.
* **Robust Flash Validation:** Plausibility checks protect against out-of-bounds array reads and bootloops on uninitialized flash memory.

