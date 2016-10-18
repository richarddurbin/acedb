This the release of Acedb 4_9 for windows.

The installer will make an entry in the start menu which will run acedb in a mode where it prompts for the database directory.

Alternatively, installing acedb registers a new file type, ".adb" with the 
windows shell. A .adb file contains a pointer to the acedb database directory,
and double-clicking on a .adb file or a shortcut will automatically start 
xace on the database. The right-click context menu for a .adb file also allows
you to open it in tace or in xace with a console window.

.adb files will be created automatically for packaged databases, but to it
is easy to create one for your own datbases, using notepad. The contents of the
file should look like this:

[ACEDB]
path=c:\acedbdatbase

--------------------------------------------------------------------------
If you are running on NT or win98, you should look at the file 
README.fonts in c:\Program Files\Acedb.
--------------------------------------------------------------------------

Known problems.
The following are known problems/issues in this beta release.
1) If acedb is installed in a path which includes spaces, it is unable
   to run scripts in the winscripts directory. This is a bug in the
   cygwin library. As a work-around, the default install path is currently
   c:/Acedb, and not c:/Program Files/Acedb, which is more correct, but 
   tickles the bug.
2) Acedb does not currently use the common installation wgf files,
   they have to be included in the database directory.



--------------------------------------------------------------------------
This release contains the cygwin1.dll library which is copyright Cygnus 
Solutions and released under the  Gnu GPL. In order to fulfill our 
obligations under the GPL, we must make  available both our source code and 
Cygnus's.
The cygwin source code is in pub/acedb/cygwin. Note that anyone who wishes to
download and use the cygwin system is almost certainly better off visting 
http://sourceware.cygnus.com/cygwin/ and downloading it from there.
--------------------------------------------------------------------------
Queries and bug reports should be sent to srk@sanger.ac.uk

Simon Kelley.
9/05/2000
