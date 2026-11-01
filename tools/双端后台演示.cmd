@echo off
chcp 65001 >nul
cd /d "%~dp0"
python --version >nul 2>&1
if errorlevel 1 (
  echo 需要安装 Python 3 才能运行后台演示。普通游戏启动不需要 Python。
  pause
  exit /b 1
)
python -X utf8 demo.py
if errorlevel 1 (
  echo 演示未完成，请保留此处错误和 logs 目录。
  pause
  exit /b 1
)
echo 双端已进入正常游戏，DemoB 正在观战 DemoA。
pause
