# 🚀 QUICK START GUIDE - Train Your Pro-Level 2v2 Bot

## 📥 Step 1: Get the Training Files

1. Create the `training/` folder in your project:
```bash
cd C:\Users\Jake\Videos\Jake\GigaLearnCPP-Leak-main
mkdir training
```

2. Download and place these 3 files in `training/`:
   - `config.yaml`
   - `rewards.py`
   - `train.py`

3. Place `setup_training.bat` in your project root

## ⚙️ Step 2: Run Setup

Double-click `setup_training.bat` or run:
```bash
setup_training.bat
```

This will:
- ✅ Check Python 3.11
- ✅ Check CUDA/GPU
- ✅ Install dependencies (PyTorch, NumPy, TensorBoard)
- ✅ Create necessary folders
- ✅ Verify GigaLearnBot.exe exists

## 🎮 Step 3: (Optional) Setup RocketSimVis

**Want to watch training in 3D?**

1. Download from: https://github.com/ZealanL/RocketSimVis/releases
2. Extract anywhere
3. Run `RocketSimVis.exe` BEFORE starting training

**Don't want visualization?**
- Edit `training/config.yaml`
- Set `visualization: enabled: false`

## 🚀 Step 4: START TRAINING!

```bash
cd C:\Users\Jake\Videos\Jake\GigaLearnCPP-Leak-main
python training\train.py
```

That's it! Training will start from Stage 1.

---

## 📊 What Happens During Training

### Stage 1: Ball Contact (4-8 hours)
- Bot learns to touch the ball
- Learns basic movement
- Collects boost

### Stage 2: Goal Shooting (8-16 hours)  
- Bot learns to shoot towards goal
- Basic accuracy training

### Stage 3: Power & Accuracy (1-2 days)
- Power shots and dodges
- Corner shots
- Dribbling basics

### Stage 4: Aerial Fundamentals (2-3 days)
- Fast aerials
- Aerial interception
- Aerial goals

### Stage 5: Air Dribbles (3-4 days)
- Consecutive aerial touches
- Air roll control
- Air dribble goals

### Stage 6: Double Taps (3-4 days)
- Backboard reads
- Wall aerials
- Double tap goals

### Stage 7: Pro 2v2 Game Sense (4-7 days)
- Rotation and positioning
- Passing and assists
- Defensive play
- Team coordination

**Total Training Time: ~3-4 weeks on GPU**

---

## 💾 Checkpoints

Models are automatically saved:
- Every 50M steps: `training/models/stage1_ballcontact_50M.pt`
- Stage completion: `training/models/stage1_ballcontact_final.pt`

Resume training:
```bash
python training\train.py --resume
```

Start from specific stage:
```bash
python training\train.py --stage 4
```

---

## 📈 Monitor Training

### Option 1: Console Output
Watch the terminal - stats update every 1000 steps

### Option 2: TensorBoard
```bash
cd training\logs\tensorboard
tensorboard --logdir .
# Open: http://localhost:6006
```

### Option 3: RocketSimVis (if enabled)
Real-time 3D visualization of training

---

## ⚠️ Common Issues

### "CUDA not available"
- Your GPU isn't detected
- Check CUDA 12.8 is installed
- Training will be VERY slow on CPU (months instead of weeks)

### "Cannot find GigaLearnBot.exe"
- Make sure you compiled the project first
- Run in Visual Studio: Build → Build Solution

### "Module not found"
- Run `setup_training.bat` again
- Manually install: `pip install torch pyyaml numpy`

### Training is very slow
- Disable visualization: `visualization: enabled: false` in config.yaml
- Reduce `num_workers` in config.yaml
- Check GPU is being used (should show in startup log)

---

## 🎯 Expected Results

After full training (3-4 weeks), your bot will:
- ✅ Hit SSL/GC3+ level in 1v1
- ✅ Perform fast aerials consistently
- ✅ Execute air dribbles and double taps
- ✅ Understand 2v2 rotation and positioning
- ✅ Play at professional level

---

## 📝 Advanced Options

### Adjust Training Speed
Edit `config.yaml`:
- Reduce `timesteps` for faster (but worse) training
- Increase `batch_size` if you have GPU memory
- Adjust `learning_rate` for different learning speeds

### Customize Rewards
Edit `training/rewards.py`:
- Change reward weights
- Add custom mechanics
- Adjust success criteria

### Change Architecture
Edit `config.yaml` → `ppo` section:
- Modify `layer_sizes` for bigger/smaller networks
- Change activation functions
- Adjust training hyperparameters

---

## 🆘 Need Help?

1. Check logs: `training/logs/`
2. Watch TensorBoard graphs
3. Verify collision_meshes are in place
4. Ensure Python 3.11 (not 3.12 or 3.10)

---

## 🎉 After Training

Your final model: `training/models/stage7_pro_2v2_gamesense_final.pt`

Deploy it:
1. Load model in RLBot framework
2. Play in actual Rocket League
3. Join online matches!

---

**Ready? Let's train!** 🚀

```bash
python training\train.py
```
