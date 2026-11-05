@rem builds nulltest/fastdsp.dll (attack/release follower used by fwbasslift_model.py; optional, a Python fallback exists)
@echo off
for /f "usebackq delims=" %%i in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -property installationPath`) do call "%%i\VC\Auxiliary\Build\vcvars64.bat" >nul
cd /d %~dp0
cl /nologo /O2 /LD fastdsp.c /Fe:fastdsp.dll >nul
