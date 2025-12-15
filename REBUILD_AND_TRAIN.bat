@echo off
REM ============================================================================
REM GigaLearnCPP - Rebuild and Train
REM Rebuilds the project and starts training automatically
REM ============================================================================

echo ========================================
echo GigaLearnCPP - Rebuild and Train
echo ========================================
echo.

REM Navigate to build directory
cd C:\Users\Jake\Videos\Jake\GigaLearnCPP-Leak-main\out\build\x64-RelWithDebInfo

echo [1/2] Rebuilding project...
cmake --build . --config RelWithDebInfo -j 6

if errorlevel 1 (
    echo.
    echo Build FAILED! Check errors above.
    pause
    exit /b 1
)

echo.
echo [2/2] Build succeeded! Starting training...
echo.

REM Navigate to executable
cd RelWithDebInfo

REM Start training
GigaLearnBot.exe

echo.
echo Training session ended.
pause
