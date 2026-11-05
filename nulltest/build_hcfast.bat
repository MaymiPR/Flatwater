@rem builds nulltest/hcfast.dll: H-Comp's identified gain path per sample (hcmodel.py, ident_hc_*.py)
@echo off
for /f "usebackq delims=" %%i in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -property installationPath`) do call "%%i\VC\Auxiliary\Build\vcvars64.bat" >nul
cd /d %~dp0
cl /nologo /O2 /LD hcfast.c /Fe:hcfast.dll >nul
