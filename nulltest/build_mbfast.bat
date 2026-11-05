@rem builds nulltest/mbfast.dll: the Flatwater BassX Lift generator loop and its exact linearisation (mbmodel.py, extract_g3.py, train_fwbassxlift.py)
@echo off
for /f "usebackq delims=" %%i in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -property installationPath`) do call "%%i\VC\Auxiliary\Build\vcvars64.bat" >nul
cd /d %~dp0
cl /nologo /O2 /LD mbfast.c /Fe:mbfast.dll >nul
