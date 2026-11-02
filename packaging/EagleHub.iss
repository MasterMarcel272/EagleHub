#ifndef AppVersion
  #define AppVersion "0.1.0"
#endif

#ifndef StageDir
  #define StageDir "..\artifacts\app"
#endif

[Setup]
AppId={{A8D42F71-6E67-4A4B-9F30-E9B58BB32C84}
AppName=EagleHub
AppVersion={#AppVersion}
AppPublisher=Eagle Studio
DefaultDirName={localappdata}\Programs\Eagle Studio\EagleHub
DefaultGroupName=EagleHub
UninstallDisplayIcon={app}\EagleHub.exe
SetupIconFile=..\assets\EagleHubIcon.ico
OutputDir=..\artifacts
OutputBaseFilename=EagleHub-Setup
ArchitecturesAllowed=x64
ArchitecturesInstallIn64BitMode=x64
PrivilegesRequired=lowest
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
CloseApplications=yes
RestartApplications=no

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "Create a desktop shortcut"; GroupDescription: "Additional icons:"; Flags: unchecked

[Files]
Source: "{#StageDir}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{group}\EagleHub"; Filename: "{app}\EagleHub.exe"
Name: "{autodesktop}\EagleHub"; Filename: "{app}\EagleHub.exe"; Tasks: desktopicon

[Run]
Filename: "{app}\EagleHub.exe"; Description: "Launch EagleHub"; Flags: postinstall nowait skipifsilent
