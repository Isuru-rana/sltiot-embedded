# SLT IoT and Embedded Systems: practical code

**Live Greenhouse Control Wall:** https://isuru-rana.github.io/sltiot-embedded/

Everything runs online. No hardware, no installs: open a Wokwi link and press the green play button.

## Day 1: Embedded (inside the greenhouse controller)

| Build | What it shows | Open and run |
|---|---|---|
| E1 Digital I/O | Door switch, WATER button, debounce | https://wokwi.com/projects/479595805973381121 |
| E2 ADC | Soil moisture: raw, volts, percent | https://wokwi.com/projects/479595875908180993 |
| E3 PWM | Fan speed follows temperature | https://wokwi.com/projects/479595945188667393 |
| E4 Buses | I2C scan, OLED status screen, DHT22 | https://wokwi.com/projects/479596015335278593 |
| E5 Timing | delay() vs millis(): why a device goes deaf | https://wokwi.com/projects/479596085437845505 |
| E6 State machine | Full controller + watchdog (type `h`) | https://wokwi.com/projects/479596160729821185 |

All builds share one circuit (`wokwi/diagram.json`):

| Part | ESP32 pin |
|---|---|
| DHT22 (temp, humidity) | 15 |
| SOIL knob (moisture probe) | 34 |
| DOOR slide switch | 27 |
| WATER button | 14 |
| FAN LED (PWM) | 25 |
| PUMP LED | 26 |
| ALARM LED | 4 |
| OLED SSD1306 (I2C) | SDA 21, SCL 22 |

## The farm

- **Farm Simulator** (15 greenhouses, keep its tab visible): https://wokwi.com/projects/479596422394682369
- Wall: https://isuru-rana.github.io/sltiot-embedded/ (button "Simulated" hides the fake farms, or add `?sim=0`)
- Backup simulator if the Wokwi tab dies: `uv run backup/farm_sim.py`

## MQTT topics (public broker `broker.hivemq.com`)

| Topic | Direction | Content |
|---|---|---|
| `sltgh/<FARM>/telemetry` | device to wall | JSON: state, temp, hum, soil, fan, pump, door, alarms |
| `sltgh/<FARM>/status` | device to wall | `online` / `offline` (Last Will) |
| `sltgh/<FARM>/event` | device to wall | `NAME: HEAT RAISED` and similar |
| `sltgh/<FARM>/cmd` | wall to device | `WATER`, `FAN_MAX`, `AUTO` |
| `sltgh/region/cmd` | wall to all | `RAIN_ON`, `RAIN_OFF` |

Watch everything: HiveMQ web client https://www.hivemq.com/demos/websocket-client/ , host `broker.hivemq.com`, port `8884`, SSL on, subscribe `sltgh/#`.

The public broker has no passwords. Never send private data to it.
