@echo off
cd /d "%~dp0"
py -m uvicorn main_pair_live:app --host 0.0.0.0 --port 8001
pause
