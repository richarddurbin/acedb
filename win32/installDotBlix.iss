; -- installer.iss --
; Installer script to make a separate Dotter and Blixem distribution for Windows.
; This is for the Inno freeware installer; see http:/www.jordanr.dhs.org/isinfo.htm
;
; Note that it depends on the location of this file being  win32/installDotBlix.iss
;
; The Final distribution file will be called DotBlixem.exe, and will be left in bin.$ACEDB_MACHINE

; rnc - 17/09/03

[Setup]
Bits=32
AppName=Dotter and Blixem
AppVerName=Dotter and Blixem
ChangesAssociations=1
AppCopyright=Copyright © 1991-2003 Richard Durbin, Jean Thierry-Mieg and others.
DefaultDirName={sd}\Dotter and Blixem
DefaultGroupName=Dotter and Blixem
MinVersion=4,4
OutputDir=..\bin.WIN32_4
SourceDir=..\bin.WIN32_4
OutputBaseFileName=DotBlixem
LicenseFile=..\win32\copyright_dotblix.txt
InfoAfterFile=..\win32\README_DOTBLIX.txt

[Files]
Source: "..\w3rdparty\cygwin1.dll"; DestDir: "{app}\bin"
Source: "..\w3rdparty\cygreadline5.dll"; DestDir: "{app}\bin"
Source: "..\w3rdparty\cygncurses6.dll"; DestDir: "{app}\bin"
Source: "..\w3rdparty\gdk-1.3.dll"; DestDir: "{app}\bin"
Source: "..\w3rdparty\gtk-1.3.dll"; DestDir: "{app}\bin"
Source: "..\w3rdparty\glib-1.3.dll"; DestDir: "{app}\bin"
Source: "..\w3rdparty\gmodule-1.3.dll"; DestDir: "{app}\bin"
Source: "dotter.exe"; DestDir: "{app}\bin";
Source: "blixem.exe"; DestDir: "{app}\bin";
Source: "..\win32\README_DOTBLIX.txt"; DestDir: "{app}"

[Registry]

[Icons]

[UninstallDelete]
Type: filesandordirs; Name: "{app}";
