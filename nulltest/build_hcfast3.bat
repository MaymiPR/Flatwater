@rem builds nulltest/hcfast3.dll: the release law, third form (ident_hc_rel3.py)
@echo off
for /f "usebackq delims=" %%i in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -property installationPath`) do call "%%i\VC\Auxiliary\Build\vcvars64.bat" >nul
cd /d %~dp0
cl /nologo /O2 /LD hcfast3.c /Fe:hcfast3.dll >nul
