@rem builds plugins/FwComp/nulltest/gain_fast.dll: H-Comp's identified gain path per sample (model.py, ident_hc_*.py)
@echo off
for /f "usebackq delims=" %%i in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -property installationPath`) do call "%%i\VC\Auxiliary\Build\vcvars64.bat" >nul
cd /d %~dp0
cl /nologo /O2 /LD gain_fast.c /Fe:gain_fast.dll >nul
