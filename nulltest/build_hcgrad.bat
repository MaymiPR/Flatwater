@rem builds nulltest/hcgrad.dll: the release law, sixth form (ident_hc_rel4.py)
@echo off
for /f "usebackq delims=" %%i in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -property installationPath`) do call "%%i\VC\Auxiliary\Build\vcvars64.bat" >nul
cd /d %~dp0
cl /nologo /O2 /LD hcgrad.c /Fe:hcgrad.dll >nul
