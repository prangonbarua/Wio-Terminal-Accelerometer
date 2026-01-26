#!/usr/bin/env python3
"""
GPS Tracking Server - Minimalist Dashboard
Style: door-sensor.w1mx.scripts.mit.edu
Run: python3 gps_server.py
Open: http://localhost:5002
"""

from flask import Flask, request, jsonify, render_template_string
from datetime import datetime

app = Flask(__name__)

# Store GPS data
gps_data = {
    'speed': 0.0,
    'peak_speed': 0.0,
    'lat': 0.0,
    'lon': 0.0,
    'altitude': 0.0,
    'satellites': 0,
    'last_update': None,
    'history': []
}

HTML_TEMPLATE = '''
<!DOCTYPE html>
<html>
<head>
    <title>GPS Speed</title>
    <meta http-equiv="refresh" content="2">
    <style>
        body {
            font-family: Arial, Helvetica, sans-serif;
            background: #fff;
            color: #333;
            margin: 40px;
        }
        h1 {
            font-size: 18px;
            font-weight: normal;
            color: #666;
            margin-bottom: 10px;
        }
        .speed {
            font-size: 180px;
            font-weight: bold;
            line-height: 1;
        }
        .speed.moving { color: #8dc73f; }
        .speed.stopped { color: #999; }
        .unit {
            font-size: 48px;
            color: #999;
            margin-left: 10px;
        }
        .timestamp {
            font-size: 14px;
            color: #999;
            margin-top: 20px;
        }
        .section {
            margin-top: 40px;
            padding-top: 20px;
            border-top: 1px solid #eee;
        }
        .section-title {
            font-size: 14px;
            color: #999;
            margin-bottom: 15px;
        }
        .chart {
            height: 80px;
            display: flex;
            align-items: flex-end;
            gap: 2px;
            background: #f9f9f9;
            padding: 10px;
        }
        .bar {
            flex: 1;
            background: #8dc73f;
            min-width: 3px;
        }
        .bar.stopped { background: #ddd; }
        .stats {
            display: flex;
            gap: 60px;
            margin-top: 30px;
        }
        .stat-label {
            font-size: 12px;
            color: #999;
            text-transform: uppercase;
        }
        .stat-value {
            font-size: 36px;
            font-weight: bold;
            color: #333;
        }
        .stat-value.peak { color: #e74c3c; }
        .coords {
            font-family: monospace;
            font-size: 14px;
            color: #666;
            margin-top: 20px;
        }
    </style>
</head>
<body>
    <h1>GPS Speed</h1>

    {% if data.speed > 0.5 %}
        <div class="speed moving">{{ "%.0f"|format(data.speed) }}<span class="unit">MPH</span></div>
    {% else %}
        <div class="speed stopped">0<span class="unit">MPH</span></div>
    {% endif %}

    <div class="timestamp">
        {% if data.last_update %}
            {{ data.last_update.strftime('%I:%M %p') }} on {{ data.last_update.strftime('%B %d, %Y') }}
        {% else %}
            Waiting for data...
        {% endif %}
    </div>

    <div class="stats">
        <div>
            <div class="stat-label">Peak</div>
            <div class="stat-value peak">{{ "%.0f"|format(data.peak_speed) }}</div>
        </div>
        <div>
            <div class="stat-label">Altitude</div>
            <div class="stat-value">{{ "%.0f"|format(data.altitude * 3.28084) }} ft</div>
        </div>
        <div>
            <div class="stat-label">Satellites</div>
            <div class="stat-value">{{ data.satellites }}</div>
        </div>
    </div>

    <div class="section">
        <div class="section-title">Speed history</div>
        <div class="chart">
            {% for point in data.history[-100:] %}
                {% set h = (point.speed / 80 * 100) if point.speed < 80 else 100 %}
                <div class="bar {% if point.speed < 1 %}stopped{% endif %}" style="height: {{ h }}%;"></div>
            {% endfor %}
        </div>
    </div>

    {% if data.lat != 0 %}
    <div class="coords">
        {{ "%.6f"|format(data.lat) }}, {{ "%.6f"|format(data.lon) }}
    </div>
    {% endif %}
</body>
</html>
'''

@app.route('/')
def index():
    return render_template_string(HTML_TEMPLATE, data=gps_data)

@app.route('/api/gps/update')
def update_gps():
    gps_data['lat'] = float(request.args.get('lat', 0))
    gps_data['lon'] = float(request.args.get('lon', 0))
    gps_data['speed'] = float(request.args.get('speed', 0))
    gps_data['altitude'] = float(request.args.get('altitude', 0))
    gps_data['satellites'] = int(request.args.get('satellites', 0))
    gps_data['last_update'] = datetime.now()

    if gps_data['speed'] > gps_data['peak_speed']:
        gps_data['peak_speed'] = gps_data['speed']

    gps_data['history'].append({'speed': gps_data['speed']})
    if len(gps_data['history']) > 500:
        gps_data['history'] = gps_data['history'][-500:]

    print(f"Speed: {gps_data['speed']:.0f} MPH")
    return jsonify({'status': 'ok'})

@app.route('/api/reset')
def reset():
    gps_data['peak_speed'] = 0.0
    gps_data['history'] = []
    return jsonify({'status': 'reset'})

if __name__ == '__main__':
    print("=" * 40)
    print("GPS Server")
    print("Open: http://localhost:5002")
    print("=" * 40)
    app.run(host='0.0.0.0', port=5002, debug=True)
