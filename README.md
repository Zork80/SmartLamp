# SmartLamp

Dies ist das Projekt für die SmartLamp Steuerung.

## Konfiguration

Falls keine WLAN-Zugangsdaten hinterlegt sind, startet die SmartLamp automatisch im Access-Point-Modus (AP-Modus).
Verbinde dich mit dem WLAN der Lampe (**SSID:** `SmartLampConfig`, **Passwort:** `12345678`) und konfiguriere SSID und Passwort bequem über die Weboberfläche.

## Installation

Dieses Projekt basiert auf PlatformIO.

1. Öffne das Projekt in einer IDE mit PlatformIO-Unterstützung (z. B. VS Code).
2. Verbinde den Mikrocontroller per USB.
3. Führe den Befehl `Upload` aus, um die Firmware zu kompilieren und zu flashen.
4. Führe den Befehl `Upload Filesystem Image` aus, um die Webseiten-Dateien auf den Mikrocontroller zu laden.

## Benötigte Teile

* 3D-gedruckte Teile
* ESP32 Dev Board
* WS2812 RGB LED Ring (12 oder 24 LEDs)
* 5x M4 x 6 mm Schrauben
* 3x Kabel zum Anschluss des LED-Rings (Löten am Ring, Dupont-Stecker am ESP32)
* 1 USB-Kabel

## 3D-Druck

Die Dateien für den 3D-Druck können hier heruntergeladen werden: [SmartLamp auf Printables](https://www.printables.com/model/237208-smartlamp)

## Home Assistant Integration

Die Firmware unterstützt MQTT und die automatische Erkennung durch Home Assistant (MQTT Discovery).

**Voraussetzungen:**
1.  Ein laufender MQTT-Broker (z.B. das "Mosquitto broker" Add-on in Home Assistant).
2.  Die MQTT-Integration muss in Home Assistant konfiguriert sein.

**Einrichtung:**
1.  Trage die IP-Adresse deines MQTT-Brokers in der Weboberfläche der SmartLamp unter "Network & Device Settings" ein.
2.  Speichere die Einstellung und starte die Lampe neu.
3.  Die Lampe sollte nun automatisch als neues Gerät unter **Einstellungen -> Geräte & Dienste -> MQTT** in Home Assistant erscheinen und ist sofort steuerbar.

Es ist keine manuelle Konfiguration in der `configuration.yaml` notwendig.
