@rem builds plugins/FwBassLeveler/nulltest/model.dll (Flatwater Bass Leveler's riding model, driven by model.py)
@echo off
for /f "usebackq delims=" %%i in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -property installationPath`) do call "%%i\VC\Auxiliary\Build\vcvars64.bat" >nul
cd /d %~dp0
cl /nologo /O2 /fp:fast /D_USE_MATH_DEFINES /LD model.c /Fe:model.dll >nul
