AUTOMAKE_OPTIONS = subdir-objects
CC = @CXX@
AM_LDFLAGS = -pthread  $(LIBOPTS_LDADD) -lz3 -lbliss -Lsimian-core/bliss-0.73
#if COND_STATIC_LINK
#  AM_LDFLAGS += -static ../solver/solver-src/solvers.a ../solver/big-int/big-int.a 
#endif
AM_CXXFLAGS = -DHAVE_STDINT_H -Wall $(LIBOPTS_CFLAGS) -O2 -std=c++11 -Isimian-core/bliss-0.73
AM_CFLAGS = -DHAVE_STDINT_H -Wall $(LIBOPTS_CFLAGS) -O2 -std=c++11
#INCLUDES = -Isimian-core/bliss-0.73
#bin_PROGRAMS = isp.exe
bin_PROGRAMS = simian
simian_SOURCES = Socket.cpp ServerSocket.cpp MpiProc.cpp Envelope.cpp \
                  name2id.cpp  Scheduler.cpp  Counter.cpp Transition.cpp TransitionList.cpp \
                  InterleavingTree.cpp utility.cpp Mo.cpp Encoding.cpp SMTEncoding.cpp \
	          sched-opt.c main.cpp \
						simian-core/verifier.cpp simian-core/print.cpp simian-core/smtenc.cpp












