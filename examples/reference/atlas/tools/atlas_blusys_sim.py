#!/usr/bin/env python3
"""
Wire-level simulator for the blusys `atlas_capability`.

Connects MQTTS to the Atlas EMQX broker using deviceId+mqttToken, subscribes
to atlas/{id}/down/{command,state,ota}, publishes the same JSON shapes the
firmware emits (initial retained state, heartbeat, event/ack). Prints every
lifecycle event so we can correlate against Atlas logs.

Usage:
    python3 atlas_blusys_sim.py --device-id <uuid> --mqtt-token <uuid> \
        [--broker localhost] [--port 8883] [--ca-cert /path/ca.crt]
"""
from __future__ import annotations

import argparse
import json
import os
import signal
import ssl
import sys
import time
from dataclasses import dataclass, field
from threading import Event

import paho.mqtt.client as mqtt

FIRMWARE_VERSION = "1.0.0-blusys"
HEARTBEAT_S = 5
STATE_S = 20


@dataclass
class SimState:
    phase: str = "idle"
    broker_connected: bool = False
    heartbeats_sent: int = 0
    states_published: int = 0
    commands_handled: int = 0
    last_log: dict = field(default_factory=dict)


def log(tag: str, **fields):
    ts = time.strftime("%H:%M:%S")
    parts = [f"{k}={v}" for k, v in fields.items()]
    print(f"[{ts}] [{tag}] {' '.join(parts)}", flush=True)


def make_client(device_id: str, token: str, ca_cert: str) -> mqtt.Client:
    c = mqtt.Client(
        callback_api_version=mqtt.CallbackAPIVersion.VERSION2,
        client_id=device_id,
        protocol=mqtt.MQTTv311,
    )
    c.username_pw_set(device_id, token)
    # Mirror the ESP-IDF mqtt stack: verify chain against the Atlas dev CA,
    # but skip hostname verification (cert is for 'localhost') and relax
    # X.509 strict checks since the dev-generated CA doesn't carry the
    # keyUsage extension that Python's ssl now enforces by default.
    ctx = ssl.create_default_context(cafile=ca_cert)
    ctx.check_hostname = False
    ctx.verify_mode    = ssl.CERT_REQUIRED
    if hasattr(ssl, "VERIFY_X509_STRICT"):
        ctx.verify_flags &= ~ssl.VERIFY_X509_STRICT
    c.tls_set_context(ctx)
    # LWT — what the backend sees when we disappear.
    lwt_topic = f"atlas/{device_id}/up/state"
    c.will_set(lwt_topic, json.dumps({"online": False}), qos=1, retain=True)
    return c


def main() -> int:
    p = argparse.ArgumentParser()
    p.add_argument("--device-id", required=True)
    p.add_argument("--mqtt-token", required=True)
    p.add_argument("--broker", default="localhost")
    p.add_argument("--port", type=int, default=8883)
    p.add_argument("--ca-cert", default="/home/oguzkaganozt/codebase/atlas/infra/emqx/certs/ca.crt")
    p.add_argument("--run-seconds", type=int, default=0,
                   help="If >0, exit cleanly after this many seconds.")
    args = p.parse_args()

    state = SimState()
    stop = Event()

    up_state      = f"atlas/{args.device_id}/up/state"
    up_heartbeat = f"atlas/{args.device_id}/up/heartbeat"
    up_event      = f"atlas/{args.device_id}/up/event"
    down_command = f"atlas/{args.device_id}/down/command"
    down_state    = f"atlas/{args.device_id}/down/state"
    down_ota      = f"atlas/{args.device_id}/down/ota"

    client = make_client(args.device_id, args.mqtt_token, args.ca_cert)

    def on_connect(c, userdata, flags, reason_code, properties=None):
        log("broker", event="connected", rc=str(reason_code))
        state.broker_connected = True
        state.phase = "online"
        # Subscribe to everything the firmware subscribes to.
        for t in (down_command, down_state, down_ota):
            c.subscribe(t, qos=1)
            log("subscribe", topic=t)
        # Initial retained state — matches atlas_capability::poll() on first connect.
        payload = json.dumps({"online": True, "firmwareVersion": FIRMWARE_VERSION})
        c.publish(up_state, payload, qos=1, retain=True)
        log("publish", topic=up_state, payload=payload, retain=True)
        state.states_published += 1

    def on_disconnect(c, userdata, flags, reason_code, properties=None):
        log("broker", event="disconnected", rc=str(reason_code))
        state.broker_connected = False
        state.phase = "connecting_broker"

    def on_message(c, userdata, msg):
        payload = msg.payload.decode("utf-8", errors="replace")
        log("recv", topic=msg.topic, qos=msg.qos, payload=payload)
        state.last_log[msg.topic] = payload

        if msg.topic == down_command:
            state.commands_handled += 1
            try:
                cmd = json.loads(payload)
                ack = {
                    "type": "command_ack",
                    "timestamp": int(time.time() * 1000),
                    "commandId": cmd.get("commandId"),
                    "success": True,
                    "details": "sim-ok",
                }
                c.publish(up_event, json.dumps(ack), qos=1)
                log("publish", topic=up_event, payload=json.dumps(ack))
            except Exception as e:  # noqa: BLE001
                log("error", parsing_cmd=str(e))
        elif msg.topic == down_state:
            pass
        elif msg.topic == down_ota:
            ann = json.loads(payload) if payload else {}
            log("ota", announcement=json.dumps(ann))

    client.on_connect    = on_connect
    client.on_disconnect = on_disconnect
    client.on_message    = on_message

    def handle_signal(signum, frame):
        log("sim", event="shutdown", signal=signum)
        stop.set()
    signal.signal(signal.SIGINT,  handle_signal)
    signal.signal(signal.SIGTERM, handle_signal)

    log("sim", broker=args.broker, port=args.port, device=args.device_id)
    client.connect(args.broker, args.port, keepalive=30)
    client.loop_start()

    started_ms = time.time()
    last_heartbeat = 0.0
    last_state     = 0.0
    try:
        while not stop.is_set():
            now = time.time()
            if args.run_seconds and (now - started_ms) >= args.run_seconds:
                log("sim", event="run_seconds_reached")
                break
            if state.broker_connected and (now - last_heartbeat) >= HEARTBEAT_S:
                last_heartbeat = now
                hb = {
                    "online": True,
                    "firmwareVersion": FIRMWARE_VERSION,
                    "freeHeap": 180000,
                    "rssi": -55,
                    "uptimeMs": int((now - started_ms) * 1000),
                }
                client.publish(up_heartbeat, json.dumps(hb), qos=1)
                state.heartbeats_sent += 1
                log("publish", topic=up_heartbeat, payload=json.dumps(hb))
            if state.broker_connected and (now - last_state) >= STATE_S:
                last_state = now
                st = {
                    "online": True,
                    "firmwareVersion": FIRMWARE_VERSION,
                    "freeHeap": 180000,
                    "rssi": -55,
                    "phase": state.phase,
                    "commandsHandled": state.commands_handled,
                    "heartbeatsSent": state.heartbeats_sent,
                }
                client.publish(up_state, json.dumps(st), qos=1, retain=True)
                state.states_published += 1
                log("publish", topic=up_state, payload=json.dumps(st), retain=True)
            time.sleep(0.25)
    finally:
        log("sim", event="stopping",
            heartbeats=state.heartbeats_sent,
            states=state.states_published,
            commands=state.commands_handled)
        client.loop_stop()
        # Clean disconnect — backend should mark offline via EMQX webhook.
        client.disconnect()
    return 0


if __name__ == "__main__":
    sys.exit(main())
