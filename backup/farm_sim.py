# /// script
# dependencies = ["paho-mqtt>=2"]
# ///
"""BACKUP farm simulator (the main one runs in Wokwi: code/farm_simulator).

Same 15 greenhouses, same topics, same JSON, same scripted story.
Use it only if the Wokwi simulator tab dies.

    uv run farm_sim.py          # Ctrl+C to stop (publishes offline for all)
"""

import json
import math
import random
import signal
import time

import paho.mqtt.client as mqtt

BASE, DAY_S = "sltgh", 300
NAMES = ["NUWARAELIYA-03", "BANDARAWELA-07", "WELIMADA-11", "HAPUTALE-05", "DAMBULLA-02",
         "KANDAPOLA-04", "RAGALA-09", "BADULLA-12", "KEPPETIPOLA-06", "MATALE-08",
         "KANDY-15", "KURUNEGALA-10", "ANURADHAPURA-01", "JAFFNA-13", "HAMBANTOTA-14"]
HOT, THIRSTY, DOOR, BROKEN, FLAKY = range(5)
STATES = ["IDLE", "WATERING", "SOAKING", "ALARM"]

rng = random.Random(7)
farms = [dict(soil=rng.uniform(40, 80), use=rng.uniform(0.4, 1.2), base=rng.uniform(17, 22), hum=rng.randint(60, 85),
              rssi=rng.randint(-85, -55), state=0, at=0.0, mode="AUTO", force=False, offline=False, last="")
         for _ in NAMES]
farms[THIRSTY]["use"], farms[BROKEN]["soil"] = 2.5, 35
region = {"rain": False}


def on_message(client, userdata, msg):
    cmd, topic = msg.payload.decode(errors="ignore").strip(), msg.topic
    if topic.endswith("/region/cmd"):
        region["rain"] = cmd == "RAIN_ON"
        print("REGION", cmd)
        return
    name = topic.split("/")[1]
    if name in NAMES:
        f = farms[NAMES.index(name)]
        if cmd == "WATER":
            f["force"] = True
        elif cmd in ("FAN_MAX", "AUTO"):
            f["mode"] = cmd
        print("ORDER", name, cmd)


def step(i, phase, sun, client):
    f, now = farms[i], time.time()
    temp = f["base"] + 9 * sun + random.uniform(-0.3, 0.3)
    if i == HOT:
        temp += 14 * max(0.0, math.sin(math.pi * min(1, max(0, (phase - 0.05) / 0.35))))
    door = i == DOOR and 0.15 < phase < 0.30
    f["soil"] = max(0.0, f["soil"] - f["use"] * (0.5 + sun) * random.uniform(0.5, 1.5))
    hot = temp > 40
    s = f["state"]
    if s == 0:
        if hot:
            f["state"], f["at"] = 3, now
        elif (f["soil"] < 30 and not region["rain"]) or f["force"]:
            f["state"], f["at"], f["force"] = 1, now, False
    elif s == 1:
        if hot:
            f["state"], f["at"] = 3, now
        elif now - f["at"] > 10:
            f["state"], f["at"] = 2, now
    elif s == 2 and now - f["at"] > 15:
        f["state"], f["at"] = 0, now
    elif s == 3 and temp < 38:
        f["state"], f["at"] = 0, now
    if f["state"] == 1 and i != BROKEN:
        f["soil"] = min(100.0, f["soil"] + 9)
    fan = 100 if f["state"] == 3 or f["mode"] == "FAN_MAX" else (0 if door else int(min(100, max(0, (temp - 25) * 10))))
    alarms = []
    if hot:
        alarms.append("HEAT")
    if door:
        alarms.append("DOOR OPEN")
    if f["soil"] < 15:
        alarms.append("PUMP FAULT" if i == BROKEN else "SOIL DRY")
    alarms = " ".join(alarms)
    d = {"farm": NAMES[i], "state": STATES[f["state"]], "mode": f["mode"], "rain": region["rain"], "temp": round(temp, 1),
         "hum": f["hum"], "soil": int(f["soil"]), "fan": fan, "pump": "ON" if f["state"] == 1 else "OFF",
         "door": "OPEN" if door else "CLOSED", "alarms": alarms, "rssi": f["rssi"] + random.randint(-4, 4), "sim": True}
    client.publish(f"{BASE}/{NAMES[i]}/telemetry", json.dumps(d))
    for a in ("HEAT", "DOOR OPEN", "PUMP FAULT", "SOIL DRY"):
        if (a in alarms) != (a in f["last"]):
            client.publish(f"{BASE}/{NAMES[i]}/event", f"{NAMES[i]}: {a} {'RAISED' if a in alarms else 'cleared'}")
    f["last"] = alarms


def main():
    client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2, client_id=f"farmsim-py-{random.randint(0, 0xffff):x}")
    client.on_message = on_message
    client.connect("broker.hivemq.com", 1883, keepalive=30)
    client.subscribe(f"{BASE}/region/cmd")
    for n in NAMES:
        client.subscribe(f"{BASE}/{n}/cmd")
    client.loop_start()
    t0 = time.time()
    print(f"Farm simulator running: {len(NAMES)} greenhouses. Ctrl+C to stop.")
    try:
        while True:
            phase = ((time.time() - t0) % DAY_S) / DAY_S
            sun = max(0.0, math.sin(2 * math.pi * phase))
            fl = farms[FLAKY]
            off = 0.55 < phase < 0.75
            if off != fl["offline"]:
                fl["offline"] = off
                client.publish(f"{BASE}/{NAMES[FLAKY]}/status", "offline" if off else "online")
            for i in range(len(NAMES)):
                if not farms[i]["offline"]:
                    step(i, phase, sun, client)
                    time.sleep(0.2)
            time.sleep(2)
    except KeyboardInterrupt:
        pass
    finally:
        signal.signal(signal.SIGINT, signal.SIG_IGN)
        for n in NAMES:
            client.publish(f"{BASE}/{n}/status", "offline")
        time.sleep(1)
        client.loop_stop()


if __name__ == "__main__":
    main()
