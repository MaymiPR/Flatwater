@rem builds plugins/FwComp/nulltest/gain_fast4.dll: the release law, fourth form (fit_release4.py)
@echo off
for /f "usebackq delims=" %%i in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -property installationPath`) do call "%%i\VC\Auxiliary\Build\vcvars64.bat" >nul
cd /d %~dp0
cl /nologo /O2 /LD gain_fast4.c /Fe:gain_fast4.dll >nul
