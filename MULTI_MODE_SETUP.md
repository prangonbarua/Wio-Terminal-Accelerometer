# Multi-Mode Wio Terminal Setup Guide

## 🎵🚗 Speedometer + Billboard Hot 100 Display

Your Wio Terminal now has TWO modes you can switch between:

**Mode 1: GPS Speedometer** - Real-time speed tracking
**Mode 2: Billboard Hot 100** - Current #1 song with album cover

---

## 📦 What You Need:

### Hardware:
1. ✅ Wio Terminal (you have this)
2. 🔲 NEO-6M GPS Module (ordered - $13)
3. 🔲 3.5" TFT Display (ordered - $13-15)
4. 🔲 Jumper Wires (ordered - $6)

### Software:
- Arduino IDE with Wio Terminal board support
- WiFi network credentials

---

## 🔧 Setup Steps:

### Step 1: Update WiFi Credentials

Open `wio_multi_mode.ino` and update these lines:

```cpp
const char* ssid = "YOUR_WIFI_SSID";        // Your WiFi name
const char* password = "YOUR_WIFI_PASSWORD"; // Your WiFi password
```

### Step 2: Deploy Your Flask App (if not already)

Your Billboard Hot 100 Flask app needs to be running online so the Wio Terminal can access it.

**Option A: Railway (Recommended)**
- Already deployed at your Railway URL
- Use that URL in the code

**Option B: Ngrok (Local testing)**
```bash
cd ~/Documents/GitHub/Billboard-Hot-100-Website
python3 app.py
# In another terminal:
ngrok http 5001
```

### Step 3: Update API URL in Code

Find this line in `wio_multi_mode.ino`:

```cpp
String url = "http://YOUR_FLASK_APP_URL/api/hot100/current";
```

Replace with your actual URL:
```cpp
String url = "https://your-app.railway.app/api/hot100/current";
```

### Step 4: Install Required Arduino Libraries

In Arduino IDE, go to **Sketch → Include Library → Manage Libraries**

Install:
- ✅ `Seeed Arduino LIS3DH` (already installed)
- ✅ `Seeed_Arduino_LCD` (built-in with Wio Terminal)
- 🔲 `ArduinoJson` by Benoit Blanchon (NEW - for parsing API response)
- 🔲 `HTTPClient` (usually built-in with ESP32/SAMD)

### Step 5: Upload Code

1. Open `wio_multi_mode.ino` in Arduino IDE
2. Select **Tools → Board → Seeeduino Wio Terminal**
3. Select correct port
4. Click **Upload**

---

## 🎮 How to Use:

### Switching Modes:
**Press the TOP button** (Button A) to switch between:
- Speedometer Mode
- Billboard Hot 100 Mode

### Speedometer Mode:
- Shows current speed (MPH)
- Shows peak speed
- Updates in real-time based on accelerometer

### Billboard Mode:
- Shows current #1 Billboard Hot 100 song
- Displays song name and artist
- Album cover placeholder (ready for image display)
- Updates from your Flask app

---

## 📱 Testing the API:

Test if your Flask API is working:

1. Make sure your Flask app is running
2. Visit in browser: `https://your-app-url.com/api/hot100/current`
3. You should see JSON like:
```json
{
  "song": "Espresso",
  "artist": "Sabrina Carpenter",
  "weeks_at_1": 3,
  "image_url": "...",
  "date": "2024-01-17"
}
```

---

## 🔮 Future Enhancements (When GPS Arrives):

### GPS Integration:
- Replace accelerometer speed with actual GPS speed
- More accurate speedometer
- Can add distance tracking
- Trip recording

### Album Cover Display:
- Download album cover image over WiFi
- Display on 3.5" external screen
- Full color album art

### Additional Modes:
- Weather display
- Clock/Date
- Trip computer
- Music player controls

---

## 🐛 Troubleshooting:

### WiFi Not Connecting:
- Check SSID and password are correct
- Make sure Wio Terminal is in range
- Wio Terminal only supports 2.4GHz WiFi (not 5GHz)

### Billboard Data Not Loading:
- Check Flask app is running and accessible
- Test API endpoint in browser
- Check Serial Monitor for error messages

### Button Not Working:
- Make sure you're pressing the TOP button (Button A)
- Check Serial Monitor to see mode switch messages

### Display Issues:
- Make sure backlight is on
- Check TFT_eSPI_BACKUP library is deleted
- Use built-in Seeed_Arduino_LCD library

---

## 📝 Next Steps:

**When your GPS module arrives:**
1. I'll create updated code with GPS integration
2. Wire GPS to Wio Terminal (4 wires)
3. Get real accurate speed readings

**When your 3.5" display arrives:**
1. I'll create dual-display code
2. Big speed numbers on external display
3. Album cover art display

**Ready to test?** Upload the code and let me know how it works!
