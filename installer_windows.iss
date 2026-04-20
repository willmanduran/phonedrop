[Setup]
AppId={{B7A24F82-1D4B-4E3D-BB8A-E7B8B1234567}
AppName=PhoneDrop
AppVersion=1.0.0
AppPublisher=Willman's Toolbox
DefaultDirName={autopf}\PhoneDrop
DefaultGroupName=PhoneDrop
OutputBaseFilename=PhoneDrop_Setup
SetupIconFile=assets\phonedrop.ico
Compression=lzma
SolidCompression=yes
PrivilegesRequired=admin

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"
Name: "spanish"; MessagesFile: "compiler:Languages\Spanish.isl"

[CustomMessages]
english.ContextMenuEntry=Send to phone with PhoneDrop
spanish.ContextMenuEntry=Enviar al teléfono con PhoneDrop

[Files]
Source: "build\phonedrop.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "web\*"; DestDir: "{app}\web"; Flags: ignoreversion recursesubdirs

[Icons]
Name: "{group}\PhoneDrop"; Filename: "{app}\phonedrop.exe"
Name: "{commondesktop}\PhoneDrop"; Filename: "{app}\phonedrop.exe"
Name: "{usersendto}\PhoneDrop"; Filename: "{app}\phonedrop.exe"

[Registry]
Root: HKA; Subkey: "Software\Classes\*\shell\PhoneDrop"; ValueType: string; ValueData: "{cm:ContextMenuEntry}"; Flags: uninsdeletekey
Root: HKA; Subkey: "Software\Classes\*\shell\PhoneDrop"; ValueType: string; ValueName: "Icon"; ValueData: "{app}\phonedrop.exe"; Flags: uninsdeletekey
Root: HKA; Subkey: "Software\Classes\*\shell\PhoneDrop\command"; ValueType: string; ValueData: """{app}\phonedrop.exe"" ""%1"""; Flags: uninsdeletekey

Root: HKA; Subkey: "Software\Classes\Directory\shell\PhoneDrop"; ValueType: string; ValueData: "{cm:ContextMenuEntry}"; Flags: uninsdeletekey
Root: HKA; Subkey: "Software\Classes\Directory\shell\PhoneDrop"; ValueName: "Icon"; ValueData: "{app}\phonedrop.exe"; Flags: uninsdeletekey
Root: HKA; Subkey: "Software\Classes\Directory\shell\PhoneDrop\command"; ValueType: string; ValueData: """{app}\phonedrop.exe"" ""%1"""; Flags: uninsdeletekey

[Run]
Filename: "{app}\phonedrop.exe"; Description: "{cm:LaunchProgram,PhoneDrop}"; Flags: nowait postinstall skipifsilent