@echo off
REM GigaLearnCPP Training Setup Script
REM This script prepares your environment for training

echo ====================================
echo GigaLearnCPP Training Setup
echo ====================================

REM Check Python installation
echo.
echo [1/6] Checking Python installation...
python --version
if errorlevel 1 (
    echo ERROR: Python not found!
    echo Please install Python 3.11 first.
    pause
    exit /b 1
)

REM Check CUDA
echo.
echo [2/6] Checking CUDA installation...
nvcc --version
if errorlevel 1 (
    echo WARNING: CUDA compiler not found.
    echo Training will be SLOW on CPU!
    echo Press any key to continue anyway...
    pause
)

REM Install Python dependencies
echo.
echo [3/6] Installing Python dependencies...
pip install --break-system-packages torch torchvision torchaudio --index-url https://download.pytorch.org/whl/cu121
pip install --break-system-packages pyyaml numpy tensorboard

REM Create training directories
echo.
echo [4/6] Creating training directories...
if not exist "training" mkdir training
if not exist "training\models" mkdir training\models
if not exist "training\logs" mkdir training\logs
if not exist "training\logs\tensorboard" mkdir training\logs\tensorboard

REM Copy configuration files
echo.
echo [5/6] Setting up configuration files...
REM Files already created - just verify they exist
if not exist "training\config.yaml" (
    echo ERROR: config.yaml not found!
    echo Please make sure all training files are in the training\ folder
    pause
    exit /b 1
)

REM Verify GigaLearnBot.exe exists
echo.
echo [6/6] Verifying GigaLearnBot.exe...
if exist "out\build\x64-RelWithDebInfo\RelWithDebInfo\GigaLearnBot.exe" (
    echo ✓ GigaLearnBot.exe found!
) else (
    echo ERROR: GigaLearnBot.exe not found!
    echo Please compile the project first in Visual Studio
    pause
    exit /b 1
)

echo.
echo ====================================
echo ✓ Setup Complete!
echo ====================================
echo.
echo Next steps:
echo 1. Make sure collision_meshes are in place
echo 2. Optionally: Start RocketSimVis for visualization
echo 3. Run: python training\train.py
echo.
echo Press any key to exit...
pause >nul
