@rem builds plugins/FwVocalChain/nulltest/deess.dll (the de-esser model for fit_deess.py)
@echo off
for /f "usebackq delims=" %%i in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -property installationPath`) do call "%%i\VC\Auxiliary\Build\vcvars64.bat" >nul
cd /d %~dp0
cl /nologo /O2 /LD deess.c /Fe:deess.dll >nul
