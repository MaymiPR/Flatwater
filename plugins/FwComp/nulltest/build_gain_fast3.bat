@rem builds plugins/FwComp/nulltest/gain_fast3.dll: the release law, third form (fit_release3.py)
@echo off
for /f "usebackq delims=" %%i in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -property installationPath`) do call "%%i\VC\Auxiliary\Build\vcvars64.bat" >nul
cd /d %~dp0
cl /nologo /O2 /LD gain_fast3.c /Fe:gain_fast3.dll >nul
