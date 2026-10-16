@echo off
setlocal
cd /d "%~dp0"
echo RoboCore S3 / Robot Driver with ESP32S3 Lite
echo Select ESP32-S3 / Factory / UART, then choose the correct COM port.
echo This launcher does not start flashing automatically.
start "" "flash_download_tool_3.9.9_R2.exe"
