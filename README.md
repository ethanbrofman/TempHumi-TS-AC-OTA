# TempHumi-TS-AC-OTA
## Temperature/Humidity Sensor based on ESP8266, with ThingSpeak integration, Wifi AutoConnect, and OTA updates.

Utilizing an AI-THINKER ESP-12, an AM-2303 DHT22 sensor, a 5V->3V voltage regulator, and a USB charger, this unit features the following software:

- ThingSpeak API integration at periodic intervals
- Wifi AutoConnect (https://github.com/tzapu/WiFiManager) provides smart Wifi management (Auto reconnect, AP mode config)
- OTA updates (https://github.com/jandrassy/ArduinoOTA) provides easy OTA updates of firmware
