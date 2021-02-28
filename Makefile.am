AUTOMAKE_OPTIONS = foreign
ACLOCAL_AMFLAGS = -I m4
SUBDIRS = libopts scheduler profiler scripts
if COND_GUI
  SUBDIRS += UI
endif

TARDIR=./@PACKAGE_TARNAME@-@PACKAGE_VERSION@
LIBTOOL_DEPS = @LIBTOOLS_DEPS@

libtool: $(LIBTOOL_DEPS)
	$(SHELL) ./config.status --recheck

tarball:
	-rm -rf $(TARDIR) $(TARDIR).tar.gz
	mkdir $(TARDIR)
	mkdir $(TARDIR)/scheduler
	mkdir $(TARDIR)/profiler
	mkdir $(TARDIR)/UI
	mkdir $(TARDIR)/scripts
	mkdir $(TARDIR)/libopts
	mkdir $(TARDIR)/m4
	cp ./INSTALL ./config.sub ./config.guess ./ltmain.sh ./*.m4 ./configure ./depcomp ./install-sh ./LICENSE ./Makefile.in ./missing $(TARDIR)
	cp ./scheduler/*.cpp ./scheduler/*.hpp ./scheduler/Makefile.in ./scheduler/config.hpp.in ./scheduler/*.c ./scheduler/*.h $(TARDIR)/scheduler
	@-rm $(TARDIR)/scheduler/config.hpp
	cp ./profiler/*.c ./profiler/*.cpp ./profiler/*.h ./profiler/Makefile.in $(TARDIR)/profiler
	cd ./UI; make ispui.jar
	cp ./UI/ispui.jar ./UI/Makefile.in $(TARDIR)/UI
	cp ./scripts/isp*.in ./scripts/Makefile.in ./scripts/mpi.h.in ./scripts/assert.h.in $(TARDIR)/scripts
	cp -r ./libopts/* $(TARDIR)/libopts
	rm -rf $(TARDIR)/libopts/.libs $(TARDIR)/libopts/_libs $(TARDIR)/libopts/libopts.la
	rm -rf $(TARDIR)/libopts/*.o $(TARDIR)/libopts/*.lo
	cp ./m4/* $(TARDIR)/m4
	if [ -d isp-tests ]; then \
		mkdir $(TARDIR)/isp-tests/; \
		svn list -R ./isp-tests | awk '/\/$$/ {print "$(TARDIR)/isp-tests/" $$1}' | xargs mkdir; \
		svn list -R ./isp-tests | awk '/[^\/]$$/ {print "./isp-tests/" $$0}' | xargs -I{} cp {} $(TARDIR)/{}; \
	fi
	if [ -d VS ]; then \
		mkdir $(TARDIR)/VS/; \
		cp -r ./VS/* $(TARDIR)/VS \
	fi
	tar -czf $(TARDIR).tar.gz $(TARDIR)
	rm -rf $(TARDIR)

if COND_TESTS
tests:
	cd isp-tests; make tests
endif
