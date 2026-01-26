#!/usr/bin/env python3
"""
GPS Tracking Server - Minimalist Dashboard
Style inspired by door-sensor.w1mx.scripts.mit.edu
Run: python3 gps_server.py
Open: http://localhost:5002
"""

from flask import Flask, request, jsonify, render_template_string
from datetime import datetime
import json

app = Flask(__name__)

# Store GPS data
gps_data = {
    'lat': 0.0,
    'lon': 0.0,
    'speed': 0.0,
    'altitude': 0.0,
    'satellites': 0,
    'peak_speed': 0.0,
    'avg_speed': 0.0,
    'trip_distance': 0.0,
    'last_update': None,
    'history': []  # List of {time, speed, lat, lon}
}

HTML_TEMPLATE = '''
<!DOCTYPE html>
<html>
<head>
    <title>GPS Tracker</title>
    <meta http-equiv="refresh" content="2">
    <style>
        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }
        body {
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
            background: #1a1a1a;
            color: #fff;
            min-height: 100vh;
            padding: 40px;
        }
        .container {
            max-width: 800px;
            margin: 0 auto;
        }
        h1 {
            font-size: 24px;
            font-weight: 400;
            color: #888;
            margin-bottom: 20px;
        }
        .status {
            margin-bottom: 40px;
        }
        .speed-display {
            font-size: 120px;
            font-weight: 700;
            line-height: 1;
            margin-bottom: 10px;
        }
        .speed-unit {
            font-size: 36px;
            font-weight: 400;
            color: #888;
            margin-left: 10px;
        }
        .green { color: #8dc73f; }
        .yellow { color: #f0c040; }
        .red { color: #e74c3c; }
        .gray { color: #666; }

        .timestamp {
            font-size: 14px;
            color: #666;
            margin-bottom: 30px;
        }
        .stats {
            display: grid;
            grid-template-columns: repeat(3, 1fr);
            gap: 30px;
            margin-bottom: 40px;
        }
        .stat {
            background: #252525;
            padding: 20px;
            border-radius: 8px;
        }
        .stat-label {
            font-size: 12px;
            color: #888;
            text-transform: uppercase;
            letter-spacing: 1px;
            margin-bottom: 8px;
        }
        .stat-value {
            font-size: 32px;
            font-weight: 600;
        }
        .stat-unit {
            font-size: 14px;
            color: #666;
            margin-left: 5px;
        }
        .location {
            background: #252525;
            padding: 20px;
            border-radius: 8px;
            margin-bottom: 40px;
        }
        .location-title {
            font-size: 12px;
            color: #888;
            text-transform: uppercase;
            letter-spacing: 1px;
            margin-bottom: 15px;
        }
        .coords {
            font-family: 'Monaco', 'Consolas', monospace;
            font-size: 18px;
            color: #8dc73f;
        }
        .altitude {
            margin-top: 10px;
            font-size: 16px;
            color: #f0c040;
        }
        .satellites {
            margin-top: 10px;
            font-size: 14px;
            color: #666;
        }
        .chart-section {
            margin-top: 40px;
        }
        .chart-title {
            font-size: 14px;
            color: #888;
            margin-bottom: 15px;
        }
        .chart {
            background: #252525;
            border-radius: 8px;
            padding: 20px;
            height: 150px;
            display: flex;
            align-items: flex-end;
            gap: 2px;
        }
        .bar {
            flex: 1;
            background: #8dc73f;
            min-height: 2px;
            border-radius: 2px 2px 0 0;
            transition: height 0.3s;
        }
        .bar.low { background: #8dc73f; }
        .bar.medium { background: #f0c040; }
        .bar.high { background: #e74c3c; }

        .no-data {
            color: #666;
            font-style: italic;
        }
        .gps-status {
            display: inline-block;
            padding: 5px 12px;
            border-radius: 20px;
            font-size: 12px;
            font-weight: 600;
            text-transform: uppercase;
            margin-bottom: 20px;
        }
        .gps-status.connected { background: #2d5a1d; color: #8dc73f; }
        .gps-status.disconnected { background: #5a1d1d; color: #e74c3c; }
    </style>
</head>
<body>
    <div class="container">
        <h1>GPS Tracker</h1>

        {% if data.last_update %}
            {% set age = (now - data.last_update).total_seconds() %}
            {% if age < 10 %}
                <div class="gps-status connected">● Live</div>
            {% else %}
                <div class="gps-status disconnected">● Offline</div>
            {% endif %}
        {% else %}
            <div class="gps-status disconnected">● Waiting for data</div>
        {% endif %}

        <div class="status">
            {% set speed = data.speed %}
            {% if speed < 30 %}
                <div class="speed-display green">{{ "%.0f"|format(speed) }}<span class="speed-unit">MPH</span></div>
            {% elif speed < 60 %}
                <div class="speed-display yellow">{{ "%.0f"|format(speed) }}<span class="speed-unit">MPH</span></div>
            {% else %}
                <div class="speed-display red">{{ "%.0f"|format(speed) }}<span class="speed-unit">MPH</span></div>
            {% endif %}
        </div>

        <div class="timestamp">
            {% if data.last_update %}
                Last update: {{ data.last_update.strftime('%I:%M:%S %p') }}
            {% else %}
                No data received yet
            {% endif %}
        </div>

        <div class="stats">
            <div class="stat">
                <div class="stat-label">Peak Speed</div>
                <div class="stat-value red">{{ "%.1f"|format(data.peak_speed) }}<span class="stat-unit">MPH</span></div>
            </div>
            <div class="stat">
                <div class="stat-label">Average</div>
                <div class="stat-value" style="color: #3498db;">{{ "%.1f"|format(data.avg_speed) }}<span class="stat-unit">MPH</span></div>
            </div>
            <div class="stat">
                <div class="stat-label">Trip Distance</div>
                <div class="stat-value" style="color: #9b59b6;">{{ "%.2f"|format(data.trip_distance) }}<span class="stat-unit">mi</span></div>
            </div>
        </div>

        <div class="location">
            <div class="location-title">Current Location</div>
            {% if data.lat != 0 or data.lon != 0 %}
                <div class="coords">{{ "%.6f"|format(data.lat) }}, {{ "%.6f"|format(data.lon) }}</div>
                <div class="altitude">↑ {{ "%.0f"|format(data.altitude * 3.28084) }} ft</div>
                <div class="satellites">{{ data.satellites }} satellites</div>
            {% else %}
                <div class="no-data">Waiting for GPS fix...</div>
            {% endif %}
        </div>

        <div class="chart-section">
            <div class="chart-title">Speed History (last 60 readings)</div>
            <div class="chart">
                {% if data.history %}
                    {% for point in data.history[-60:] %}
                        {% set h = (point.speed / 100 * 100) if point.speed < 100 else 100 %}
                        {% if point.speed < 30 %}
                            <div class="bar low" style="height: {{ h }}%;" title="{{ point.speed }} MPH"></div>
                        {% elif point.speed < 60 %}
                            <div class="bar medium" style="height: {{ h }}%;" title="{{ point.speed }} MPH"></div>
                        {% else %}
                            <div class="bar high" style="height: {{ h }}%;" title="{{ point.speed }} MPH"></div>
                        {% endif %}
                    {% endfor %}
                {% else %}
                    <div class="no-data" style="margin: auto;">No history yet</div>
                {% endif %}
            </div>
        </div>
    </div>
</body>
</html>
'''

@app.route('/')
def index():
    return render_template_string(HTML_TEMPLATE, data=gps_data, now=datetime.now())

@app.route('/api/gps/update')
def update_gps():
    """Receive GPS data from Wio Terminal"""
    gps_data['lat'] = float(request.args.get('lat', 0))
    gps_data['lon'] = float(request.args.get('lon', 0))
    gps_data['speed'] = float(request.args.get('speed', 0))
    gps_data['altitude'] = float(request.args.get('altitude', 0))
    gps_data['satellites'] = int(request.args.get('satellites', 0))
    gps_data['last_update'] = datetime.now()

    # Update peak speed
    if gps_data['speed'] > gps_data['peak_speed']:
        gps_data['peak_speed'] = gps_data['speed']

    # Update average (simple running average)
    if gps_data['speed'] > 0.5:
        history_speeds = [p['speed'] for p in gps_data['history'] if p['speed'] > 0.5]
        history_speeds.append(gps_data['speed'])
        gps_data['avg_speed'] = sum(history_speeds) / len(history_speeds)

    # Add to history
    gps_data['history'].append({
        'time': datetime.now().isoformat(),
        'speed': gps_data['speed'],
        'lat': gps_data['lat'],
        'lon': gps_data['lon']
    })

    # Keep only last 500 readings
    if len(gps_data['history']) > 500:
        gps_data['history'] = gps_data['history'][-500:]

    print(f"GPS Update: {gps_data['speed']:.1f} MPH @ ({gps_data['lat']:.6f}, {gps_data['lon']:.6f})")

    return jsonify({'status': 'ok'})

@app.route('/api/gps/data')
def get_data():
    """Get current GPS data as JSON"""
    return jsonify({
        'lat': gps_data['lat'],
        'lon': gps_data['lon'],
        'speed': gps_data['speed'],
        'altitude': gps_data['altitude'],
        'satellites': gps_data['satellites'],
        'peak_speed': gps_data['peak_speed'],
        'avg_speed': gps_data['avg_speed'],
        'last_update': gps_data['last_update'].isoformat() if gps_data['last_update'] else None
    })

@app.route('/api/reset')
def reset():
    """Reset all stats"""
    gps_data['peak_speed'] = 0.0
    gps_data['avg_speed'] = 0.0
    gps_data['trip_distance'] = 0.0
    gps_data['history'] = []
    return jsonify({'status': 'reset'})

if __name__ == '__main__':
    print("=" * 50)
    print("GPS Tracking Server")
    print("=" * 50)
    print(f"Dashboard: http://localhost:5002")
    print(f"API endpoint: http://localhost:5002/api/gps/update")
    print("=" * 50)
    app.run(host='0.0.0.0', port=5002, debug=True)
