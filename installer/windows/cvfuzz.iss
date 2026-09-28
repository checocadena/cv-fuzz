; Windows installer for CV Fuzz (Inno Setup 6).
; Built by .github/workflows/release.yml:  ISCC /DAppVersion=1.0.0 /DArtDir=<artefacts> cvfuzz.iss
#ifndef AppVersion
  #define AppVersion "1.0.0"
#endif
#ifndef ArtDir
  #define ArtDir "..\..\build\CVFuzz_artefacts\Release"
#endif

[Setup]
AppId={{6D1F2C1E-7B0A-4C55-9C6E-CF0FA2A8B1D3}
AppName=CV Fuzz
AppVersion={#AppVersion}
AppPublisher=Checo Cadena
AppPublisherURL=https://checocadena.com
DefaultDirName={autopf}\CV Fuzz
DefaultGroupName=CV Fuzz
DisableProgramGroupPage=yes
OutputBaseFilename=CV-Fuzz-Windows-Setup
OutputDir=..\..\dist
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin
UninstallDisplayName=CV Fuzz

[Messages]
WelcomeLabel2=A fuzz plugin and interactive resume by Checo Cadena.%n%nThis is a C++ model of the fuzz pedal I designed and built during my time at NYU. It features asymmetric clipping, a starvable bias stage, 4x oversampling and a tone control. The interface is my resume. Each knob shapes the sound and opens up a different part of my resume.

[Types]
Name: "full"; Description: "VST3 plugin and standalone app"
Name: "custom"; Description: "Choose components"; Flags: iscustom

[Components]
Name: "vst3"; Description: "VST3 plugin (C:\Program Files\Common Files\VST3)"; Types: full custom
Name: "app"; Description: "Standalone app"; Types: full custom

[Files]
Source: "{#ArtDir}\VST3\CV Fuzz.vst3\*"; DestDir: "{commoncf64}\VST3\CV Fuzz.vst3"; Flags: recursesubdirs createallsubdirs ignoreversion; Components: vst3
Source: "{#ArtDir}\Standalone\CV Fuzz.exe"; DestDir: "{app}"; Flags: ignoreversion; Components: app

[Icons]
Name: "{autoprograms}\CV Fuzz"; Filename: "{app}\CV Fuzz.exe"; Components: app

[Run]
Filename: "{app}\CV Fuzz.exe"; Description: "Open CV Fuzz"; Flags: nowait postinstall skipifsilent; Components: app
