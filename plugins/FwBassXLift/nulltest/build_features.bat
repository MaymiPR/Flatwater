@rem builds plugins/FwBassXLift/nulltest/features.dll (the Flatwater BassX Lift training feature helpers, train_net.py)
@echo off
for /f "usebackq delims=" %%i in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -property installationPath`) do call "%%i\VC\Auxiliary\Build\vcvars64.bat" >nul
cd /d %~dp0
cl /nologo /O2 /LD features.c /Fe:features.dll >nul
del features.exp features.lib features.obj 2>nul
