include Makefile.def

CURRDIR=./
RM=rm -f
NULL=
OBLANK=$(NULL) $(NULL)

DATE=`date +"%d%m%Y"`

# targets built via this file use message catalogs compiled into binary:

include makedefs.mi

# -I option needed to include (dynamically created) *.msh/rsc files which are in the object subdirectory.
# The weird ./ suffix is needed to keep things together if TARG_OBJDIR is empty, i.e. everything
# ends up in the same directory:

ALLFLAGS = $(TARG_CFLAGS) -I$(TARG_OBJDIR)./ $(MSX_FLAGS) -DINCDIR=\"$(INCDIR)\"

include makedefs.files

#---------------------------------------------------------------------------
# Primary Targets

binaries: $(ALLTARGETS)

binaries-das: $(DASLTARGET) $(DASMSGTARGETS)

all: binaries docs

include makedefs.src

docs: docs_DE docs_EN

$(ASLTARGET): $(ASM_OBJECTS) $(AS_OBJECTS) $(ST_OBJECTS) $(CODE_OBJECTS) $(NLS_OBJECTS)
	$(TARG_LD) -o $(ASLTARGET) $(ASM_OBJECTS) $(AS_OBJECTS) $(ST_OBJECTS) $(CODE_OBJECTS) $(NLS_OBJECTS) -lm $(TARG_LDFLAGS)

$(DASLTARGET): $(DASM_OBJECTS) $(DAS_OBJECTS) $(ST_OBJECTS) $(DECODE_OBJECTS)  $(NLS_OBJECTS)
	$(TARG_LD) -o $(DASLTARGET) $(DASM_OBJECTS) $(DAS_OBJECTS) $(ST_OBJECTS)  $(DECODE_OBJECTS) $(NLS_OBJECTS) $(TARG_LDFLAGS)

$(PLISTTARGET): $(PLIST_OBJECTS) $(NLS_OBJECTS)
	$(TARG_LD) -o $(PLISTTARGET) $(PLIST_OBJECTS) $(NLS_OBJECTS) -lm $(TARG_LDFLAGS)

$(ALINKTARGET): $(ALINK_OBJECTS) $(NLS_OBJECTS)
	$(TARG_LD) -o $(ALINKTARGET) $(ALINK_OBJECTS) $(NLS_OBJECTS) -lm $(TARG_LDFLAGS)

$(PBINDTARGET): $(PBIND_OBJECTS) $(NLS_OBJECTS)
	$(TARG_LD) -o $(PBINDTARGET) $(PBIND_OBJECTS) $(NLS_OBJECTS) -lm $(TARG_LDFLAGS)

$(P2HEXTARGET): $(P2HEX_OBJECTS) $(NLS_OBJECTS)
	$(TARG_LD) -o $(P2HEXTARGET) $(P2HEX_OBJECTS) $(NLS_OBJECTS) -lm $(TARG_LDFLAGS)

$(P2BINTARGET): $(P2BIN_OBJECTS) $(NLS_OBJECTS)
	$(TARG_LD) -o $(P2BINTARGET) $(P2BIN_OBJECTS) $(NLS_OBJECTS) -lm $(TARG_LDFLAGS)

$(RESCOMPTARGET): $(RESCOMP_OBJECTS)
	$(LD) -o $(RESCOMPTARGET) $(RESCOMP_OBJECTS) $(LDFLAGS)

$(TEX2DOCTARGET): $(TEX2DOC_OBJECTS)
	$(LD) -o $(TEX2DOCTARGET) $(TEX2DOC_OBJECTS) $(LDFLAGS) -lm

$(TEX2HTMLTARGET): $(TEX2HTML_OBJECTS)
	$(LD) -o $(TEX2HTMLTARGET) $(TEX2HTML_OBJECTS) $(LDFLAGS) -lm

$(UMLAUTTARGET): $(UMLAUT_OBJECTS)
	$(LD) -o $(UMLAUTTARGET) $(UMLAUT_OBJECTS) $(LDFLAGS)

$(UNUMLAUTTARGET): $(UNUMLAUT_OBJECTS)
	$(LD) -o $(UNUMLAUTTARGET) $(UNUMLAUT_OBJECTS) $(LDFLAGS)

$(MKDEPENDTARGET): $(MKDEPEND_OBJECTS)
	$(LD) -o $(MKDEPENDTARGET) $(MKDEPEND_OBJECTS) $(LDFLAGS)

check_targ_cc:
	@if test "$(TARG_CC)" = ""; then echo "TARG_CC is not set - please review Makefile.def"; exit 1; fi; exit 0

#---------------------------------------------------------------------------
# special rules for objects dependant on string resource files

include makedefs.str

binaries: $(ALLMSGTARGETS)

include makedefs.abh

#---------------------------------------------------------------------------
# Documentation

DOC_COM_DIR=doc_COM/
include $(DOC_COM_DIR)makedefs.dok

DOC_DE_DIR=doc_DE/
include $(DOC_DE_DIR)makedefs.dok
DOC_EN_DIR=doc_EN/
include $(DOC_EN_DIR)makedefs.dok

#---------------------------------------------------------------------------
# Supplementary Targets

test: binaries
	cd tests; OBJDIR=$(TARG_OBJDIR) RUNCMD=$(TARG_RUNCMD) TARG_EXEXTENSION=$(TARG_EXEXTENSION) V=$(V) ./testall "$(TESTDIRS)"

install: all
	INSTROOT=$(INSTROOT) OBJDIR=$(OBJDIR) TARG_OBJDIR=$(TARG_OBJDIR) TARG_EXEXTENSION=$(TARG_EXEXTENSION) ./install.sh $(BINDIR) $(INCDIR) $(MANDIR) $(LIBDIR) $(DOCDIR)

clean_doc: clean_doc_DE clean_doc_EN

clean: clean_doc
	if test "$(HOST_OBJEXTENSION)" != ""; then $(RM) *$(HOST_OBJEXTENSION) $(OBJDIR)*$(HOST_OBJEXTENSION); fi
	if test "$(TARG_OBJEXTENSION)" != ""; then $(RM) *$(TARG_OBJEXTENSION) $(TARG_OBJDIR)*$(TARG_OBJEXTENSION); fi
	$(RM) $(ALLTARGETS) $(HOSTTARGETS) $(OBJDIR)*.dep $(TARG_OBJDIR)*.dep *.p $(TARG_OBJDIR)*.msg *.rsc $(TARG_OBJDIR)*.msh $(TARG_OBJDIR)*.rsc tests/testlog testlog

#---------------------------------------------------------------------------
# Create Distributions

distrib: unjunk
	@if test "$(VERSION)" = ""; then echo "VERSION is not set - please specify VERSION=... as argument"; exit 1; fi; exit 0
	mkdir ../asl-$(VERSION)
	tar cf - $(DISTARCHFILES) | (cd ../asl-$(VERSION); tar xvf -)
	cd ..; tar cvf asl-$(VERSION).tar asl-$(VERSION)
	mv ../asl-$(VERSION).tar ./
	rm -rf ../asl-$(VERSION)
	gzip -9 -f asl-$(VERSION).tar

distdir: all $(UNUMLAUTTARGET)
	@if test "$(VERSION)" = ""; then echo "VERSION is not set - please specify VERSION=... as argument"; exit 1; fi; exit 0
	mkdir asl-$(VERSION)
	chmod 755 asl-$(VERSION)
	OBJDIR=$(OBJDIR) TARG_OBJDIR=$(TARG_OBJDIR) TARG_EXEXTENSION=$(TARG_EXEXTENSION) ./install.sh asl-$(VERSION)/bin asl-$(VERSION)/include asl-$(VERSION)/man asl-$(VERSION)/lib asl-$(VERSION)/doc

win32-distdir: all $(UNUMLAUTTARGET)
	rm -rf as
	mkdir as
	cmd /cinstw32.cmd as\\bin as\\include as\\man as\\lib as\\doc

bindist-tgz: distdir
	tar cvf asl-$(VERSION)-bin.tar asl-$(VERSION)
	rm -rf asl-$(VERSION)
	gzip -9 -f asl-$(VERSION)-bin.tar 

bindist-zip: distdir
	-mv asl-$(VERSION)/lib/*.msg asl-$(VERSION)/bin/
	rmdir asl-$(VERSION)/lib
	mv asl-$(VERSION)/man/man1/* asl-$(VERSION)/man/
	rmdir asl-$(VERSION)/man/man1/
	cd asl-$(VERSION) && zip -9 -r ../asl-$(VERSION)-bin.zip .
	rm -rf asl-$(VERSION)

win32-bindist: win32-distdir
	cd as; zip -9 -r ../as$(VERSION).zip *.*
	zip -9 -r as$(VERSION).zip bin/cyg*
	rm -rf as

#---------------------------------------------------------------------------
# the Debian package (only works under Debian Linux!!!)

debian: docs debversion
	echo "asl (`./debversion -v`) stable; urgency=low" >debian/changelog
	echo "" >>debian/changelog
	echo "  * no changelog here" >>debian/changelog
	echo "" >>debian/changelog
	echo " -- Alfred Arnold <alfred@ccac.rwth-aachen.de> " `822-date` >>debian/changelog
	echo "" >>debian/changelog
	echo `./debversion -v`; 
	dpkg-shlibdeps $(ASLTARGET) $(ALINKTARGET) $(PBINDTARGET) $(PLISTTARGET) $(P2HEXTARGET) $(P2BINTARGET)
	rm -rf bindebian
	mkdir -p bindebian/DEBIAN
	echo "Package: asl" >>bindebian/DEBIAN/control
	echo "Version:" `./debversion -v` >>bindebian/DEBIAN/control
	echo "Section: base" >>bindebian/DEBIAN/control
	echo "Priority: optional" >>bindebian/DEBIAN/control
	echo "Architecture:" `./debversion -a` >>bindebian/DEBIAN/control
	cat debian-files/control >>bindebian/DEBIAN/control
	cp debian-files/postinst debian-files/prerm bindebian/DEBIAN/
	mkdir -p bindebian/usr/lib/asl/
	cp *.msg bindebian/usr/lib/asl/
	mkdir bindebian/usr/lib/asl/include/
	cp include/*.inc bindebian/usr/lib/asl/include/
	mkdir -p bindebian/usr/share/doc/asl/
	cp debian-files/copyright bindebian/usr/share/doc/asl/
	cp changelog bindebian/usr/share/doc/asl/
	mkdir bindebian/usr/share/doc/asl/de/ bindebian/usr/share/doc/asl/en/
	cp doc_DE/as.doc doc_DE/as.html bindebian/usr/share/doc/asl/de/
	cp doc_EN/as.doc doc_EN/as.html bindebian/usr/share/doc/asl/en/
	cp debian-files/changelog.Debian bindebian/usr/share/doc/asl/
	gzip -9 bindebian/usr/share/doc/asl/changelog*
	mkdir -p bindebian/usr/bin
	cp $(ASLTARGET) bindebian/usr/bin
	cp $(ALINKTARGET) bindebian/usr/bin
	cp $(PBINDTARGET) bindebian/usr/bin
	cp $(PLISTTARGET) bindebian/usr/bin
	cp $(P2HEXTARGET) bindebian/usr/bin
	cp $(P2BINTARGET) bindebian/usr/bin
	strip bindebian/usr/bin/*
	strip -R .note -R .comment bindebian/usr/bin/*
	mkdir -p bindebian/usr/share/man/man1
	cp man/*.1 bindebian/usr/share/man/man1
	gzip -9 bindebian/usr/share/man/man1/*.1
	find bindebian -type f | xargs chmod 644
	chmod 755 bindebian/usr/bin/* bindebian/DEBIAN/postinst bindebian/DEBIAN/prerm
	find bindebian -type d | xargs chmod 755
	fakeroot dpkg-deb --build bindebian
	mv bindebian.deb asl_`./debversion -v`_`./debversion -a`.deb

#---------------------------------------------------------------------------
# for my own use only...

archive: unjunk asport.tar.gz
zarchive: unjunk asport.zip

asport.tar.gz: $(ARCHFILES)
	tar cvf asport.tar $(ARCHFILES)
	gzip -9 -f asport.tar

asport.zip: $(ARCHFILES)
	zip -9 -r asport $(ARCHFILES)

unjunk: clean_doc_DE clean_doc_EN
	$(RM) `find . -name "testlog" -print` \
	   `find . -name "*~" -print` \
	   `find . -name "core" -print` \
	   `find . -name "*.core" -print` \
	   `find . -name "*.lst" -print` \
	   `find . -name "lst" -print` \
	   `find . -name "*.noi" -print`

#---------------------------------------------------------------------------

.SUFFIXES: .asm
.asm.p:
	./asl -L -q $*.asm
