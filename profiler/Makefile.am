CC = @CXX@
AM_CFLAGS = -g -fPIC -c -I@MPI_INCDIR@ -Wall -O2 -fpermissive
AM_CXXFLAGS = -g -fPIC -c -I@MPI_INCDIR@ -Wall -O2 -fpermissive

lib_LTLIBRARIES = libisp_prof.la
libisp_prof_la_SOURCES = Client.c HashMap.c HashMap_comm.c HashMap_match.c Profiler.c mem.cpp

include_isp_HEADERS = isp.h
