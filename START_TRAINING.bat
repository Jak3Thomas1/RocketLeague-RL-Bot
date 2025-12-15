@echo off
REM ============================================================================
REM GigaLearnCPP Master Launcher
REM Starts training and metrics automatically in separate windows
REM ============================================================================

echo ========================================
echo GigaLearnCPP Master Launcher
echo ========================================
echo.
echo Starting training bot and metrics viewer...
echo.

REM Set Python path
set PYTHONPATH=C:\Users\Jake\Videos\Jake\GigaLearnCPP-Leak-main

REM Navigate to executable location
cd C:\Users\Jake\Videos\Jake\GigaLearnCPP-Leak-main\out\build\x64-RelWithDebInfo\RelWithDebInfo

REM Start metrics receiver in new window
echo Starting metrics viewer...
start "GigaLearn Metrics" cmd /k "cd python_scripts && python metric_receiver.py"

REM Wait 2 seconds for metrics to initialize
timeout /t 2 /nobreak >nul

REM Start training bot in this window
echo Starting training bot...
echo.
GigaLearnBot.exe

REM When training exits, close metrics window
taskkill /FI "WINDOWTITLE eq GigaLearn Metrics*" /F >nul 2>&1

echo.
echo Training session ended.
pause
