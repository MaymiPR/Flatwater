@rem builds plugins/FwComp/nulltest/gain_fast5.dll: the release law, fifth form (fit_release4.py)
@echo off
for /f "usebackq delims=" %%i in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -property installationPath`) do call "%%i\VC\Auxiliary\Build\vcvars64.bat" >nul
cd /d %~dp0
cl /nologo /O2 /LD gain_fast5.c /Fe:gain_fast5.dll >nul
