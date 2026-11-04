# SLT IoT and Embedded Systems: practical code

**Live Greenhouse Control Wall:** https://isuru-rana.github.io/sltiot-embedded/

**MQTT Notifier** (beep + browser notification for any topic): https://isuru-rana.github.io/sltiot-embedded/notify/

Everything runs online. No hardware, no installs: open a Wokwi link and press the green play button.

## Day 1: Embedded (inside the greenhouse controller)

What we built in class, in order:

| Step | What it shows | Open and run |
|---|---|---|
| 1. Digital | Switch on pin 14 turns the LED on pin 18 on and off | https://wokwi.com/projects/479648555380147201 |
| 2. Analog | Knob on pin 34 sets LED brightness on pin 18 (`pot * 255 / 4095`) | https://wokwi.com/projects/479656776007799809 |
| 3. DHT22 | Read temperature and humidity (pin 15) | https://wokwi.com/projects/479658635297239041 |
| 4. OLED | Print text on the SSD1306 screen (SDA 21, SCL 22) | https://wokwi.com/projects/479660226991136769 |
| 5. Simple device | Whole greenhouse with only if/else: fan by temp, pump by soil or button, alarm by door or heat, OLED | https://wokwi.com/projects/479662728307194881 |
| 6. State machine | IDLE, WATERING, ALARM with only if/else and HIGH/LOW | https://wokwi.com/projects/479663430209541121 |

Code: steps 1 to 5 in `code/basics/`, step 6 in `code/e6_simple_state/`.

More Day 1 builds (one topic each, for practice at home):

| Build | What it shows | Open and run |
|---|---|---|
| E1 Digital I/O | Door switch, WATER button, debounce | https://wokwi.com/projects/479595805973381121 |
| E2 ADC | Soil moisture: raw, volts, percent | https://wokwi.com/projects/479595875908180993 |
| E3 PWM | Fan speed follows temperature | https://wokwi.com/projects/479595945188667393 |
| E4 Buses | I2C scan, OLED status screen, DHT22 | https://wokwi.com/projects/479596015335278593 |

Start of Day 2: timing and reliability

| Build | What it shows | Open and run |
|---|---|---|
| Millis on button | Press the button: prints millis() in seconds and ms on the Serial Monitor | https://wokwi.com/projects/479737697353143297 |
| E5 Timing | delay() vs millis(): why a device goes deaf | https://wokwi.com/projects/479596085437845505 |
| SOAKING with delay | Water 5 s, soak 10 s with delay(): heat alarm is missed while it waits | https://wokwi.com/projects/479735800185034753 |
| SOAKING with millis | Same device with millis(): heat alarm comes at once | https://wokwi.com/projects/479735818218448897 |
| SOAKING delay + ntfy | SOAKING with delay, sends each state change to your phone (ntfy) | https://wokwi.com/projects/479754729751775233 |
| SOAKING millis + ntfy | SOAKING with millis, sends each state change to your phone (ntfy) | https://wokwi.com/projects/479754805859546113 |
| SOAKING delay + MQTT | SOAKING with delay, publishes each state change to `sltiot/change-me/state` | https://wokwi.com/projects/479754868729033729 |
| SOAKING millis + MQTT | SOAKING with millis, publishes each state change to `sltiot/change-me/state` | https://wokwi.com/projects/479754920742609921 |
| SOAKING millis + ntfy + MQTT | SOAKING with millis, each state change goes to ntfy and MQTT at the same time | https://wokwi.com/projects/479754977386706945 |
| Interrupt basic | Loop is busy blinking with delay(). POLL button is missed, INTERRUPT button lights its LED at once | https://wokwi.com/projects/479735998133173249 |
| E6 with timers | Adds SOAKING with a timer + watchdog (type `h`) | https://wokwi.com/projects/479596160729821185 |

The greenhouse builds (E1 to E6, SOAKING) share one circuit (`wokwi/diagram.json`). Millis on button, Interrupt basic, WiFi basic, the HTTP and MQTT examples have their own small circuits.

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

## Day 2: IoT (the greenhouse goes online)

| Build | What it shows | Open and run |
|---|---|---|
| WiFi basic | Connect to WiFi (`Wokwi-GUEST`, no password) and print the IP address | https://wokwi.com/projects/479741033012992001 |
| HTTP My IP | Ask the internet for your public IP (api.ipify.org) | https://wokwi.com/projects/479742038793952257 |
| HTTP Time | Get the time now in Colombo from the internet (wttr.in) | https://wokwi.com/projects/479742056680576001 |
| HTTP Weather | Get the weather now in Colombo (wttr.in) | https://wokwi.com/projects/479742075011788801 |
| ntfy basic | Phone notification every 5 s (10 messages). Change `topic`, subscribe in the ntfy app | https://wokwi.com/projects/479744623651231745 |
| MQTT send | Send "Hello from ESP32" every 5 s to `sltiot/change-me/hello`. Watch it in the HiveMQ web client | https://wokwi.com/projects/479744908267372545 |
| MQTT LED | Send `1` or `0` to `sltiot/change-me/led` from the HiveMQ web client: LED turns on or off | https://wokwi.com/projects/479744943344349185 |
| I1 Publish | WiFi + MQTT: one JSON message every 5 s | https://wokwi.com/projects/479597790735339521 |
| I2 Online greenhouse | E6 + telemetry, Last Will, remote orders (`useTLS = true` = encrypted) | https://wokwi.com/projects/479597904048686081 |
| I3 Phone alerts | I2 + ntfy push when an alarm starts or clears | https://wokwi.com/projects/479598475249100801 |
| I4 ThingSpeak | HTTP to a cloud platform with free charts (needs your Write API Key) | https://wokwi.com/projects/479598545659951105 |
| I5 Deep sleep | Battery field sensor: wake, send, sleep | https://wokwi.com/projects/479598016110044161 |

Change `farmId` to your own name (place + number, e.g. `KANDY-21`) and your greenhouse appears on the wall.
In I3 change `ntfyTopic` to a hard-to-guess name and subscribe to it in the free ntfy app.
I5 note: Wokwi restarts the chip instead of a true deep sleep, so its wake counter stays at 1 there. On real hardware it counts up.

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
