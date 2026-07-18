#!/usr/bin/env python3
"""
Industrial Protocol Bridge - Web Dashboard
Displays real-time sensor data from the bridge engine.
"""

from flask import Flask, jsonify, render_template_string
import random
import time
from datetime import datetime

app = Flask(__name__)

# Simulated sensor data (in production: read from shared memory or MQTT)
sensors = {
    "Motortemperatur": {
        "value": 0.0, "unit": "°C",
        "threshold": 80.0, "alarm": False,
        "history": []
    },
    "Hydraulikdruck": {
        "value": 0.0, "unit": "bar",
        "threshold": 8.0, "alarm": False,
        "history": []
    },
    "Luftfeuchtigkeit": {
        "value": 0.0, "unit": "%",
        "threshold": 90.0, "alarm": False,
        "history": []
    },
    "Versorgungsspannung": {
        "value": 0.0, "unit": "V",
        "threshold": 28.0, "alarm": False,
        "history": []
    }
}

# Bridge status
bridge_status = {
    "state": "RUNNING",
    "poll_count": 0,
    "alarm_count": 0,
    "mqtt_sent": 0,
    "start_time": datetime.now().isoformat()
}

def update_sensors():
    """Simulate sensor readings (same logic as bridge_engine.c)"""
    bridge_status["poll_count"] += 1
    any_alarm = False

    for name, sensor in sensors.items():
        if name == "Motortemperatur":
            sensor["value"] = round(65.0 + random.uniform(-10, 15), 1)
        elif name == "Hydraulikdruck":
            sensor["value"] = round(5.0 + random.uniform(-2, 3), 1)
        elif name == "Luftfeuchtigkeit":
            sensor["value"] = round(60.0 + random.uniform(-15, 30), 1)
        elif name == "Versorgungsspannung":
            sensor["value"] = round(24.0 + random.uniform(-1, 1), 1)

        sensor["alarm"] = sensor["value"] > sensor["threshold"]
        if sensor["alarm"]:
            any_alarm = True
            bridge_status["alarm_count"] += 1

        # Keep last 20 readings for history
        sensor["history"].append({
            "time": datetime.now().strftime("%H:%M:%S"),
            "value": sensor["value"]
        })
        if len(sensor["history"]) > 20:
            sensor["history"].pop(0)

        bridge_status["mqtt_sent"] += 1

    bridge_status["state"] = "ALARM" if any_alarm else "RUNNING"

# HTML Template
DASHBOARD_HTML = """
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="utf-8">
    <meta http-equiv="refresh" content="3">
    <title>Industrial Bridge Dashboard</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body {
            font-family: Arial, sans-serif;
            background: #1a1a2e;
            color: #eee;
            padding: 20px;
        }
        h1 {
            text-align: center;
            color: #0abde3;
            margin-bottom: 10px;
            font-size: 22px;
        }
        .subtitle {
            text-align: center;
            color: #666;
            margin-bottom: 20px;
            font-size: 13px;
        }
        .status-bar {
            display: flex;
            justify-content: center;
            gap: 30px;
            margin-bottom: 20px;
            padding: 12px;
            background: #16213e;
            border-radius: 8px;
        }
        .status-item {
            text-align: center;
            font-size: 13px;
        }
        .status-value {
            font-size: 18px;
            font-weight: bold;
            color: #0abde3;
        }
        .status-value.alarm { color: #ee5a24; }
        .sensors {
            display: grid;
            grid-template-columns: 1fr 1fr;
            gap: 15px;
            max-width: 800px;
            margin: 0 auto;
        }
        .sensor-card {
            background: #16213e;
            border-radius: 10px;
            padding: 18px;
            border-left: 4px solid #0abde3;
        }
        .sensor-card.alarm {
            border-left-color: #ee5a24;
            background: #2d1f1f;
        }
        .sensor-name {
            font-size: 13px;
            color: #888;
            margin-bottom: 8px;
        }
        .sensor-value {
            font-size: 32px;
            font-weight: bold;
            color: #0abde3;
        }
        .sensor-card.alarm .sensor-value { color: #ee5a24; }
        .sensor-unit {
            font-size: 16px;
            color: #666;
            margin-left: 4px;
        }
        .sensor-threshold {
            font-size: 12px;
            color: #555;
            margin-top: 8px;
        }
        .sensor-status {
            display: inline-block;
            padding: 3px 10px;
            border-radius: 12px;
            font-size: 11px;
            font-weight: bold;
            margin-top: 8px;
        }
        .sensor-status.ok {
            background: #0a3d2e;
            color: #2ecc71;
        }
        .sensor-status.alarm {
            background: #3d0a0a;
            color: #ee5a24;
        }
        .history {
            margin-top: 10px;
            font-size: 11px;
            color: #555;
        }
        footer {
            text-align: center;
            margin-top: 25px;
            color: #444;
            font-size: 11px;
        }
    </style>
</head>
<body>
    <h1>Industrial Protocol Bridge</h1>
    <p class="subtitle">Modbus TCP → MQTT | Real-time Monitoring</p>

    <div class="status-bar">
        <div class="status-item">
            <div>State</div>
            <div class="status-value {{ 'alarm' if status.state == 'ALARM' else '' }}">
                {{ status.state }}
            </div>
        </div>
        <div class="status-item">
            <div>Polls</div>
            <div class="status-value">{{ status.poll_count }}</div>
        </div>
        <div class="status-item">
            <div>Alarms</div>
            <div class="status-value {{ 'alarm' if status.alarm_count > 0 else '' }}">
                {{ status.alarm_count }}
            </div>
        </div>
        <div class="status-item">
            <div>MQTT Sent</div>
            <div class="status-value">{{ status.mqtt_sent }}</div>
        </div>
    </div>

    <div class="sensors">
        {% for name, sensor in sensors.items() %}
        <div class="sensor-card {{ 'alarm' if sensor.alarm else '' }}">
            <div class="sensor-name">{{ name }}</div>
            <div>
                <span class="sensor-value">{{ sensor.value }}</span>
                <span class="sensor-unit">{{ sensor.unit }}</span>
            </div>
            <div class="sensor-threshold">
                Threshold: {{ sensor.threshold }} {{ sensor.unit }}
            </div>
            <span class="sensor-status {{ 'alarm' if sensor.alarm else 'ok' }}">
                {{ 'ALARM' if sensor.alarm else 'OK' }}
            </span>
            {% if sensor.history %}
            <div class="history">
                Last: {{ sensor.history[-1].time }}
            </div>
            {% endif %}
        </div>
        {% endfor %}
    </div>

    <footer>
        Industrial Protocol Bridge v1.0 | Steve Meka
    </footer>
</body>
</html>
"""

@app.route("/")
def dashboard():
    update_sensors()
    return render_template_string(DASHBOARD_HTML,
                                  sensors=sensors,
                                  status=bridge_status)

@app.route("/api/sensors")
def api_sensors():
    update_sensors()
    return jsonify({
        "sensors": {name: {
            "value": s["value"],
            "unit": s["unit"],
            "alarm": s["alarm"],
            "threshold": s["threshold"]
        } for name, s in sensors.items()},
        "status": bridge_status
    })

@app.route("/api/health")
def api_health():
    return jsonify({"status": "ok", "uptime": bridge_status["poll_count"]})

if __name__ == "__main__":
    print("========================================")
    print("  Industrial Bridge Dashboard v1.0")
    print("  http://localhost:5000")
    print("========================================")
    app.run(host="0.0.0.0", port=5000, debug=False)
