bin_SCRIPTS=isp
if COND_MPICXX
  bin_SCRIPTS += ispcxx ispCC
endif
#bin_SCRIPTS += ispcc
#bin_SCRIPTS += mopper
#bin_SCRIPTS += simian
if COND_GUI
  bin_SCRIPTS += ispUI
endif

include_isp_HEADERS = mpi.h assert.h

distclean: distclean-am
	-rm -f ispcc
	-rm -f Makefile
