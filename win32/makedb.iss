[Setup]
Bits=32
AppName=New database
AppVerName=New Acedb Database
DefaultGroupName=Acedb Databases
DefaultDirName={sd}\Acedb Databases\New Database
MinVersion=4,4
OutputBaseFileName=makedb
OutputDir=..\bin.WIN32_4
SourceDir=..\bin.WIN32_4
WindowVisible=0
InfoAfterFile= ..\win32\newblurb.txt

[Files]
Source: "wspec\cachesize.wrm"; DestDir: "{app}\wspec";
Source: "wspec\database.wrm"; DestDir: "{app}\wspec";
Source: "wspec\displays.wrm"; DestDir: "{app}\wspec";
Source: "wspec\psfonts.wrm"; DestDir: "{app}\wspec";
Source: "wspec\constraints.wrm"; DestDir: "{app}\wspec";
Source: "wspec\layout.wrm"; DestDir: "{app}\wspec";
Source: "wspec\models.wrm"; DestDir: "{app}\wspec";
Source: "wspec\passwd.wrm"; DestDir: "{app}\wspec";
Source: "wspec\subclasses.wrm"; DestDir: "{app}\wspec";
Source: "wspec\options.wrm"; DestDir: "{app}\wspec";
Source: "database\*.wrm"; DestDir: "{app}\database";
Source: "database\database.map"; DestDir: "{app}\database";
Source: "..\wgf\*"; DestDir: "{app}\wgf";

[ini]
Filename: "{app}\New database.adb"; Section: "ACEDB"; Key: "path"; String: "{app}";

[Icons]
Name: "{group}\New database"; Filename: "{app}\New database.adb"
Name: "{userdesktop}\New database"; Filename: "{app}\New database.adb"

[UninstallDelete]
Type: filesandordirs; Name: "{app}";