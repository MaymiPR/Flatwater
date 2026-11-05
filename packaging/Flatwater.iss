; Windows installer for the whole Flatwater suite (Inno Setup 7). Built by .github/workflows/release.yml, or locally after a
; Release build of every plug-in's _VST3, _CLAP and _Standalone targets:
;   pwsh packaging/make_iss_include.ps1 build      (writes packaging/plugins.iss.inc from plugins.txt)
;   iscc /DAppVersion=<version> packaging\Flatwater.iss          (writes dist\Flatwater-<version>-Windows-Setup.exe)
; /DTestRoot=<folder> makes a test installer that puts everything under that folder and needs no administrator rights
; (to check the packaging and the uninstaller without touching Program Files).

#ifndef AppVersion
  #define AppVersion "0.1.0"
#endif
#ifdef TestRoot
  #define VstDir TestRoot + "\VST3\Flatwater"
  #define ClapDir TestRoot + "\CLAP\Flatwater"
  #define AppDir TestRoot + "\App"
#else
  #define VstDir "{commoncf64}\VST3\Flatwater"
  #define ClapDir "{commoncf64}\CLAP\Flatwater"
  #define AppDir "{autopf}\Flatwater"
#endif

[Setup]
AppName=Flatwater
AppVersion={#AppVersion}
AppPublisher=Flatwater
AppPublisherURL=https://github.com/MaymiPR/Flatwater
AppSupportURL=https://github.com/MaymiPR/Flatwater/issues
DefaultDirName={#AppDir}
DefaultGroupName=Flatwater
DisableProgramGroupPage=yes
DisableDirPage=yes
LicenseFile=..\LICENSE
InfoBeforeFile=ABOUT.txt
OutputDir=..\dist
#ifdef TestRoot
OutputBaseFilename=Flatwater-{#AppVersion}-Windows-Setup-TEST
PrivilegesRequired=lowest
AppId={{398E4AB3-D38D-4221-8D28-A241FC7F460D}
#else
OutputBaseFilename=Flatwater-{#AppVersion}-Windows-Setup
PrivilegesRequired=admin
AppId={{FD12655C-898E-4BCF-9B39-16B5540D1933}
#endif
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
; Inno Setup 7: a 64-bit Setup.exe (the default would still be 32-bit); AppVerName and TimeStampsInUTC are
; stated because their defaults changed in 7 (name plus version with no localized word; UTC time stamps).
SetupArchitecture=x64
AppVerName=Flatwater {#AppVersion}
TimeStampsInUTC=yes
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
UninstallDisplayName=Flatwater
CloseApplications=yes

[Types]
Name: "full"; Description: "All plug-ins, VST3 and CLAP"
Name: "all"; Description: "All plug-ins, VST3, CLAP and the standalone apps"
Name: "custom"; Description: "Choose plug-ins and formats"; Flags: iscustom

; [Components], [Files], [Icons] and [UninstallDelete] for every plug-in in plugins.txt
#include "plugins.iss.inc"

[Files]
Source: "..\LICENSE"; DestDir: "{app}"; DestName: "LICENSE.txt"; Flags: ignoreversion
Source: "THIRD-PARTY-NOTICES.txt"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{group}\Uninstall Flatwater"; Filename: "{uninstallexe}"
