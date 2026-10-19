@echo off
REM Drag samples.csv (or raw.jsonl) onto this file, or run: DRAW_MAP_WINDOWS.bat logs\scan_xxx\samples.csv
cd /d "%~dp0"
if "%~1"=="" (echo Drag a samples.csv onto this file & pause & exit /b)
python draw_map.py "%~1"
start "" "%~dp1map.svg"
pause
