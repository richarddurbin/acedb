#####################################################################
#### Main makefile for Acedb, contig tools and Image
#
#
# $Id: makefile,v 1.132 2012-03-12 11:31:21 gb10 Exp $
#####################################################################



# don't change this, using other shells can lead to portability problems.
SHELL = /bin/sh



#####################################################################
#### ACEDB programs


# Sub categories of code, please add new stuff to one of these groups if possible
#

OTTERLACE_BIN = tace giface xace belvu makeUserPasswd sgifaceserver xremote

ACE_LIBS = libfree.a libtsfree.a libgraph.a libgex.a libmd5.a libgd.a libacecl.a libace.a

ACE_BIN = xace xacezmap xremote tace giface taql saceserver sgifaceserver saceclient

ACEMBLY_BIN = makeSCF xacembly tacembly gifacemblyserver

ALIGN_BIN = blixem blixelect blixemh dotter dotterbatch belvu

OTHER_BIN = efetch gnbk gnbkclient genefeatures

STD_TOOLS = makeUserPasswd acediff genefeatures tagcount
TOOLS_BIN = acediffsorted pmapace diskdump diskfix metacheck homonym $(STD_TOOLS)

TEST_BIN = xtest coltest giftest boxtest md5test acelibtest

ACE5_BIN = tace5 xace5 xacembly5 tacembly5

JADE_BIN = jade2ace jadeserver jade2sybase

CTF_BIN = scf2ctf ctf2scf

UNSUPPORTED_BIN = sxaceclient

INSTALL_BIN = $(ACE_BIN) $(ACEMBLY_BIN) $(ALIGN_BIN) $(OTHER_BIN) $(TOOLS_BIN)




# Make sure this is ALWAYS the first target in this makefile, the overnight and release
# build systems all rely on this.
all : libs ace other tools

otterlace: $(OTTERLACE_BIN)

dist_build: libs ace $(ALIGN_BIN) $(STD_TOOLS)

libs : $(ACE_LIBS)

ace : $(ACE_BIN)

acembly : libstaden.a $(ACEMBLY_BIN)

other : $(ALIGN_BIN) $(OTHER_BIN)

tools : $(TOOLS_BIN)

test : $(TEST_BIN)

ace5 : $(ACE5_BIN)

jade : $(JADE_BIN)

ctf : $(CTF_BIN)

pfetch: pfetch_libs


# This target makes sure that the time stamp reported with the "-version" flag
# for the executables gets changed just once per make. This means that all
# executables produced in a single make will have the same timestamp. The
# timestamp code is in wnq/aceversion.c which gets archived into libfree.a,
# which is itself included in (almost) all executables.
#
# The build directory is also stored in aceversion.c so we know where an
# executable was built.
#
VERSION_TIMESTAMP_FILE = version_timestamp_file
version_timestamp:
	@chmod u+w ./wnq/aceversion.c
	@./wtools/aceSetReleaseDir ./wnq "`pwd`/bin.$(ACEDB_MACHINE)"
	@chmod u-w ./wnq/aceversion.c
	@touch bin.$(ACEDB_MACHINE)/$(VERSION_TIMESTAMP_FILE)


# this is a new version of the below target, I'm attempting to clean up
# the acedb build because we have lots of defunct targets/duplication etc.
# Note that each line here is a copy of the above relevant lines _in order_,
# now I've tidied this up please try to keep it like this.
#
libfree.a libtsfree.a libgraph.a libgex.a libmd5.a libgd.a libacecl.a libace.a  libaccl.a \
taceclient accmd saccmd acctest acstest \
xace xacezmap xremote tace giface taql \
saceserver sgifaceserver saceclient sxaceclient sace_build \
libstaden.a makeSCF xacembly tacembly gifacemblyserver \
blixem blixemh blixelect dotter dotterbatch belvu jade2ace efetch gnbk gnbkclient \
makeUserPasswd acediff acediffsorted pmapace diskdump diskfix metacheck homonym tagcount \
xtest coltest giftest boxtest md5test acelibtest \
tace5 xace5 xacembly5 tacembly5 \
jadeserver jade2sybase genefeatures \
scf2ctf ctf2scf \
WinDist WinDotBlix \
pfetch_libs  dummy  xacez \
dist : bin-dir version_timestamp
	@echo 'making' $@ 'for $(ACEDB_MACHINE)'
	@cd bin.$(ACEDB_MACHINE) ; \
	cp ../wmake/$(ACEDB_MACHINE)_DEF ./deffile; \
	cp ../wmake/truemake ./makefile ; \
	chmod u+w ./makefile ./deffile; touch ./makefile ./deffile; \
	export VERSION_TIMESTAMP_FILE=$(VERSION_TIMESTAMP_FILE) ; $(MAKE) $@

# Really this should be merged with the above, all that's different is the message...
libfree_clean libgraph_clean libace_clean libxace_clean \
		xace_clean xacezmap_clean tace_clean \
		xaceclient_clean \
		aceclient_clean aceserver_clean \
		sace_clean \
		xacembly_clean tacembly_clean \
		gifacemblyserver_clean \
		pfetch_clean clean bindistclean : bin-dir
	@echo "removing `echo $@ | cut -d_ -f1` .o files for $(ACEDB_MACHINE)"
	@cd bin.$(ACEDB_MACHINE) ; \
	cp ../wmake/truemake ./makefile ; \
	cp ../wmake/$(ACEDB_MACHINE)_DEF ./deffile; \
	chmod u+w ./makefile ./deffile; touch ./makefile ./deffile; \
	$(MAKE) $@


#####################################################################
# to install binaries,subdirectories in $ACEDB/bin
INSTALL_DIRS = wdoc wgf wquery wscripts wspec wtools


install :
	if [ ! -d $(ACEDB)/bin ] ; then \
	  mkdir $(ACEDB)/bin; \
	fi
	for target in $(INSTALL_DIRS) ; do  \
	echo 'installing' $$target 'for $(ACEDB_MACHINE)' ; \
	cp -r $$target $(ACEDB)/bin ; \
	done
	@cd bin.$(ACEDB_MACHINE) ; \
	$(MAKE) "INSTALL_BIN=$(INSTALL_BIN)" install_bin ; \


#####################################################################
#### CONTIG programs

contig_all:
	@echo ""
	@echo " Compiling ContigC programs"
	@echo "-------------------------------------------------"
	$(MAKE) contig pmap fphinx fpabi openmapdb mapsub extend \
		getascdb lastclones contasp logdb rancl randb \
		model anytoany checkdb rmvec
	@echo ""
	@echo "These binaries are available :"
	@echo ""
	@ls -l bin.$(ACEDB_MACHINE)  | grep '^-..x'
	@echo ""

## graphical contig programs  (build libfree and libgraph beforehand)
contig pmap : bin-dir
	@echo 'making' $@ 'for $(ACEDB_MACHINE)'
	@cd bin.$(ACEDB_MACHINE) ; \
	cp ../wmake/$(ACEDB_MACHINE)_DEF ./deffile; \
	cp ../wmake/truemake ./makefile ; \
	chmod u+w makefile deffile; touch makefile deffile; \
	$(MAKE) libfree.a libgraph.a libgraphXaw.a libgd.a ; \
	cp ../wmake/contigmake ./makefile ; \
	chmod u+w makefile; touch makefile; \
	$(MAKE) $@

## text-only contig support programs
fphinx fpabi contig_dumpace openmapdb mapsub \
		extend getascdb lastclones contasp \
		logdb rancl randb model anytoany \
		checkdb rmvec : bin-dir
	@echo 'making' $@ 'for $(ACEDB_MACHINE)'
	@cd bin.$(ACEDB_MACHINE) ; \
	cp ../wmake/$(ACEDB_MACHINE)_DEF ./deffile; \
	cp ../wmake/contigmake ./makefile ; \
	chmod u+w makefile deffile; touch makefile deffile; \
	$(MAKE) $@

contig_clean:	bin-dir
	@echo 'removing contigc .o files for $(ACEDB_MACHINE)'
	@cd bin.$(ACEDB_MACHINE) ; \
	cp ../wmake/contigmake ./makefile ; \
	cp ../wmake/$(ACEDB_MACHINE)_DEF ./deffile; \
	chmod u+w makefile deffile; touch makefile deffile; \
	$(MAKE) $@



#####################################################################
#### IMAGE programs

image:
	@echo ""
	@echo " Compiling Image 3 fingerprint image analysis"
	@echo "-------------------------------------------------"
	$(MAKE) im3 amStep2 amStep3 amStep4 amStep5 im3conv image_configure banddiff
	@echo ""
	@echo "These binaries are available :"
	@echo ""
	@ls -l bin.$(ACEDB_MACHINE)  | grep '^-..x'
	@echo ""

## graphical image apps (build libfree and libgraph beforehand)
im3 bstt : bin-dir
	@echo 'making' $@ 'for $(ACEDB_MACHINE)' ; echo
	@cd bin.$(ACEDB_MACHINE) ; \
	cp ../wmake/$(ACEDB_MACHINE)_DEF ./deffile; \
	cp ../wmake/truemake ./makefile ; \
	chmod u+w makefile deffile; touch makefile deffile; \
	$(MAKE) libfree.a libgraph.a libgex.a libgd.a ; \
	cp ../wmake/im3make ./makefile ; \
	chmod u+w makefile; touch makefile; \
	$(MAKE)  $@

## automated analysis apps, written in C++
amStep2 amStep3 amStep4 amStep5 : bin-dir
	@echo 'making' $@ 'for $(ACEDB_MACHINE)'
	@cd bin.$(ACEDB_MACHINE) ; \
	cp ../wmake/$(ACEDB_MACHINE)_DEF ./deffile; \
	cp ../wmake/im3make ./makefile ; \
	chmod u+w makefile deffile; touch makefile deffile; \
	$(MAKE) $@

## image utilities (build libfree.a beforehand)
im3conv banddiff: bin-dir
	@echo 'making' $@ 'for $(ACEDB_MACHINE)'
	@cd bin.$(ACEDB_MACHINE) ; \
	cp ../wmake/$(ACEDB_MACHINE)_DEF ./deffile; \
	cp ../wmake/truemake ./makefile ; \
	chmod u+w makefile deffile; touch makefile deffile; \
	$(MAKE) libfree.a ; \
	cp ../wmake/im3make ./makefile ; \
	chmod u+w makefile; touch makefile; \
	$(MAKE) $@

## configure utility - doesn't not require any libraries
image_configure:	bin-dir
	@cd bin.$(ACEDB_MACHINE) ; \
	cp ../wmake/im3make ./makefile ; \
	cp ../wmake/$(ACEDB_MACHINE)_DEF ./deffile; \
	chmod u+w makefile deffile; touch makefile deffile; \
	$(MAKE) $@

image_clean:	bin-dir
	@echo 'removing image .o files for $(ACEDB_MACHINE)'
	@cd bin.$(ACEDB_MACHINE) ; \
	cp ../wmake/im3make ./makefile ; \
	cp ../wmake/$(ACEDB_MACHINE)_DEF ./deffile; \
	chmod u+w makefile deffile; touch makefile deffile; \
	$(MAKE) $@

im2: bin-dir
	@echo 'making' $@ 'for $(ACEDB_MACHINE)' ; echo
	@cd bin.$(ACEDB_MACHINE) ; \
	cp ../im2src/makefile.image2 . ; \
	chmod u+w makefile.image2 ; \
	$(MAKE) -f makefile.image2 $@


#####################################################################
#### binary directory bin.$(ACEDB_MACHINE)
####
#### Dependency used by all of the above targets to check that user
#### has set up the ACEDB_MACHINE environment variable correctly.
####
bin-dir:
	@if test $(ACEDB_MACHINE) ; then exit ; \
	else \
	echo  'To compile, please setenv ACEDB_MACHINE (now not set) to one of: ' ; \
	cd wmake ; echo ; ls *DEF ; \
	echo	 '      without the _DEF ending,' ; \
	echo     '      e.g.  add in your .login file the line '; \
	echo  ' setenv ACEDB_MACHINE  SUN' ; \
	echo '  or some other machine name defined in the directory wmake.' ;\
	echo '     Thank you' ; echo ; echo ; exit 1 ; fi
	@if test -f wmake/$(ACEDB_MACHINE)_DEF ; then exit ; \
	else echo  'To compile, please setenv ACEDB_MACHINE (now = $(ACEDB_MACHINE) ) to one of: ' ; \
	cd wmake ; echo ; ls *DEF ; \
	echo	 '      without the _DEF ending,' ; \
	echo     '      i.e.  add in your .login file the line '; \
	echo  ' setenv ACEDB_MACHINE  SUN' ; \
	echo '  or some other machine name defined in the directory wmake.' ;\
	echo '     Thank you' ; echo ; echo ; exit 1 ; fi
	@if test -d bin.$(ACEDB_MACHINE)  ; then exit ; \
	else mkdir bin.$(ACEDB_MACHINE)  ; fi

#####################################################################
#### end of makefile
#####################################################################
