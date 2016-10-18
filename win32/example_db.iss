; -- example_db.iss --
; Sample installer script for an acedb database for windows distribution.
; This is for the Inno freeware installer; see http:/www.jordanr.dhs.org/isinfo.htm
;
; Put this (.iss) file in the same directory as the database, change Cgcace  everywhere
; to your database, name, add any extra Files/dirs to the [Files] section and adjust the
; OutputDir to somewhere with plenty of space.
;
; Note that the Inno compression code seems to be very inefficient: so expect to wait 
; hours for a large DB. 

; srk - 17/12/99

[Setup]
Bits=32
AppName=Cgcace
AppVerName=Cgcace
DefaultGroupName=Acedb Databases
DefaultDirName={sd}\Acedb Databases\Cgcace
MinVersion=4,4
OutputBaseFileName=Cgcace
OutputDir=e:\

[Files]
Source: "database\*"; DestDir: "{app}\database";
Source: "wspec\*"; DestDir: "{app}\wspec";

[ini]
Filename: "{app}\Cgcace.adb"; Section: "ACEDB"; Key: "path"; String: "{app}";

[Icons]
Name: "{group}\Cgcace"; Filename: "{app}\Cgcace.adb"
Name: "{userdesktop}\Cgcace"; Filename: "{app}\Cgcace.adb"

[UninstallDelete]
Type: filesandordirs; Name: "{app}";



