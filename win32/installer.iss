; -- installer.iss --
; Installer script to make acedb for windows distribution.
; This is for the Inno freeware installer; see http:/www.jordanr.dhs.org/isinfo.htm
;
; Note that it depends on the location of this files being  win32/installer.iss
;
; The Final distribution file will be called Acedb.exe, and will be left in bin.$ACEDB_MACHINE

; srk - 14/12/99

[Setup]
Bits=32
AppName=Acedb
AppVerName=Acedb xxxx
ChangesAssociations=1
AppCopyright=Copyright © 1991-2002 Richard Durbin, Jean Thierry-Mieg and others.
DefaultDirName={sd}\Acedb
DefaultGroupName=Acedb
MinVersion=4,4
OutputDir=..\bin.WIN32_4
SourceDir=..\bin.WIN32_4
OutputBaseFileName=Acedb
LicenseFile=..\win32\copyright.win
InfoBeforeFile=..\win32\READMEFIRST.txt
InfoAfterFile=..\win32\README.txt

[Files]
Source: "..\w3rdparty\cygwin1.dll"; DestDir: "{app}\bin"
Source: "..\w3rdparty\cygreadline5.dll"; DestDir: "{app}\bin"
Source: "..\w3rdparty\cygncurses6.dll"; DestDir: "{app}\bin"
Source: "..\w3rdparty\gdk-1.3.dll"; DestDir: "{app}\bin"
Source: "..\w3rdparty\gtk-1.3.dll"; DestDir: "{app}\bin"
Source: "..\w3rdparty\glib-1.3.dll"; DestDir: "{app}\bin"
Source: "..\w3rdparty\gmodule-1.3.dll"; DestDir: "{app}\bin"
Source: "xace.exe"; DestDir: "{app}\bin";
Source: "tace.exe"; DestDir: "{app}\bin";
Source: "giface.exe"; DestDir: "{app}\bin";
Source: "acediff.exe"; DestDir: "{app}\bin";
Source: "saceserver.exe"; DestDir: "{app}\bin";
Source: "saceclient.exe"; DestDir: "{app}\bin";
Source: "sxaceclient.exe"; DestDir: "{app}\bin";
Source: "dotter.exe"; DestDir: "{app}\bin";
Source: "blixem.exe"; DestDir: "{app}\bin";
Source: "..\w3rdparty\rxvt.exe"; DestDir: "{app}\bin";
Source: "startace.exe"; DestDir: "{app}\bin";
Source: "makedb.exe"; DestDir: "{app}\bin";
Source: "..\w3rdparty\sort.exe"; DestDir: "{app}\bin";
Source: "..\w3rdparty\sh.exe"; DestDir: "{app}\bin";
Source: "..\w3rdparty\gzip.exe"; DestDir: "{app}\bin";
Source: "winaceshell.exe"; DestDir: "{app}\bin";
Source: "makeUserPasswd.exe"; DestDir: "{app}\bin";
Source: "..\win32\README.3rdParty.txt"; DestDir: "{app}";
Source: "..\win32\README.fonts.txt"; DestDir: "{app}";
Source: "..\win32\README.txt"; DestDir: "{app}";
Source: "..\whelp\*"; DestDir: "{app}\whelp";
Source: "..\wtools\*"; DestDir: "{app}\wtools";
Source: "..\winscripts\*"; DestDir: "{app}\winscripts";
Source: "..\wdoc\*"; DestDir: "{app}\wdoc";
Source: "wspec\wingtkrc"; DestDir: "{app}\wspec";
Source: "wspec\cachesize.wrm"; DestDir: "{app}\wspec";
Source: "wspec\database.wrm"; DestDir: "{app}\wspec";
Source: "wspec\displays.wrm"; DestDir: "{app}\wspec";
Source: "wspec\psfonts.wrm"; DestDir: "{app}\wspec";

[Registry]
Root: HKLM; SubKey: "Software\Microsoft\Windows\CurrentVersion\App Paths\startace.exe"; ValueType: string; ValueData: "{app}\bin\startace.exe"; Flags: uninsdeletekey
Root: HKLM; SubKey: "Software\Microsoft\Windows\CurrentVersion\App Paths\startace.exe"; ValueType: string; ValueName: "Path"; ValueData:"{app}\bin; {app}\winscripts";
Root: HKCR; SubKey: ".adb"; ValueType: string; ValueData: "Acedb.Database.Proxy"; Flags: uninsdeletekey;
Root: HKCR; SubKey: "Acedb.Database.Proxy"; ValueType: string; ValueData:"Acedb Database"; Flags: uninsdeletekey;
Root: HKCR; SubKey: "Acedb.Database.Proxy\DefaultIcon"; ValueType: string; ValueData: "{app}\bin\startace.exe"
Root: HKCR; SubKey: "Acedb.Database.Proxy\shell"; ValueType: string; ValueData: "open";
Root: HKCR; SubKey: "Acedb.Database.Proxy\shell\open"; ValueType: string; ValueData: "&Open"
Root: HKCR; SubKey: "Acedb.Database.Proxy\shell\open\command"; ValueType: string; ValueData: """{app}\bin\startace.exe"" ""{app}\bin\xace.exe"" ""%1"""
Root: HKCR; SubKey: "Acedb.Database.Proxy\shell\xace"; ValueType: string; ValueData: "&Open in Console Xace";
Root: HKCR; SubKey: "Acedb.Database.Proxy\shell\xace\command"; ValueType: string; ValueData: """{app}\bin\startace.exe"" ""{app}\bin\xace.exe"" ""%1"" ""{app}\bin\rxvt.exe"""
Root: HKCR; SubKey: "Acedb.Database.Proxy\shell\tace"; ValueType: string; ValueData: "&Open in Tace";
Root: HKCR; SubKey: "Acedb.Database.Proxy\shell\tace\command"; ValueType: string; ValueData: """{app}\bin\startace.exe"" ""{app}\bin\tace.exe"" ""%1"" ""{app}\bin\rxvt.exe"""
Root: HKCR; SubKey: ".wrm"; ValueType: string; ValueData: "Acedb.ConfigFile"; Flags: uninsdeletekey;
Root: HKCR; SubKey: "Acedb.ConfigFile"; ValueType: string; ValueData:"Acedb Configuration"; Flags: uninsdeletekey;
Root: HKCR; SubKey: "Acedb.ConfigFile\DefaultIcon"; ValueType: string; ValueData: "{app}\bin\startace.exe"
Root: HKCR; SubKey: "Acedb.ConfigFile\shell"; ValueType: string; ValueData: "open";
Root: HKCR; SubKey: "Acedb.ConfigFile\shell\open"; ValueType: string; ValueData: "&Open"
Root: HKCR; SubKey: "Acedb.ConfigFile\shell\open\command"; ValueType: string; ValueData: "notepad ""%1"""
Root: HKLM; SubKey: "Software\Acedb.org\Acedb\4.x"; ValueType: string; ValueName: "InstallPath"; ValueData: "{app}"; Flags: uninsdeletekey;

; Need more than the default amount of heap for wormbase......
Root: HKCU; SubKey: "Software\Cygnus Solutions\Cygwin"; Valuetype: dword; ValueName: "heap_chunk_in_mb"; ValueData: "256" 

[Icons]
Name: "{group}\Acedb"; Filename: "{app}\bin\startace"; Parameters:"{app}\bin\xace.exe";
Name: "{group}\New database Wizard"; Filename: "{app}\bin\makedb"; Parameters: "/SP-";

[UninstallDelete]
Type: filesandordirs; Name: "{app}";
