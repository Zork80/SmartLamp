# SmartLamp

A smart, 3D-printable bedside lamp with a customizable lithophane shade. It features sleep and wake-up lights, various color themes, and is controlled by an ESP32.

The lamp offers a modern web interface for control and configuration and integrates seamlessly with Home Assistant.

## Features

*   **Web Interface:** Control all functions via a responsive web interface, accessible from any device on the network.
*   **Dynamic Themes:** A variety of built-in light themes as well as custom color selection.
*   **Wake-up & Sleep Timer (Dawn/Dusk):** Simulates a sunrise to wake up gently or a sunset to fall asleep.
*   **Easy Wi-Fi Setup:** If no network is configured, the lamp starts in Access Point mode for easy initial setup.
*   **Home Assistant Integration:** Automatic detection and integration into Home Assistant via MQTT for advanced control and automation.
*   **PlatformIO Project:** Easy to compile and upload using Visual Studio Code and the PlatformIO extension.

## Required Hardware

*   3D-printed parts (see below)
*   ESP32 Dev Board
*   WS2812B RGB LED Ring (designed for 24 LEDs)
*   5x M2 self-tapping screws
*   Micro-USB cable and power supply

## 3D Printing

The files for 3D printing and instructions can be found here: [SmartLamp on Printables](https://www.printables.com/model/237208-smartlamp)

## Software Installation

This project is based on PlatformIO.

1.  Open the project folder in VS Code with the PlatformIO extension installed.
2.  Connect the ESP32 to your computer via USB.
3.  Use PlatformIO commands to:
    *   **`Upload`** (compiles and flashes the main firmware).
    *   **`Upload Filesystem Image`** (uploads the web interface files).

## Configuration

### 1. Initial Wi-Fi Setup

If no Wi-Fi credentials are saved, the lamp starts an Access Point (AP).

1.  Connect your phone or computer to the Wi-Fi network:
    *   **SSID:** `SmartLampConfig`
    *   **Password:** `12345678`
2.  A captive portal should open automatically, or navigate to `192.168.4.1` in your browser.
3.  Enter your Wi-Fi SSID and password in the web interface under "Network & Device Settings".
4.  Save and reboot. The lamp will then connect to your network.

### 2. Home Assistant Integration

The firmware supports MQTT and automatic discovery by Home Assistant (MQTT Discovery).

**Prerequisites:**
1.  A running MQTT broker (e.g., the "Mosquitto broker" add-on in Home Assistant).
2.  The MQTT integration must be configured in Home Assistant.

**Setup:**
1.  Enter the IP address of your MQTT broker in the SmartLamp web interface under "Network & Device Settings".
2.  Save the settings and restart the lamp.
3.  The lamp should now automatically appear as a new device under **Settings -> Devices & Services -> MQTT** in Home Assistant and is immediately controllable.

No manual configuration in `configuration.yaml` is necessary.
