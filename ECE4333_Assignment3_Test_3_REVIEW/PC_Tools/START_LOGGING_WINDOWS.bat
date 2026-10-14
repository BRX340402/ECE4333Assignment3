@echo off
cd /d "%~dp0"
python a3_wifi_logger.py --seconds 120 --out run01
pause
