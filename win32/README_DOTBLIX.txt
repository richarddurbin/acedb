Note: A copy of this readme file will be placed in the directory  
where you install Dotter and Blixem for your comfort and convenience.
Share and enjoy!

This release of Dotter and Blixem corresponds to Acedb 4_9u for Windows.

The installer will not make an entry in the start menu since this 
release of both dotter and blixem must be run from a command prompt.

This version of blixem uses pfetch to retrieve sequence entries from
the EMBL database.  There is no option to use efetch.  This means the 
environment variable BLIXEM_FETCH_PFETCH must exist, though the value 
is unimportant.  A start-up message that blixem is unable to efetch the 
sequence implies that this variable does not exist. 

Note that this doesn't stop blixem from running, but restricts it to 
local files only.  It's also necessary for your PATH to include the 
location of blixem and pfetch.

To run Blixem, at the command prompt, type
   blixem <sequencfile> <datafile>
eg
   blixem ACEDB_sequence ACEDB_data


To run Dotter, at the command prompt, type
   dotter <file1> <file2>
eg
   dotter ACEDB_sequence ACEDB_sequence2

--------------------------------------------------------------------------
This release contains version 1.3.22-1 of the cygwin1.dll library which is 
copyright Cygnus Solutions and released under the  Gnu GPL. In order to 
fulfill our obligations under the GPL, we must ensure that both our own 
and Cygnus' source code is freely available.

Our own source code is available for free download from our website
www.acedb.org.

Cygnus source code may be downloaded from their website www.cygnus.com.
--------------------------------------------------------------------------
Queries and bug reports should be sent to rnc@sanger.ac.uk

Rob Clack.
18/09/2003
