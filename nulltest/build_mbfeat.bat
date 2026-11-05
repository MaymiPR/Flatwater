@rem builds nulltest/mbfeat.dll (the Flatwater BassX Lift training feature helpers, train_fwbassxlift.py)
@echo off
for /f "usebackq delims=" %%i in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -property installationPath`) do call "%%i\VC\Auxiliary\Build\vcvars64.bat" >nul
cd /d %~dp0
cl /nologo /O2 /LD mbfeat.c /Fe:mbfeat.dll >nul
del mbfeat.exp mbfeat.lib mbfeat.obj 2>nul
