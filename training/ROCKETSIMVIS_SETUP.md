# RocketSimVis Setup Guide

RocketSimVis lets you watch your bot train in real-time 3D visualization.

## Download & Install

### Option 1: Pre-built Binary (Easiest)
1. Go to: https://github.com/ZealanL/RocketSimVis/releases
2. Download latest release for Windows
3. Extract to `C:\RocketSimVis\` (or anywhere you want)

### Option 2: Build from Source
```bash
git clone https://github.com/ZealanL/RocketSimVis.git
cd RocketSimVis
# Follow build instructions in their README
```

## Running RocketSimVis

### Step 1: Start RocketSimVis
```bash
# Navigate to RocketSimVis folder
cd C:\RocketSimVis

# Run the executable
RocketSimVis.exe
```

### Step 2: Configure Connection
- RocketSimVis listens on `localhost:7777` by default
- Your training script will connect automatically

### Step 3: Start Training
```bash
# In a separate terminal, run your training
cd C:\Users\Jake\Videos\Jake\GigaLearnCPP-Leak-main
python training\train.py
```

## What You'll See

RocketSimVis will show:
- ✅ Real-time 3D view of the arena
- ✅ Bot(s) playing Rocket League
- ✅ Ball trajectory
- ✅ Boost pads
- ✅ Goal detection
- ✅ Car orientation and actions

## Controls

- **Mouse**: Rotate camera
- **WASD**: Move camera
- **Scroll**: Zoom in/out
- **Space**: Pause/Resume
- **R**: Reset view

## Troubleshooting

### "Connection Failed"
- Make sure RocketSimVis is running BEFORE you start training
- Check firewall isn't blocking port 7777
- In config.yaml, verify: `visualization: enabled: true`

### "No visual updates"
- Check the update_frequency in config.yaml
- Try setting it to 10 for more frequent updates

### Performance Issues
- Visualization uses extra CPU/GPU
- If training is slow, set `visualization: enabled: false`
- You can always watch replays later

## Optional: Disable Visualization

If you don't want visualization:
1. Edit `training/config.yaml`
2. Set `visualization: enabled: false`
3. Training will run faster without it

## Alternative: Use Logs Only

You can monitor training without RocketSimVis:

```bash
# Watch TensorBoard logs
cd training/logs/tensorboard
tensorboard --logdir .
# Open browser to http://localhost:6006
```
