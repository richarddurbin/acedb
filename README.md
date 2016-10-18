# acedb

This is the official github repository of the Sanger Institute branch of
acedb (also capitalised as ACeDB, Acedb, ACEDB etc.).

Acedb was originally
developed as the genome database system and browser for the model organism *C. elegans* by Jean Thierry-Mieg and Richard Durbin, starting around 1990.  It was subsequently extended over many years, with many applications in biology and elsewhere, and
substantial input and additions from many others including Simon Kelley and Ed Griffiths.  Around the late 1990s there was a major fork, with one branch maintained and developed by Jean
Thierry-Mieg at NCBI, with in particular extensive additions for assembly and
 RNAseq data analysis, and the other banch maintained by a series
of individuals at the Wellcome Trust Sanger Institute, with in
particular the addition of the zmap code used by the Havana vertebrate
gene annotation group.  

In October 2016 ongoing active support of the second branch by Ed
Griffiths and his team at the Sanger Insitute came to an end, and the 
code was archived in this repository at
[https://github.com/richarddurbin/acedb](https://github.com/richarddurbin/acedb).  The NCBI fork continues to be maintained by [Jean Thierry-Mieg](mailto:mieg@ncbi.nlm.nih.gov). 

acedb is licensed under the GNU public licence as described in the file
[wdoc/GNU_LICENSE.README] (wdoc/GNU_LICENSE.README).

Extensive documentation from a range of different times, including the original READMEs, can be found in [wdoc/](wdoc/).

In brief, to compile and get going in a Unix environment you first need to install the readline package and Glib, GDK and GTK, which can be obtained together in the GTK+ package from  [www.gtk.org](http://www.gtk.org).  Then you need to set the environment variable ACEDB_MACHINE to something appropriate in [wmake/](wmake/), e.g.
```
setenv ACEDB_MACHINE LINUX_4
```
then
```
ln -s wmake/make
make
```
You should have lots of executables (and other files) in the directory `bin.{ACEDB_MACHINE}/`, e.g. in this case `bin.LINUX_4/`. You can either link to these or add this directory to your path.  Once you have done this, you can make a test database in the `wdemo/` directory with
```
mkdir wdemo/database                        # needed to store the binary database
echo `whoami` >> wdemo/wspec/passwd.wrm     # needed to give yourself write access - use your user id directly if missing 'whoami' 
tace wdemo
y                                           # this is your reply to the question whether you want to initialise a new empty database
parse wdemo/database.data.ace
classes                                     # will show how many objects you have created in which classes
save
quit
```
Now you can run the graphical interface with the command `xace wdemo` and browse the database. And of course you can do lots of other things, either via the command line or graphical interface, or `aceserver`/`aceclient` system.  Just try things out, or even read the documentation.  And enjoy!







