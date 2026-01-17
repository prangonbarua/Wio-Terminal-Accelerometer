# Wio Terminal Accelerometer

A real-time acceleration measurement tool for the Seeed Wio Terminal, featuring auto-calibration, noise filtering, and peak acceleration tracking.

## Features

- Real-time acceleration measurement in m/s²
- Peak acceleration tracking
- Auto-calibration on startup
- Noise filtering to reduce sensor jitter
- Smoothing filter for stable readings
- Visual display on Wio Terminal LCD
- Serial monitor output for debugging

## Hardware Requirements

- [Seeed Wio Terminal](https://www.seeedstudio.com/Wio-Terminal-p-4509.html)
- USB-C cable for programming and power

## Software Requirements

- [Arduino IDE](https://www.arduino.cc/en/software) (v1.8.0 or later)
- Required Arduino Libraries:
  - `Seeed_Arduino_LIS3DHTR` (Accelerometer library)
  - `TFT_eSPI` (Display library)

## Installation

### 1. Install Arduino IDE

Download and install the Arduino IDE from the [official website](https://www.arduino.cc/en/software).

### 2. Add Wio Terminal Board Support

1. Open Arduino IDE
2. Go to **File > Preferences**
3. Add the following URL to **Additional Boards Manager URLs**:
   ```
   https://files.seeedstudio.com/arduino/package_seeedstudio_index.json
   ```
4. Go to **Tools > Board > Boards Manager**
5. Search for "Wio Terminal" and install **Seeed SAMD Boards**

### 3. Install Required Libraries

1. Go to **Sketch > Include Library > Manage Libraries**
2. Search and install:
   - `Seeed Arduino LIS3DH` by Seeed Studio
   - `TFT_eSPI` by Bodmer

### 4. Upload the Code

1. Open `wio_terminal_accelerometer.ino` in Arduino IDE
2. Connect your Wio Terminal via USB-C
3. Select **Tools > Board > Seeeduino Wio Terminal**
4. Select the correct port under **Tools > Port**
5. Click **Upload**

## Usage

1. Power on the Wio Terminal
2. Keep the device still on a flat surface during the calibration phase (1.5 seconds)
3. Once calibrated, the screen will display "READY!"
4. Move or shake the device to measure acceleration
5. The display shows:
   - **Current Accel**: Real-time acceleration
   - **Peak Accel**: Highest acceleration recorded
6. Press the **RESET button** to clear the peak value and recalibrate

## Display Information

```
┌─────────────────────────────────┐
│ Current Accel:                  │
│ 2.45 m/s²                       │
│                                 │
│ Peak Accel:                     │
│ 5.89 m/s²                       │
│                                 │
│ Press RESET button to clear     │
└─────────────────────────────────┘
```

## Technical Details

### Acceleration Measurement

- **Sensor**: LIS3DHTR 3-axis accelerometer
- **Range**: ±2g
- **Sample Rate**: 100Hz
- **Update Rate**: 20Hz (50ms delay)
- **Noise Threshold**: 0.5 m/s²

### Filtering

- **Smoothing**: Moving average filter (10 samples)
- **Calibration**: 100-sample average during startup
- **Gravity Compensation**: Automatically removes gravity component

### Color Coding

- **Cyan**: Current acceleration label
- **Green**: Current acceleration value
- **Yellow**: Peak acceleration label
- **Red**: Peak acceleration value

## Troubleshooting

### "Accelerometer Error!" message

- Check that the Wio Terminal is genuine and has the LIS3DHTR sensor
- Try resetting the device
- Check library installations

### Inaccurate readings

- Ensure device is kept completely still during calibration
- Recalibrate by pressing the RESET button
- Check that the device is on a level surface during calibration

### Upload issues

- Make sure you've selected the correct board and port
- Try sliding the power switch to ON position
- Press the reset button twice quickly to enter bootloader mode

## Serial Monitor Output

Enable the Serial Monitor at 115200 baud to see debug output:
```
Calibration complete:
  Offset X: 0.0123
  Offset Y: -0.0045
  Offset Z: 0.0089
Current: 2.45 m/s² | Peak: 5.89 m/s²
```

## License

This project is open source and available under the MIT License.

## Contributing

Contributions are welcome! Feel free to submit issues or pull requests.

## Acknowledgments

- Seeed Studio for the Wio Terminal platform
- Arduino community for libraries and support
