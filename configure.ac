# Configure script for ISP

# =============================================================================
#                                   MACROS
# =============================================================================
# ==================== 
# SEARCH_MPIDIR
# ====================
# Find path of MPI directory. 
AC_DEFUN([SEARCH_MPIDIR], [

MPI_PATH=/usr/local$PATH_SEPARATOR/usr/local/mpich$PATH_SEPARATOR/usr$PATH_SEPARATOR/usr/mpich
AC_PATH_PROGS([mpi_dir],[bin/mpiexec bin/mpirun],[],${MPI_PATH})
mpi_dir=`echo ${mpi_dir} | sed -e 's,/bin/mpiexec$,,g' -e 's,/bin/mpirun$,,g'`

]) # end of SEARCH_MPIDIR

# ==================== 
# SEARCH_MPICC
# ====================
# Find name and path of MPI C compiler. 
AC_DEFUN([SEARCH_MPICC], [

MPI_PATH=$mpi_bin_dir$PATH_SEPARATOR$mpi_dir/bin$PATH_SEPARATOR$PATH$PATH_SEPARATOR/usr/local/bin$PATH_SEPARATOR/usr/local/mpich/bin$PATH_SEPARATOR/usr/bin$PATH_SEPARATOR/usr/mpich/bin
AC_PATH_PROGS([mpicc],[mpicc mpcc mpcc_r],[],${MPI_PATH})

]) # end of SEARCH_MPICC


# ==================== 
# SEARCH_MPICXX
# ====================
# Find name and path of MPI cxx compiler. 
AC_DEFUN([SEARCH_MPICXX], [

MPI_PATH=$mpi_bin_dir$PATH_SEPARATOR$mpi_dir/bin$PATH_SEPARATOR$PATH$PATH_SEPARATOR/usr/local/bin$PATH_SEPARATOR/usr/local/mpich/bin$PATH_SEPARATOR/usr/bin$PATH_SEPARATOR/usr/mpich/bin
AC_PATH_PROGS([mpicxx],[mpicxx mpiCC mpCC mpCC_r],[],${MPI_PATH})

]) # end of SEARCH_MPICXX


# ==================== 
# SEARCH_JAVA
# ====================

# Find the path of java, javac, and jar.
AC_DEFUN([SEARCH_JAVA], [

if test -z "${JAVA_HOME}" ; then
  JAVA_PATH=${PATH}
else
  ## try jre/bin first just in case we don't have full JDK
  JAVA_PATH=${JAVA_HOME}$PATH_SEPARATOR${JAVA_HOME}/jre/bin$PATH_SEPARATOR${JAVA_HOME}/bin$PATH_SEPARATOR${JAVA_HOME}/../bin$PATH_SEPARATOR${PATH}
fi
JAVA_PATH=${JAVA_PATH}$PATH_SEPARATOR/usr/java/bin$PATH_SEPARATOR/usr/jdk/bin$PATH_SEPARATOR/usr/lib/java/bin$PATH_SEPARATOR/usr/lib/jdk/bin$PATH_SEPARATOR/usr/local/java/bin$PATH_SEPARATOR/usr/local/jdk/bin$PATH_SEPARATOR/usr/local/lib/java/bin$PATH_SEPARATOR/usr/local/lib/jdk/bin
AC_PATH_PROG(java,java,[java],${JAVA_PATH})
AC_PATH_PROG(javac,javac,[javac],${JAVA_PATH})
AC_PATH_PROG(jar,jar,[jar],${JAVA_PATH})

]) #end of SEARCH_JAVA


# ===================== 
# CHECK_MPIC_COMPILE
# =====================

# Check compilation of MPI C program.
# Try with MPI_Finalize and also with PMPI_Finalize.
AC_DEFUN([CHECK_MPIC_COMPILE], [
AC_MSG_CHECKING([for compilation of an MPI C program])

old_CC=$CC
old_CFLAGS=$CFLAGS
old_LIBS=$LIBS

CC="$mpicc"
CFLAGS=""
LIBS=""

# Only test for MPI_Finalize; MPI_Init takes argc, argv,
# which configure-main doesn't provide
AC_LANG_PUSH(C)
AC_COMPILE_IFELSE(
  [AC_LANG_PROGRAM([#include "mpi.h"], 
                   [MPI_Finalize();],
                   [PMPI_Finalize();])], 
  [AC_MSG_RESULT([success])
   AC_DEFINE([HAVE_MPI], [1], 
             [Define if program for MPI could be compiled and linked])],
  [AC_MSG_ERROR([MPI C compiler not found; check paths for MPI package first...])])
AC_LANG_POP([C])

CC=$old_CC
CFLAGS=$old_CFLAGS
LIBS=$old_LIBS
]) # end of CHECK_MPIC_COMPILE


# ===================== 
# CHECK_MPICXX_COMPILE
# =====================

# Check compilation of MPI C++ program.
# Try with MPI_Finalize and also with PMPI_Finalize.
AC_DEFUN([CHECK_MPICXX_COMPILE], [
AC_MSG_CHECKING([for compilation of an MPI C++ program])

old_CXX=$CXX
old_CXXFLAGS=$CXXFLAGS
old_LIBS=$LIBS
CXX="$mpicxx"
CXXFLAGS=""
LIBS=""

# Only test for MPI_Finalize; MPI_Init takes argc, argv,
# which configure-main doesn't provide
success=1
AC_LANG_PUSH(C++)
AC_COMPILE_IFELSE(
  [AC_LANG_PROGRAM([#include "mpi.h"], 
                   [MPI_Finalize();],
                   [PMPI_Finalize();])], 
  [AC_MSG_RESULT([success])
   success=1],
  [AC_MSG_WARN([MPI C++ compiler not found...])
   success=0])
AC_LANG_POP([C++])
AM_CONDITIONAL([COND_MPICXX], [test "$success" = 1])

CXX=$old_CXX
CXXFLAGS=$old_CXXFLAGS
LIBS=$old_LIBS
]) # end of CHECK_MPICXX_COMPILE


# ==================== 
# AX_OPENMP
# ====================

# This is Steven G. Johnson's script to detect OpenMP. It is from:
# http://autoconf-archive.cryp.to/ax_openmp.html
AC_DEFUN([AX_OPENMP], [
AC_PREREQ(2.61) dnl for _AC_LANG_PREFIX

AC_CACHE_CHECK([for OpenMP flag of _AC_LANG compiler], ax_cv_[]_AC_LANG_ABBREV[]_openmp, [save[]_AC_LANG_PREFIX[]FLAGS=$[]_AC_LANG_PREFIX[]FLAGS
ax_cv_[]_AC_LANG_ABBREV[]_openmp=unknown
# Flags to try:  -fopenmp (gcc), -openmp (icc), -mp (SGI & PGI),
#                -xopenmp (Sun), -omp (Tru64), -qsmp=omp (AIX), none
ax_openmp_flags="-fopenmp -openmp -mp -xopenmp -omp -qsmp=omp none"
if test "x$OPENMP_[]_AC_LANG_PREFIX[]FLAGS" != x; then
  ax_openmp_flags="$OPENMP_[]_AC_LANG_PREFIX[]FLAGS $ax_openmp_flags"
fi
for ax_openmp_flag in $ax_openmp_flags; do
  case $ax_openmp_flag in
    none) []_AC_LANG_PREFIX[]FLAGS=$save[]_AC_LANG_PREFIX[] ;;
    *) []_AC_LANG_PREFIX[]FLAGS="$save[]_AC_LANG_PREFIX[]FLAGS $ax_openmp_flag" ;;
  esac
  AC_TRY_LINK_FUNC(omp_set_num_threads,
        [ax_cv_[]_AC_LANG_ABBREV[]_openmp=$ax_openmp_flag; break])
done
[]_AC_LANG_PREFIX[]FLAGS=$save[]_AC_LANG_PREFIX[]FLAGS
])
if test "x$ax_cv_[]_AC_LANG_ABBREV[]_openmp" = "xunknown"; then
  m4_default([$2],:)
else
  if test "x$ax_cv_[]_AC_LANG_ABBREV[]_openmp" != "xnone"; then
    OPENMP_[]_AC_LANG_PREFIX[]FLAGS=$ax_cv_[]_AC_LANG_ABBREV[]_openmp
  fi
  m4_default([$1], [AC_DEFINE(HAVE_OPENMP,1,[Define if OpenMP is enabled])])
  openmpExists=1
fi
])dnl AX_OPENMP


# Expand a variable all the way, so it can be used in .in files.
AC_DEFUN([AS_AC_EXPAND],
[
  EXP_VAR=[$1]
  FROM_VAR=[$2]

  dnl first expand prefix and exec_prefix if necessary
  prefix_save=$prefix
  exec_prefix_save=$exec_prefix

  dnl if no prefix given, then use /usr/local, the default prefix
  if test "x$prefix" = "xNONE"; then
    prefix="$ac_default_prefix"
  fi
  dnl if no exec_prefix given, then use prefix
  if test "x$exec_prefix" = "xNONE"; then
    exec_prefix=$prefix
  fi

  full_var="$FROM_VAR"
  dnl loop until it doesn't change anymore
  while true; do
    new_full_var="`eval echo $full_var`"
    if test "x$new_full_var" = "x$full_var"; then break; fi
    full_var=$new_full_var
  done

  dnl clean up
  full_var=$new_full_var
  AC_SUBST([$1], "$full_var")

  dnl restore prefix and exec_prefix
  prefix=$prefix_save
  exec_prefix=$exec_prefix_save
])

# =============================================================================
#                                 AUTOCONF STUFF
# =============================================================================

AC_PREREQ(2.61)
AC_INIT([isp],[0.2.0],[gauss@cs.utah.edu])
#AM_INIT_AUTOMAKE([isp], [0.2.0], [no-define])
AM_INIT_AUTOMAKE

AM_MAINTAINER_MODE
AC_CONFIG_SRCDIR([scheduler/Scheduler.cpp])
AC_CONFIG_SRCDIR([profiler/Profiler.c])
AC_CONFIG_SRCDIR([libopts/libopts.c])
AC_CONFIG_HEADER([libopts/config.h])
AC_CONFIG_HEADER([scheduler/config.hpp])

AC_PROG_LIBTOOL
AC_SUBST(LIBTOOL_DEPS)

AC_CONFIG_MACRO_DIR([m4])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_RANLIB

# Checks for header files.
AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS([arpa/inet.h netdb.h netinet/in.h stdlib.h string.h sys/socket.h sys/time.h unistd.h],
                 [],
                 [AC_MSG_ERROR([not found])])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_CONST
AC_C_INLINE
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_HEADER_TIME

# Checks for library functions.
AC_FUNC_FORK
AC_FUNC_MALLOC
AC_FUNC_SELECT_ARGTYPES
AC_TYPE_SIGNAL
AC_CHECK_FUNCS([gethostbyname memmove memset select socket strdup])
#=====================
#LIBOPTS
AC_DISABLE_SHARED
case "${enable_local_libopts+set}" in
 set) ;;
  *) enable_local_libopts=yes ;;
esac
case "${enable_libopts_install+set}" in
 set) ;;
 *) enable_libopts_install=no ;;
esac
LIBOPTS_CHECK

#=====================


# =============================================================================
#                            ALLOW USER TO SET COMPILER
# =============================================================================

# C compiler
AC_ARG_WITH([cc],
            AS_HELP_STRING([--with-cc=c_compiler],
                           [Give the path to C compiler [gcc]]),
            [CC="$withval"]) # default

# C++ compiler
AC_ARG_WITH([cxx],
            AS_HELP_STRING([--with-cxx=c++_compiler],
                           [Give the path to C++ compiler [g++]]),
            [CXX="$withval"]) # default

# =============================================================================
#                                   DETECT MPI
# =============================================================================

# MPI directory
#AC_MSG_CHECKING([for the MPI directory])
AC_ARG_WITH([mpi_dir],
            AS_HELP_STRING([--with-mpi-dir=MPIDIR],
                           [Give the path for MPI @<:@search in standard locations@:>@]),
            [AC_MSG_CHECKING([for the MPI directory]) # user option
             mpi_dir="$withval"
             AC_MSG_RESULT([$mpi_dir])],
            [SEARCH_MPIDIR])                          # default
#AC_MSG_RESULT([$mpi_dir])
MPIDIR=$mpi_dir
AC_SUBST([MPIDIR])

# MPI include directory (mpi.h)
AC_MSG_CHECKING([for the MPI include directory])
AC_ARG_WITH([mpi_inc_dir],
            AS_HELP_STRING([--with-mpi-inc-dir=MPI_INCDIR],
                           [Give the path for MPI-include-files @<:@MPIDIR/include@:>@]),
            [mpi_inc_dir="$withval"],         # user option
            [mpi_inc_dir="$mpi_dir/include"]) # default
if ![ test -f "$mpi_inc_dir/mpi.h" ] ; then
  AC_MSG_ERROR([mpi.h not found - try using --with-mpi-dir=.. or --with-mpi-inc-dir=.. options])
fi
AC_MSG_RESULT([$mpi_inc_dir])
MPI_INCDIR=$mpi_inc_dir
AC_SUBST([MPI_INCDIR])

# MPI bin directory
AC_MSG_CHECKING([for the MPI bin directory])
AC_ARG_WITH([mpi_bin_dir],
            AS_HELP_STRING([--with-mpi-bin-dir=MPI_BINDIR],
                           [Give the path for MPI-binaries @<:@MPIDIR/bin@:>@]),
            [mpi_bin_dir="$withval"],     # user option
            [mpi_bin_dir="$mpi_dir/bin"]) # default
AC_MSG_RESULT([$mpi_bin_dir])
MPI_BINDIR=$mpi_bin_dir
AC_SUBST([MPI_BINDIR])
AM_CONDITIONAL([COND_HAS_MPIEXEC], [test -e "$mpi_bin_dir/mpiexec"])

# MPI c compiler
#AC_MSG_CHECKING([for the MPI c compiler])
AC_ARG_WITH([mpicc],
            AS_HELP_STRING([--with-mpicc=MPICC],
                           [Give the path for the MPI c compiler @<:@detected automatically@:>@]),
            [AC_MSG_CHECKING([for the MPI c compiler]) # user option
             mpicc="$withval"
             AC_MSG_RESULT([$mpicc])],
            [SEARCH_MPICC])                            # default
#AC_MSG_RESULT([$mpicc])
MPICC=$mpicc
AC_SUBST([MPICC])

# MPI cxx compiler
#AC_MSG_CHECKING([for the MPI cxx compiler])
AC_ARG_WITH([mpicxx],
            AS_HELP_STRING([--with-mpicxx=MPICXX],
                           [Give the path for the MPI cxx compiler @<:@detected automatically@:>@]),
            [AC_MSG_CHECKING([for the MPI cxx compiler]) # user option
             mpicc="$withval"                   
             AC_MSG_RESULT([$mpicc])],
            [SEARCH_MPICXX])                             # default
#AC_MSG_RESULT([$mpicxx])
MPICXX=$mpicxx
AC_SUBST([MPICXX])

CHECK_MPIC_COMPILE
CHECK_MPICXX_COMPILE

# =============================================================================
#                         DETECT FUNCTION NAME VARIABLE
# =============================================================================

AC_MSG_CHECKING([for function name variable])
AC_LANG_PUSH(C)
AC_COMPILE_IFELSE(
  [AC_LANG_PROGRAM([const char* test = __PRETTY_FUNCTION__;])],
  [funcname=__PRETTY_FUNCTION__
   AC_MSG_RESULT([__PRETTY_FUNCTION__])],
  [AC_COMPILE_IFELSE(
    [AC_LANG_PROGRAM([const char* test = __FUNCTION__;])],
    [funcname=__FUNCTION__
     AC_MSG_RESULT([__FUNCTION__])],
    [AC_COMPILE_IFELSE(
      [AC_LANG_PROGRAM([const char* test = __func__;])],
      [funcname=__func__
       AC_MSG_RESULT([__func__])],
      [funcname=\"???\"
       AC_MSG_RESULT([none])])])])
AC_LANG_POP([C])
FUNCTION_NAME_VAR=$funcname
AC_SUBST([FUNCTION_NAME_VAR])

# =============================================================================
#                     DETECT FOR STATIC SCHEDULER COMPILATION
# =============================================================================

AC_MSG_CHECKING([for static linking option])
old_LDFLAGS=$LDFLAGS
LDFLAGS="$LDFLAGS -static"
AC_LANG_PUSH(C++)
AC_LINK_IFELSE(
  [AC_LANG_PROGRAM([int test = 0;])],
  [AC_MSG_RESULT([-static])
   success=1],
  [AC_MSG_RESULT([no])
   success=0])
AC_LANG_POP([C++])
AM_CONDITIONAL([COND_STATIC_LINK], [test "$success" = 1])
LDFLAGS=$old_LDFLAGS


# =============================================================================
#                                 DETECT OPENMP
# =============================================================================

AC_ARG_ENABLE([openmp],
              AS_HELP_STRING([--enable-openmp],
                             [Enable multi-threaded version using OpenMP.
                              In some cases (i.e. >= 16 cores) this might speed-up
                              ISP, but in all of our benchmarks it has only slowed
                              it down.]),
              [USE_OPENMP=1])

if test "${USE_OPENMP}"; then
    AX_OPENMP([AC_CHECK_HEADER(omp.h, [USE_OPENMP=1], [unset USE_OPENMP])], [unset USE_OPENMP])
    if test "${USE_OPENMP}"; then
        CFLAGS="$CFLAGS -DUSE_OPENMP $OPENMP_CFLAGS"
        CXXFLAGS="$CXXFLAGS -DUSE_OPENMP $OPENMP_CXXFLAGS"
    fi
fi


# =============================================================================
#                                 FIB ENABLED?
# =============================================================================

AC_ARG_ENABLE([fib],
              AS_HELP_STRING([--disable-fib],
                             [Disable the runtime option of FIB.]),
              [],
              [CXXFLAGS="$CXXFLAGS -DFIB"
               CFLAGS="$CFLAGS -DFIB"])

# =============================================================================
#                                 CONFIG_DEBUG_SCHED
# =============================================================================

AC_ARG_ENABLE([debug-sched],
              AS_HELP_STRING([--enable-debug-sched],
                             [Enable scheduler debug messages.]),
              [CXXFLAGS="$CXXFLAGS -DCONFIG_DEBUG_SCHED"
               CFLAGS="$CFLAGS -DCONFIG_DEBUG_SCHED"],
              [])

# =============================================================================
#                                 CONFIG_DEBUG_PROF
# =============================================================================

AC_ARG_ENABLE([debug-prof],
              AS_HELP_STRING([--enable-debug-prof],
                             [Enable profiler debug messages.]),
              [CXXFLAGS="$CXXFLAGS -DCONFIG_DEBUG_PROF"
               CFLAGS="$CFLAGS -DCONFIG_DEBUG_PROF"],
              [])

# =============================================================================
#                                 CONFIG_DEBUG
# =============================================================================

AC_ARG_ENABLE([debug],
              AS_HELP_STRING([--enable-debug],
                             [Enable scheduler and profiler debug messages.]),
              [CXXFLAGS="$CXXFLAGS -DCONFIG_DEBUG_PROF -DCONFIG_DEBUG_SCHED"
               CFLAGS="$CFLAGS -DCONFIG_DEBUG_PROF -DCONFIG_DEBUG_SCHED"],
              [])

# =============================================================================
#                                 CONFIG_OPTIONAL_AMPLE_SET_FIX
# =============================================================================

AC_ARG_ENABLE([optional-ample-set-fix],
              AS_HELP_STRING([--enable-optional-ample-set-fix],
                             [Make the ample set fix optional.]),
              [CXXFLAGS="$CXXFLAGS -DCONFIG_OPTIONAL_AMPLE_SET_FIX"
               CFLAGS="$CFLAGS -DCONFIG_OPTIONAL_AMPLE_SET_FIX"],
              [])

# =============================================================================
#                                 CONFIG_BOUNDED_MIXING
# =============================================================================

AC_ARG_ENABLE([bounded-mixing],
              AS_HELP_STRING([--enable-bounded-mixing],
                             [Enable vounded mixing.]),
              [CXXFLAGS="$CXXFLAGS -DCONFIG_BOUNDED_MIXING"
               CFLAGS="$CFLAGS -DCONFIG_BOUNDED_MIXING"],
              [])

# =============================================================================
#                                 GUI / JAVA
# =============================================================================

AC_ARG_ENABLE([gui],
              AS_HELP_STRING([--disable-gui],
                             [Disable the Java GUI.]),
              [USE_GUI=0],
              [USE_GUI=1])
if test "${USE_GUI}"; then

    # Find JAVA Executable
    AC_ARG_WITH([java],
                AS_HELP_STRING([--with-java=JAVAPATH],
                               [Give the path for the JAVA executable @<:@automatic search@:>@]),
                [AC_MSG_CHECKING([for java])    # user option
                 java="$withval"
                 javac="javac"
                 jar="jar"
                 AC_MSG_RESULT([$java])],
                [SEARCH_JAVA])                  # default

    if ![ test -f "$java" ]; then
      USE_GUI=0
      AC_MSG_WARN([java not detected, UI disabled])
    else
      JAVA=$java
      AC_SUBST([JAVA])
      JAVAC=$javac
      AC_SUBST([JAVAC])
      JAR=$jar
      AC_SUBST([JAR])
    fi

fi
AM_CONDITIONAL([COND_GUI], [test "$USE_GUI" = 1])
AM_CONDITIONAL([COND_COMPILE_JAVA], [test -e "UI/UI/ispMainUI.java"])

# =============================================================================
#                                     ISP-TESTS
# =============================================================================

# Create if exists.
#if test -d "isp-tests" ; then
#    AC_CONFIG_FILES([isp-tests/run], [chmod +x isp-tests/run])
#    AC_CONFIG_FILES([isp-tests/mpi.h])
#fi
AM_CONDITIONAL([COND_TESTS], [test -d "isp-tests"])

AS_AC_EXPAND(BINDIR, $bindir)
AS_AC_EXPAND(LIBDIR, $libdir)
AS_AC_EXPAND(include_ispdir, $includedir)
include_ispdir=${include_ispdir}/isp
AS_AC_EXPAND(INCDIR, $includedir)
INCDIR=${INCDIR}/isp

AC_CONFIG_FILES([Makefile
          scheduler/Makefile
          profiler/Makefile
          UI/Makefile
          scripts/Makefile
          scripts/isp
          scripts/ispcc2
          scripts/ispCC
	  scripts/mopper	
	  scripts/simian
          scripts/ispcxx
          scripts/ispUI
          scripts/mpi.h
          scripts/assert.h])

AC_CONFIG_COMMANDS(ispcc,
                   [rm -f scripts/ispcc; mv scripts/ispcc2 scripts/ispcc])

AC_OUTPUT
