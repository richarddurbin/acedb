; -- cdrom.iss --
; 
; This is for the Inno freeware installer; see http:/www.jordanr.dhs.org/isinfo.htm
;
; Install script for the setup.exe file on a CDROM.
; Simply installs first the acedb software, and then the database.

[Setup]
Bits=32
AppName=C Elegans Database CDROM
AppVerName=the 7th January 2000 beta release
CreateAppDir=0
MinVersion=4,4
OutputDir=.
OutputBaseFileName=Setup
InfoBeforeFile=cdromblurb.txt
Uninstallable=0

[Run]
Filename: "{src}\Acedb.exe"; Parameters: "/SP-";
Filename: "{src}\Wormace.exe"; Parameters: "/SP-";




