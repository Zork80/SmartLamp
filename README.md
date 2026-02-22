# SmartLamp

This is the project for the SmartLamp controller.

## Configuration

If no Wi-Fi credentials are saved, the SmartLamp will automatically start in Access Point (AP) mode.
Connect to the lamp's Wi-Fi network (**SSID:** `SmartLampConfig`, **Password:** `12345678`) and configure your SSID and password conveniently via the web interface.

## Installation

This project is based on PlatformIO.

1. Open the project in an IDE with PlatformIO support (e.g., VS Code).
2. Connect the microcontroller via USB.
3. Execute the `Upload` command to compile and flash the firmware.
4. Execute the `Upload Filesystem Image` command to upload the website files to the microcontroller.

## Required Parts

* 3D-printed parts
* ESP32 Dev board
* WS2812 RGB LED Ring (12 oder 24 LEDs)
* 5x M4 x 6 mm screws
* 3x wires to connect the LED ring (soldering on the ring, Dupont connectors on the ESP32)
* 1 USB-Cable

## 3D Printing

The files for 3D printing can be downloaded here: [SmartLamp on Printables](https://www.printables.com/model/237208-smartlamp)

## Home Assistant Integration

The firmware supports MQTT and automatic detection by Home Assistant (MQTT Discovery).

**Prerequisites:**
1.  A running MQTT broker (e.g., the "Mosquitto broker" add-on in Home Assistant).
2.  The MQTT integration must be configured in Home Assistant.

**Setup:**
1.  Enter the IP address of your MQTT broker in the SmartLamp's web interface under "Network & Device Settings".
2.  Save the settings and restart the lamp.
3.  The lamp should now automatically appear as a new device under **Settings -> Devices & Services -> MQTT** in Home Assistant and is immediately controllable.

No manual configuration in `configuration.yaml` is necessary.
