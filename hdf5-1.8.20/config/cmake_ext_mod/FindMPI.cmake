# Distributed under the OSI-approved BSD 3-Clause License.  See https://cmake.org/licensing for details.

#.rst:
# FindMPI
# -------
#
# Find a Message Passing Interface (MPI) implementation.
#
# The Message Passing Interface (MPI) is a library used to write
# high-performance distributed-memory parallel applications, and is
# typically deployed on a cluster.  MPI is a standard interface (defined
# by the MPI forum) for which many implementations are available.
#
# Variables for using MPI
# ^^^^^^^^^^^^^^^^^^^^^^^
#
# The module exposes the components ``C``, ``CXX``, ``MPICXX`` and ``Fortran``.
# Each of these controls the various MPI languages to search for.
# The difference between ``CXX`` and ``MPICXX`` is that ``CXX`` refers to the
# MPI C API being usable from C++, whereas ``MPICXX`` refers to the MPI-2 C++ API
# that was removed again in MPI-3.
#
# Depending on the enabled components the following variables will be set:
#
# ``MPI_FOUND``
#   Variable indicating that MPI settings for all requested languages have been found.
#   If no components are specified, this is true if MPI settings for all enabled languages
#   were detected. Note that the ``MPICXX`` component does not affect this variable.
# ``MPI_VERSION``
#   Minimal version of MPI detected among the requested languages, or all enabled languages
#   if no components were specified.
#
# This module will set the following variables per language in your
# project, where ``<lang>`` is one of C, CXX, or Fortran:
#
# ``MPI_<lang>_FOUND``
#   Variable indicating the MPI settings for ``<lang>`` were found and that
#   simple MPI test programs compile with the provided settings.
# ``MPI_<lang>_COMPILER``
#   MPI compiler for ``<lang>`` if such a program exists.
# ``MPI_<lang>_COMPILE_OPTIONS``
#   Compilation options for MPI programs in ``<lang>``, given as a :ref:`;-list <CMake Language Lists>`.
# ``MPI_<lang>_COMPILE_DEFINITIONS``
#   Compilation definitions for MPI programs in ``<lang>``, given as a :ref:`;-list <CMake Language Lists>`.
# ``MPI_<lang>_INCLUDE_DIRS``
#   Include path(s) for MPI header.
# ``MPI_<lang>_LINK_FLAGS``
#   Linker flags for MPI programs.
# ``MPI_<lang>_LIBRARIES``
#   All libraries to link MPI programs against.
#
# Additionally, the following :prop_tgt:`IMPORTED` targets are defined:
#
# ``MPI::MPI_<lang>``
#   Target for using MPI from ``<lang>``.
#
# The following variables indicating which bindings are present will be defined:
#
# ``MPI_MPICXX_FOUND``
#   Variable indicating whether the MPI-2 C++ bindings are present (introduced in MPI-2, removed with MPI-3).
# ``MPI_Fortran_HAVE_F77_HEADER``
#   True if the Fortran 77 header ``mpif.h`` is available.
# ``MPI_Fortran_HAVE_F90_MODULE``
#   True if the Fortran 90 module ``mpi`` can be used for accessing MPI (MPI-2 and higher only).
# ``MPI_Fortran_HAVE_F08_MODULE``
#   True if the Fortran 2008 ``mpi_f08`` is available to MPI programs (MPI-3 and higher only).
#
# If possible, the MPI version will be determined by this module. The facilities to detect the MPI version
# were introduced with MPI-1.2, and therefore cannot be found for older MPI versions.
#
# ``MPI_<lang>_VERSION_MAJOR``
#   Major version of MPI implemented for ``<lang>`` by the MPI distribution.
# ``MPI_<lang>_VERSION_MINOR``
#   Minor version of MPI implemented for ``<lang>`` by the MPI distribution.
# ``MPI_<lang>_VERSION``
#   MPI version implemented for ``<lang>`` by the MPI distribution.
#
# Note that there's no variable for the C bindings being accessible through ``mpi.h``, since the MPI standards
# always have required this binding to work in both C and C++ code.
#
# For running MPI programs, the module sets the following variables
#
# ``MPIEXEC_EXECUTABLE``
#   Executable for running MPI programs, if such exists.
# ``MPIEXEC_NUMPROC_FLAG``
#   Flag to pass to ``mpiexec`` before giving it the number of processors to run on.
# ``MPIEXEC_MAX_NUMPROCS``
#   Number of MPI processors to utilize. Defaults to the number
#   of processors detected on the host system.
# ``MPIEXEC_PREFLAGS``
#   Flags to pass to ``mpiexec`` directly before the executable to run.
# ``MPIEXEC_POSTFLAGS``
#   Flags to pass to ``mpiexec`` after other flags.
#
# Variables for locating MPI
# ^^^^^^^^^^^^^^^^^^^^^^^^^^
#
# This module performs a three step search for an MPI implementation:
#
# 1. Check if the compiler has MPI support built-in. This is the case if the user passed a
#    compiler wrapper as ``CMAKE_<LANG>_COMPILER`` or if they're on a Cray system.
# 2. Attempt to find an MPI compiler wrapper and determine the compiler information from it.
# 3. Try to find an MPI implementation that does not ship such a wrapper by guessing settings.
#    Currently, only Microsoft MPI and MPICH2 on Windows are supported.
#
# For controlling the second step, the following variables may be set:
#
# ``MPI_<lang>_COMPILER``
#   Search for the specified compiler wrapper and use it.
# ``MPI_<lang>_COMPILER_FLAGS``
#   Flags to pass to the MPI compiler wrapper during interrogation. Some compiler wrappers
#   support linking debug or tracing libraries if a specific flag is passed and this variable
#   may be used to obtain them.
# ``MPI_COMPILER_FLAGS``
#   Used to initialize ``MPI_<lang>_COMPILER_FLAGS`` if no language specific flag has been given.
#   Empty by default.
# ``MPI_EXECUTABLE_SUFFIX``
#   A suffix which is appended to all names that are being looked for. For instance you may set this
#   to ``.mpich`` or ``.openmpi`` to prefer the one or the other on Debian and its derivatives.
#
# In order to control the guessing step, the following variable may be set:
#
# ``MPI_GUESS_LIBRARY_NAME``
#   Valid values are ``MSMPI`` and ``MPICH2``. If set, only the given library will be searched for.
#   By default, ``MSMPI`` will be preferred over ``MPICH2`` if both are available.
#   This also sets ``MPI_SKIP_COMPILER_WRAPPER`` to ``true``, which may be overridden.
#
# Each of the search steps may be skipped with the following control variables:
#
# ``MPI_ASSUME_NO_BUILTIN_MPI``
#   If true, the module assumes that the compiler itself does not provide an MPI implementation and
#   skips to step 2.
# ``MPI_SKIP_COMPILER_WRAPPER``
#   If true, no compiler wrapper will be searched for.
# ``MPI_SKIP_GUESSING``
#   If true, the guessing step will be skipped.
#
# Additionally, the following control variable is available to change search behavior:
#
# ``MPI_CXX_SKIP_MPICXX``
#   Add some definitions that will disable the MPI-2 C++ bindings.
#   Currently supported are MPICH, Open MPI, Platform MPI and derivatives thereof,
#   for example MVAPICH or Intel MPI.
#
# If the find procedure fails for a variable ``MPI_<lang>_WORKS``, then the settings detected by or passed to
# the module did not work and even a simple MPI test program failed to compile.
#
# If all of these parameters were not sufficient to find the right MPI implementation, a user may
# disable the entire autodetection process by specifying both a list of libraries in ``MPI_<lang>_LIBRARIES``
# and a list of include directories in ``MPI_<lang>_ADDITIONAL_INCLUDE_DIRS``.
# Any other variable may be set in addition to these two. The module will then validate the MPI settings and store the
# settings in the cache.
#
# Cache variables for MPI
# ^^^^^^^^^^^^^^^^^^^^^^^
#
# The variable ``MPI_<lang>_INCLUDE_DIRS`` will be assembled from the following variables.
# For C and CXX:
#
# ``MPI_<lang>_HEADER_DIR``
#   Location of the ``mpi.h`` header on disk.
#
# For Fortran:
#
# ``MPI_Fortran_F77_HEADER_DIR``
#   Location of the Fortran 77 header ``mpif.h``, if it exists.
# ``MPI_Fortran_MODULE_DIR``
#   Location of the ``mpi`` or ``mpi_f08`` modules, if available.
#
# For all languages the following variables are additionally considered:
#
# ``MPI_<lang>_ADDITIONAL_INCLUDE_DIRS``
#   A :ref:`;-list <CMake Language Lists>` of paths needed in addition to the normal include directories.
# ``MPI_<include_name>_INCLUDE_DIR``
#   Path variables for include folders referred to by ``<include_name>``.
# ``MPI_<lang>_ADDITIONAL_INCLUDE_VARS``
#   A :ref:`;-list <CMake Language Lists>` of ``<include_name>`` that will be added to the include locations of ``<lang>``.
#
# The variable ``MPI_<lang>_LIBRARIES`` will be assembled from the following variables:
#
# ``MPI_<lib_name>_LIBRARY``
#   The location of a library called ``<lib_name>`` for use with MPI.
# ``MPI_<lang>_LIB_NAMES``
#   A :ref:`;-list <CMake Language Lists>` of ``<lib_name>`` that will be added to the include locations of ``<lang>``.
#
# Usage of mpiexec
# ^^^^^^^^^^^^^^^^
#
# When using ``MPIEXEC_EXECUTABLE`` to execute MPI applications, you should typically
# use all of the ``MPIEXEC_EXECUTABLE`` flags as follows:
#
# ::
#
#    ${MPIEXEC_EXECUTABLE} ${MPIEXEC_NUMPROC_FLAG} ${MPIEXEC_MAX_NUMPROCS}
#      ${MPIEXEC_PREFLAGS} EXECUTABLE ${MPIEXEC_POSTFLAGS} ARGS
#
# where ``EXECUTABLE`` is the MPI program, and ``ARGS`` are the arguments to
# pass to the MPI program.
#
# Advanced variables for using MPI
# ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
#
# The module can perform some advanced feature detections upon explicit request.
#
# **Important notice:** The following checks cannot be performed without *executing* an MPI test program.
# Consider the special considerations for the behavior of :command:`try_run` during cross compilation.
# Moreover, running an MPI program can cause additional issues, like a firewall notification on some systems.
# You should only enable these detections if you absolutely need the information.
#
# If the following variables are set to true, the respective search will be performed:
#
# ``MPI_DETERMINE_Fortran_CAPABILITIES``
#   Determine for all available Fortran bindings what the values of ``MPI_SUBARRAYS_SUPPORTED`` and
#   ``MPI_ASYNC_PROTECTS_NONBLOCKING`` are and make their values available as ``MPI_Fortran_<binding>_SUBARRAYS``
#   and ``MPI_Fortran_<binding>_ASYNCPROT``, where ``<binding>`` is one of ``F77_HEADER``, ``F90_MODULE`` and
#   ``F08_MODULE``.
# ``MPI_DETERMINE_LIBRARY_VERSION``
#   For each language, find the output of ``MPI_Get_library_version`` and make it available as ``MPI_<lang>_LIBRARY_VERSION``.
#   This information is usually tied to the runtime component of an MPI implementation and might differ depending on ``<lang>``.
#   Note that the return value is entirely implementation defined. This information might be used to identify
#   the MPI vendor and for example pick the correct one of multiple third party binaries that matches the MPI vendor.
#
# Backward Compatibility
# ^^^^^^^^^^^^^^^^^^^^^^
#
# For backward compatibility with older versions of FindMPI, these
# variables are set, but deprecated:
#
# ::
#
#    MPI_COMPILER        MPI_LIBRARY        MPI_EXTRA_LIBRARY
#    MPI_COMPILE_FLAGS   MPI_INCLUDE_PATH   MPI_LINK_FLAGS
#    MPI_LIBRARIES
#
# In new projects, please use the ``MPI_<lang>_XXX`` equivalents.
# Additionally, the following variables are deprecated:
#
# ``MPI_<lang>_COMPILE_FLAGS``
#   Use ``MPI_<lang>_COMPILE_OPTIONS`` and ``MPI_<lang>_COMPILE_DEFINITIONS`` instead.
# ``MPI_<lang>_INCLUDE_PATH``
#   For consumption use ``MPI_<lang>_INCLUDE_DIRS`` and for specifying folders use ``MPI_<lang>_ADDITIONAL_INCLUDE_DIRS`` instead.
# ``MPIEXEC``
#   Use ``MPIEXEC_EXECUTABLE`` instead.

cmake_policy(PUSH)
cmake_policy(SET CMP0057 NEW) # if IN_LIST

# include this to handle the QUIETLY and REQUIRED arguments
include(${CMAKE_ROOT}/Modules/FindPackageHandleStandardArgs.cmake)
include(GetPrerequisites)

# Generic compiler names
set(_MPI_C_GENERIC_COMPILER_NAMES          mpicc    mpcc      mpicc_r mpcc_r)
set(_MPI_CXX_GENERIC_COMPILER_NAMES        mpicxx   mpiCC     mpcxx   mpCC    mpic++   mpc++
                                           mpicxx_r mpiCC_r   mpcxx_r mpCC_r  mpic++_r mpc++_r)
set(_MPI_Fortran_GENERIC_COMPILER_NAMES    mpif95   mpif95_r  mpf95   mpf95_r
                                           mpif90   mpif90_r  mpf90   mpf90_r
                                           mpif77   mpif77_r  mpf77   mpf77_r
                                           mpifc)

# GNU compiler names
set(_MPI_GNU_C_COMPILER_NAMES              mpigcc mpgcc mpigcc_r mpgcc_r)
set(_MPI_GNU_CXX_COMPILER_NAMES            mpig++ mpg++ mpig++_r mpg++_r mpigxx)
set(_MPI_GNU_Fortran_COMPILER_NAMES        mpigfortran mpgfortran mpigfortran_r mpgfortran_r
                                           mpig77 mpig77_r mpg77 mpg77_r)

# Intel MPI compiler names on Windows
if(WIN32)
  list(APPEND _MPI_C_GENERIC_COMPILER_NAMES       mpicc.bat)
  list(APPEND _MPI_CXX_GENERIC_COMPILER_NAMES     mpicxx.bat)
  list(APPEND _MPI_Fortran_GENERIC_COMPILER_NAMES mpifc.bat)

  # Intel MPI compiler names
  set(_MPI_Intel_C_COMPILER_NAMES            mpiicc.bat)
  set(_MPI_Intel_CXX_COMPILER_NAMES          mpiicpc.bat)
  set(_MPI_Intel_Fortran_COMPILER_NAMES      mpiifort.bat mpif77.bat mpif90.bat)

  # Intel MPI compiler names for MSMPI
  set(_MPI_MSVC_C_COMPILER_NAMES             mpicl.bat)
  set(_MPI_MSVC_CXX_COMPILER_NAMES           mpicl.bat)
else()
  # Intel compiler names
  set(_MPI_Intel_C_COMPILER_NAMES            mpiicc)
  set(_MPI_Intel_CXX_COMPILER_NAMES          mpiicpc  mpiicxx mpiic++)
  set(_MPI_Intel_Fortran_COMPILER_NAMES      mpiifort mpiif95 mpiif90 mpiif77)
endif()

# PGI compiler names
set(_MPI_PGI_C_COMPILER_NAMES              mpipgcc mppgcc)
set(_MPI_PGI_CXX_COMPILER_NAMES            mpipgCC mppgCC)
set(_MPI_PGI_Fortran_COMPILER_NAMES        mpipgf95 mpipgf90 mppgf95 mppgf90 mpipgf77 mppgf77)

# XLC MPI Compiler names
set(_MPI_XL_C_COMPILER_NAMES               mpxlc      mpxlc_r    mpixlc     mpixlc_r)
set(_MPI_XL_CXX_COMPILER_NAMES             mpixlcxx   mpixlC     mpixlc++   mpxlcxx   mpxlc++   mpixlc++   mpxlCC
                                           mpixlcxx_r mpixlC_r   mpixlc++_r mpxlcxx_r mpxlc++_r mpixlc++_r mpxlCC_r)
set(_MPI_XL_Fortran_COMPILER_NAMES         mpixlf95   mpixlf95_r mpxlf95 mpxlf95_r
                                           mpixlf90   mpixlf90_r mpxlf90 mpxlf90_r
                                           mpixlf77   mpixlf77_r mpxlf77 mpxlf77_r
                                           mpixlf     mpixlf_r   mpxlf   mpxlf_r)

# Prepend vendor-specific compiler wrappers to the list. If we don't know the compiler,
# attempt all of them.
# By attempting vendor-specific compiler names first, we should avoid situations where the compiler wrapper
# stems from a proprietary MPI and won't know which compiler it's being used for. For instance, Intel MPI
# controls its settings via the I_MPI_CC environment variables if the generic name is being used.
# If we know which compiler we're working with, we can use the most specialized wrapper there is in order to
# pick up the right settings for it.
foreach (LANG IN ITEMS C CXX Fortran)
  set(_MPI_${LANG}_COMPILER_NAMES "")
  foreach (id IN ITEMS GNU Intel MSVC PGI XL)
    if (NOT CMAKE_${LANG}_COMPILER_ID OR CMAKE_${LANG}_COMPILER_ID STREQUAL id)
      list(APPEND _MPI_${LANG}_COMPILER_NAMES ${_MPI_${id}_${LANG}_COMPILER_NAMES}${MPI_EXECUTABLE_SUFFIX})
    endif()
    unset(_MPI_${id}_${LANG}_COMPILER_NAMES)
  endforeach()
  list(APPEND _MPI_${LANG}_COMPILER_NAMES ${_MPI_${LANG}_GENERIC_COMPILER_NAMES}${MPI_EXECUTABLE_SUFFIX})
  unset(_MPI_${LANG}_GENERIC_COMPILER_NAMES)
endforeach()

# Names to try for mpiexec
# Only mpiexec commands are guaranteed to behave as described in the standard,
# mpirun commands are not covered by the standard in any way whatsoever.
# lamexec is the executable for LAM/MPI, srun is for SLURM or Open MPI with SLURM support.
# srun -n X <executable> is however a valid command, so it behaves 'like' mpiexec.
set(_MPIEXEC_NAMES_BASE                   mpiexec mpiexec.hydra mpiexec.mpd mpirun lamexec srun)

unset(_MPIEXEC_NAMES)
foreach(_MPIEXEC_NAME IN LISTS _MPIEXEC_NAMES_BASE)
  list(APPEND _MPIEXEC_NAMES "${_MPIEXEC_NAME}${MPI_EXECUTABLE_SUFFIX}")
endforeach()
unset(_MPIEXEC_NAMES_BASE)

function (_MPI_check_compiler LANG QUERY_FLAG OUTPUT_VARIABLE RESULT_VARIABLE)
  if(DEFINED MPI_${LANG}_COMPILER_FLAGS)
    separate_arguments(_MPI_COMPILER_WRAPPER_OPTIONS NATIVE_COMMAND "${MPI_${LANG}_COMPILER_FLAGS}")
  else()
    separate_arguments(_MPI_COMPILER_WRAPPER_OPTIONS NATIVE_COMMAND "${MPI_COMPILER_FLAGS}")
  endif()
  execute_process(
    COMMAND ${MPI_${LANG}_COMPILER} ${_MPI_COMPILER_WRAPPER_OPTIONS} ${QUERY_FLAG}
    OUTPUT_VARIABLE  WRAPPER_OUTPUT OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_VARIABLE   WRAPPER_OUTPUT ERROR_STRIP_TRAILING_WHITESPACE
    RESULT_VARIABLE  WRAPPER_RETURN)
  # Some compiler wrappers will yield spurious zero return values, for example
  # Intel MPI tolerates unknown arguments and if the MPI wrappers loads a shared
  # library that has invalid or missing version information there would be warning
  # messages emitted by ld.so in the compiler output. In either case, we'll treat
  # the output as invalid.
  if("${WRAPPER_OUTPUT}" MATCHES "undefined reference|unrecognized|need to set|no version information available")
    set(WRAPPER_RETURN 255)
  endif()
  # Ensure that no error output might be passed upwards.
  if(NOT WRAPPER_RETURN EQUAL 0)
    unset(WRAPPER_OUTPUT)
  endif()
  set(${OUTPUT_VARIABLE} "${WRAPPER_OUTPUT}" PARENT_SCOPE)
  set(${RESULT_VARIABLE} "${WRAPPER_RETURN}" PARENT_SCOPE)
endfunction()

function (_MPI_interrogate_compiler lang)
  unset(MPI_COMPILE_CMDLINE)
  unset(MPI_LINK_CMDLINE)

  unset(MPI_COMPILE_OPTIONS_WORK)
  unset(MPI_COMPILE_DEFINITIONS_WORK)
  unset(MPI_INCLUDE_DIRS_WORK)
  unset(MPI_LINK_FLAGS_WORK)
  unset(MPI_LIB_NAMES_WORK)
  unset(MPI_LIB_FULLPATHS_WORK)

  # Check whether the -showme:compile option works. This indicates that we have either Open MPI
  # or a newer version of LAM/MPI, and implies that -showme:link will also work.
  # Open MPI also supports -show, but separates linker and compiler information
  _MPI_check_compiler(${LANG} "-showme:compile" MPI_COMPILE_CMDLINE MPI_COMPILER_RETURN)
  if (MPI_COMPILER_RETURN EQUAL 0)
    _MPI_check_compiler(${LANG} "-showme:link" MPI_LINK_CMDLINE MPI_COMPILER_RETURN)

    if (NOT MPI_COMPILER_RETURN EQUAL 0)
      unset(MPI_COMPILE_CMDLINE)
    endif()
  endif()

  # MPICH and MVAPICH offer -compile-info and -link-info.
  # For modern versions, both do the same as -show. However, for old versions, they do differ
  # when called for mpicxx and mpif90 and it's necessary to use them over -show in order to find the
  # removed MPI C++ bindings.
  if (NOT MPI_COMPILER_RETURN EQUAL 0)
    _MPI_check_compiler(${LANG} "-compile-info" MPI_COMPILE_CMDLINE MPI_COMPILER_RETURN)

    if (MPI_COMPILER_RETURN EQUAL 0)
      _MPI_check_compiler(${LANG} "-link-info" MPI_LINK_CMDLINE MPI_COMPILER_RETURN)

      if (NOT MPI_COMPILER_RETURN EQUAL 0)
        unset(MPI_COMPILE_CMDLINE)
      endif()
    endif()
  endif()

  # MPICH, MVAPICH2 and Intel MPI just use "-show". Open MPI also offers this, but the
  # -showme commands are more specialized.
  if (NOT MPI_COMPILER_RETURN EQUAL 0)
    _MPI_check_compiler(${LANG} "-show" MPI_COMPILE_CMDLINE MPI_COMPILER_RETURN)
  endif()

  # Older versions of LAM/MPI have "-showme". Open MPI also supports this.
  # Unknown to MPICH, MVAPICH and Intel MPI.
  if (NOT MPI_COMPILER_RETURN EQUAL 0)
    _MPI_check_compiler(${LANG} "-showme" MPI_COMPILE_CMDLINE MPI_COMPILER_RETURN)
  endif()

  if (NOT (MPI_COMPILER_RETURN EQUAL 0) OR NOT (DEFINED MPI_COMPILE_CMDLINE))
    # Cannot interrogate this compiler, so exit.
    set(MPI_${LANG}_WRAPPER_FOUND FALSE PARENT_SCOPE)
    return()
  endif()
  unset(MPI_COMPILER_RETURN)

  # We have our command lines, but we might need to copy MPI_COMPILE_CMDLINE
  # into MPI_LINK_CMDLINE, if we didn't find the link line.
  if (NOT DEFINED MPI_LINK_CMDLINE)
    set(MPI_LINK_CMDLINE "${MPI_COMPILE_CMDLINE}")
  endif()

  # At this point, we obtained some output from a compiler wrapper that works.
  # We'll now try to parse it into variables with meaning to us.
  if("${LANG}" STREQUAL "Fortran")
    # Some MPICH-1 and MVAPICH-1 versions return a three command answer for Fortran, consisting
    # out of a symlink command for mpif.h, the actual compiler command and a deletion of the
    # created symlink. We need to detect that case, remember the include path and drop the
    # symlink/deletion operation to obtain the link/compile lines we'd usually expect.
    if("${MPI_COMPILE_CMDLINE}" MATCHES "^ln -s ([^\" ]+|\"[^\"]+\") mpif.h")
      get_filename_component(MPI_INCLUDE_DIRS_WORK "${CMAKE_MATCH_1}" DIRECTORY)
      string(REGEX REPLACE "^ln -s ([^\" ]+|\"[^\"]+\") mpif.h\n" "" MPI_COMPILE_CMDLINE "${MPI_COMPILE_CMDLINE}")
      string(REGEX REPLACE "^ln -s ([^\" ]+|\"[^\"]+\") mpif.h\n" "" MPI_LINK_CMDLINE "${MPI_LINK_CMDLINE}")
      string(REGEX REPLACE "\nrm -f mpif.h$" "" MPI_COMPILE_CMDLINE "${MPI_COMPILE_CMDLINE}")
      string(REGEX REPLACE "\nrm -f mpif.h$" "" MPI_LINK_CMDLINE "${MPI_LINK_CMDLINE}")
    endif()
  endif()

  # The Intel MPI wrapper on Linux will emit some objcopy commands after its compile command
  # if -static_mpi was passed to the wrapper. To avoid spurious matches, we need to drop these lines.
  if(UNIX)
    string(REGEX REPLACE "(^|\n)objcopy[^\n]+(\n|$)" "" MPI_COMPILE_CMDLINE "${MPI_COMPILE_CMDLINE}")
    string(REGEX REPLACE "(^|\n)objcopy[^\n]+(\n|$)" "" MPI_LINK_CMDLINE "${MPI_LINK_CMDLINE}")
  endif()

  # Extract compile options from the compile command line.
  string(REGEX MATCHALL "(^| )-f([^\" ]+|\"[^\"]+\")" MPI_ALL_COMPILE_OPTIONS "${MPI_COMPILE_CMDLINE}")

  foreach(_MPI_COMPILE_OPTION IN LISTS MPI_ALL_COMPILE_OPTIONS)
    string(REGEX REPLACE "^ " "" _MPI_COMPILE_OPTION "${_MPI_COMPILE_OPTION}")
    # Ignore -fstack-protector directives: These occur on MPICH and MVAPICH when the libraries
    # themselves were built with this flag. However, this flag is unrelated to using MPI, and
    # we won't match the accompanying --param-ssp-size and -Wp,-D_FORTIFY_SOURCE flags and therefore
    # produce inconsistent results with the regularly flags.
    # Similarly, aliasing flags do not belong into our flag array.
    if(NOT "${_MPI_COMPILE_OPTION}" MATCHES "^-f(stack-protector|(no-|)strict-aliasing|PI[CE]|pi[ce])")
      list(APPEND MPI_COMPILE_OPTIONS_WORK "${_MPI_COMPILE_OPTION}")
    endif()
  endforeach()

  # Same deal, with the definitions. We also treat arguments passed to the preprocessor directly.
  string(REGEX MATCHALL "(^| )(-Wp,|-Xpreprocessor |)[-/]D([^\" ]+|\"[^\"]+\")" MPI_ALL_COMPILE_DEFINITIONS "${MPI_COMPILE_CMDLINE}")

  foreach(_MPI_COMPILE_DEFINITION IN LISTS MPI_ALL_COMPILE_DEFINITIONS)
    string(REGEX REPLACE "^ ?(-Wp,|-Xpreprocessor )?[-/]D" "" _MPI_COMPILE_DEFINITION "${_MPI_COMPILE_DEFINITION}")
    string(REPLACE "\"" "" _MPI_COMPILE_DEFINITION "${_MPI_COMPILE_DEFINITION}")
    if(NOT "${_MPI_COMPILE_DEFINITION}" MATCHES "^_FORTIFY_SOURCE.*")
      list(APPEND MPI_COMPILE_DEFINITIONS_WORK "${_MPI_COMPILE_DEFINITION}")
    endif()
  endforeach()

  # Extract include paths from compile command line
  string(REGEX MATCHALL "(^| )[-/]I([^\" ]+|\"[^\"]+\")" MPI_ALL_INCLUDE_PATHS "${MPI_COMPILE_CMDLINE}")

  # If extracting failed to work, we'll try using -showme:incdirs.
  if (NOT MPI_ALL_INCLUDE_PATHS)
    _MPI_check_compiler(${LANG} "-showme:incdirs" MPI_INCDIRS_CMDLINE MPI_INCDIRS_COMPILER_RETURN)
    if(MPI_INCDIRS_COMPILER_RETURN)
      separate_arguments(MPI_ALL_INCLUDE_PATHS NATIVE_COMMAND "${MPI_INCDIRS_CMDLINE}")
    endif()
  endif()

  foreach(_MPI_INCLUDE_PATH IN LISTS MPI_ALL_INCLUDE_PATHS)
    string(REGEX REPLACE "^ ?[-/]I" "" _MPI_INCLUDE_PATH "${_MPI_INCLUDE_PATH}")
    string(REPLACE "\"" "" _MPI_INCLUDE_PATH "${_MPI_INCLUDE_PATH}")
    get_filename_component(_MPI_INCLUDE_PATH "${_MPI_INCLUDE_PATH}" REALPATH)
    list(APPEND MPI_INCLUDE_DIRS_WORK "${_MPI_INCLUDE_PATH}")
  endforeach()

  # Extract linker paths from the link command line
  string(REGEX MATCHALL "(^| )(-Wl,|-Xlinker |)(-L|[/-]LIBPATH:|[/-]libpath:)([^\" ]+|\"[^\"]+\")" MPI_ALL_LINK_PATHS "${MPI_LINK_CMDLINE}")

  # If extracting failed to work, we'll try using -showme:libdirs.
  if (NOT MPI_ALL_LINK_PATHS)
    _MPI_check_compiler(${LANG} "-showme:libdirs" MPI_LIBDIRS_CMDLINE MPI_LIBDIRS_COMPILER_RETURN)
    if(MPI_LIBDIRS_COMPILER_RETURN)
      separate_arguments(MPI_ALL_LINK_PATHS NATIVE_COMMAND "${MPI_LIBDIRS_CMDLINE}")
    endif()
  endif()

  foreach(_MPI_LPATH IN LISTS MPI_ALL_LINK_PATHS)
    string(REGEX REPLACE "^ ?(-Wl,|-Xlinker )?(-L|[/-]LIBPATH:|[/-]libpath:)" "" _MPI_LPATH "${_MPI_LPATH}")
    string(REPLACE "\"" "" _MPI_LPATH "${_MPI_LPATH}")
    get_filename_component(_MPI_LPATH "${_MPI_LPATH}" REALPATH)
    list(APPEND MPI_LINK_DIRECTORIES_WORK "${_MPI_LPATH}")
  endforeach()

  # Extract linker flags from the link command line
  string(REGEX MATCHALL "(^| )(-Wl,|-Xlinker )([^\" ]+|\"[^\"]+\")" MPI_ALL_LINK_FLAGS "${MPI_LINK_CMDLINE}")

  foreach(_MPI_LINK_FLAG IN LISTS MPI_ALL_LINK_FLAGS)
    string(STRIP "${_MPI_LINK_FLAG}" _MPI_LINK_FLAG)
    # MPI might be marked to build with non-executable stacks but this should not propagate.
    if (NOT "${_MPI_LINK_FLAG}" MATCHES "(-Wl,|-Xlinker )-z,noexecstack")
      if (MPI_LINK_FLAGS_WORK)
        string(APPEND MPI_LINK_FLAGS_WORK " ${_MPI_LINK_FLAG}")
      else()
        set(MPI_LINK_FLAGS_WORK "${_MPI_LINK_FLAG}")
      endif()
    endif()
  endforeach()

  # Extract the set of libraries to link against from the link command
  # line
  string(REGEX MATCHALL "(^| )-l([^\" ]+|\"[^\"]+\")" MPI_LIBNAMES "${MPI_LINK_CMDLINE}")

  foreach(_MPI_LIB_NAME IN LISTS MPI_LIBNAMES)
    string(REGEX REPLACE "^ ?-l" "" _MPI_LIB_NAME "${_MPI_LIB_NAME}")
    string(REPLACE "\"" "" _MPI_LIB_NAME "${_MPI_LIB_NAME}")
    get_filename_component(_MPI_LIB_PATH "${_MPI_LIB_NAME}" DIRECTORY)
    if(NOT "${_MPI_LIB_PATH}" STREQUAL "")
      list(APPEND MPI_LIB_FULLPATHS_WORK "${_MPI_LIB_NAME}")
    else()
      list(APPEND MPI_LIB_NAMES_WORK "${_MPI_LIB_NAME}")
    endif()
  endforeach()

  if(WIN32)
    # A compiler wrapper on Windows will just have the name of the
    # library to link on its link line, potentially with a full path
    string(REGEX MATCHALL "(^| )([^\" ]+\\.lib|\"[^\"]+\\.lib\")" MPI_LIBNAMES "${MPI_LINK_CMDLINE}")
    foreach(_MPI_LIB_NAME IN LISTS MPI_LIBNAMES)
      string(REGEX REPLACE "^ " "" _MPI_LIB_NAME "${_MPI_LIB_NAME}")
      string(REPLACE "\"" "" _MPI_LIB_NAME "${_MPI_LIB_NAME}")
      get_filename_component(_MPI_LIB_PATH "${_MPI_LIB_NAME}" DIRECTORY)
      if(NOT "${_MPI_LIB_PATH}" STREQUAL "")
        list(APPEND MPI_LIB_FULLPATHS_WORK "${_MPI_LIB_NAME}")
      else()
        list(APPEND MPI_LIB_NAMES_WORK "${_MPI_LIB_NAME}")
      endif()
    endforeach()
  else()
    # On UNIX platforms, archive libraries can be given with full path.
    string(REGEX MATCHALL "(^| )([^\" ]+\\.a|\"[^\"]+\\.a\")" MPI_LIBFULLPATHS "${MPI_LINK_CMDLINE}")
    foreach(_MPI_LIB_NAME IN LISTS MPI_LIBFULLPATHS)
      string(REGEX REPLACE "^ " "" _MPI_LIB_NAME "${_MPI_LIB_NAME}")
      string(REPLACE "\"" "" _MPI_LIB_NAME "${_MPI_LIB_NAME}")
      get_filename_component(_MPI_LIB_PATH "${_MPI_LIB_NAME}" DIRECTORY)
      if(NOT "${_MPI_LIB_PATH}" STREQUAL "")
        list(APPEND MPI_LIB_FULLPATHS_WORK "${_MPI_LIB_NAME}")
      else()
        list(APPEND MPI_LIB_NAMES_WORK "${_MPI_LIB_NAME}")
      endif()
    endforeach()
  endif()

  # An MPI compiler wrapper could have its MPI libraries in the implictly
  # linked directories of the compiler itself.
  if(DEFINED CMAKE_${LANG}_IMPLICIT_LINK_DIRECTORIES)
    list(APPEND MPI_LINK_DIRECTORIES_WORK "${CMAKE_${LANG}_IMPLICIT_LINK_DIRECTORIES}")
  endif()

  # Determine full path names for all of the libraries that one needs
  # to link against in an MPI program
  unset(MPI_PLAIN_LIB_NAMES_WORK)
  foreach(_MPI_LIB_NAME IN LISTS MPI_LIB_NAMES_WORK)
    get_filename_component(_MPI_PLAIN_LIB_NAME "${_MPI_LIB_NAME}" NAME_WE)
    list(APPEND MPI_PLAIN_LIB_NAMES_WORK "${_MPI_PLAIN_LIB_NAME}")
    find_library(MPI_${_MPI_PLAIN_LIB_NAME}_LIBRARY
      NAMES "${_MPI_LIB_NAME}" "lib${_MPI_LIB_NAME}"
      HINTS ${MPI_LINK_DIRECTORIES_WORK}
      DOC "Location of the ${_MPI_PLAIN_LIB_NAME} library for MPI"
    )
    mark_as_advanced(MPI_${_MPI_PLAIN_LIB_NAME}_LIBRARY)
  endforeach()

  # Deal with the libraries given with full path next
  unset(MPI_DIRECT_LIB_NAMES_WORK)
  foreach(_MPI_LIB_FULLPATH IN LISTS MPI_LIB_FULLPATHS_WORK)
    get_filename_component(_MPI_PLAIN_LIB_NAME "${_MPI_LIB_FULLPATH}" NAME_WE)
    get_filename_component(_MPI_LIB_NAME "${_MPI_LIB_FULLPATH}" NAME)
    get_filename_component(_MPI_LIB_PATH "${_MPI_LIB_FULLPATH}" DIRECTORY)
    list(APPEND MPI_DIRECT_LIB_NAMES_WORK "${_MPI_PLAIN_LIB_NAME}")
    find_library(MPI_${_MPI_PLAIN_LIB_NAME}_LIBRARY
      NAMES "${_MPI_LIB_NAME}"
      HINTS ${_MPI_LIB_PATH}
      DOC "Location of the ${_MPI_PLAIN_LIB_NAME} library for MPI"
    )
    mark_as_advanced(MPI_${_MPI_PLAIN_LIB_NAME}_LIBRARY)
  endforeach()
  if(MPI_DIRECT_LIB_NAMES_WORK)
    set(MPI_PLAIN_LIB_NAMES_WORK "${MPI_DIRECT_LIB_NAMES_WORK};${MPI_PLAIN_LIB_NAMES_WORK}")
  endif()

  # MPI might require pthread to work. The above mechanism wouldn't detect it, but we need to
  # link it in that case. -lpthread is covered by the normal library treatment on the other hand.
  if("${MPI_COMPILE_CMDLINE}" MATCHES "-pthread")
    list(APPEND MPI_COMPILE_OPTIONS_WORK "-pthread")
    if(MPI_LINK_FLAGS_WORK)
      string(APPEND MPI_LINK_FLAGS_WORK " -pthread")
    else()
      set(MPI_LINK_FLAGS_WORK "-pthread")
    endif()
  endif()

  # If we found MPI, set up all of the appropriate cache entries
  if(NOT MPI_${LANG}_COMPILE_OPTIONS)
    set(MPI_${LANG}_COMPILE_OPTIONS          ${MPI_COMPILE_OPTIONS_WORK}     CACHE STRING "MPI ${LANG} compilation options"            FORCE)
  endif()
  if(NOT MPI_${LANG}_COMPILE_DEFINITIONS)
    set(MPI_${LANG}_COMPILE_DEFINITIONS      ${MPI_COMPILE_DEFINITIONS_WORK} CACHE STRING "MPI ${LANG} compilation definitions"        FORCE)
  endif()
  if(NOT MPI_${LANG}_ADDITIONAL_INCLUDE_DIRS)
    set(MPI_${LANG}_ADDITIONAL_INCLUDE_DIRS  ${MPI_INCLUDE_DIRS_WORK}        CACHE STRING "MPI ${LANG} additional include directories" FORCE)
  endif()
  if(NOT MPI_${LANG}_LINK_FLAGS)
    set(MPI_${LANG}_LINK_FLAGS               ${MPI_LINK_FLAGS_WORK}          CACHE STRING "MPI ${LANG} linker flags"                   FORCE)
  endif()
  if(NOT MPI_${LANG}_LIB_NAMES)
    set(MPI_${LANG}_LIB_NAMES                ${MPI_PLAIN_LIB_NAMES_WORK}     CACHE STRING "MPI ${LANG} libraries to link against"      FORCE)
  endif()
  set(MPI_${LANG}_WRAPPER_FOUND TRUE PARENT_SCOPE)
endfunction()

function(_MPI_guess_settings LANG)
  set(MPI_GUESS_FOUND FALSE)
  # Currently only MSMPI and MPICH2 on Windows are supported, so we can skip this search if we're not targeting that.
  if(WIN32)
    # MSMPI

    # The environment variables MSMPI_INC and MSMPILIB32/64 are the only ways of locating the MSMPI_SDK,
    # which is installed separately from the runtime. Thus it's possible to have mpiexec but not MPI headers
    # or import libraries and vice versa.
    if(NOT MPI_GUESS_LIBRARY_NAME OR "${MPI_GUESS_LIBRARY_NAME}" STREQUAL "MSMPI")
      # We first attempt to locate the msmpi.lib. Should be find it, we'll assume that the MPI present is indeed
      # Microsoft MPI.
      if("${CMAKE_SIZEOF_VOID_P}" EQUAL 8)
        set(MPI_MSMPI_LIB_PATH "$ENV{MSMPI_LIB64}")
        set(MPI_MSMPI_INC_PATH_EXTRA "$ENV{MSMPI_INC}/x64")
      else()
        set(MPI_MSMPI_LIB_PATH "$ENV{MSMPI_LIB32}")
        set(MPI_MSMPI_INC_PATH_EXTRA "$ENV{MSMPI_INC}/x86")
      endif()

      find_library(MPI_msmpi_LIBRARY
        NAMES msmpi
        HINTS ${MPI_MSMPI_LIB_PATH}
        DOC "Location of the msmpi library for Microsoft MPI")
      mark_as_advanced(MPI_msmpi_LIBRARY)

      if(MPI_msmpi_LIBRARY)
        # Next, we attempt to locate the MPI header. Note that for Fortran we know that mpif.h is a way
        # MSMPI can be used and therefore that header has to be present.
        if(NOT MPI_${LANG}_ADDITIONAL_INCLUDE_DIRS)
          get_filename_component(MPI_MSMPI_INC_DIR "$ENV{MSMPI_INC}" REALPATH)
          set(MPI_${LANG}_ADDITIONAL_INCLUDE_DIRS "${MPI_MSMPI_INC_DIR}" CACHE STRING "MPI ${LANG} additional include directories" FORCE)
          unset(MPI_MSMPI_INC_DIR)
        endif()

        # For MSMPI, one can compile the MPI module by building the mpi.f90 shipped with the MSMPI SDK,
        # thus it might be present or provided by the user. Figuring out which is supported is done later on.
        # The PGI Fortran compiler for instance ships a prebuilt set of modules in its own include folder.
        # Should a user be employing PGI or have built its own set and provided it via cache variables, the
        # splitting routine would have located the module files.

        # For C and C++, we're done here (MSMPI does not ship the MPI-2 C++ bindings) - however, for Fortran
        # we need some extra library to glue Fortran support together:
        # MSMPI ships 2-4 Fortran libraries, each for different Fortran compiler behaviors. The library names
        # ending with a c are using the cdecl calling convention, whereas those ending with an s are for Fortran
        # implementations using stdcall. Therefore, the 64-bit MSMPI only ships those ending in 'c', whereas the 32-bit
        # has both variants available.
        # The second difference is the last but one letter, if it's an e(nd), the length of a string argument is
        # passed by the Fortran compiler after all other arguments on the parameter list, if it's an m(ixed),
        # it's passed immediately after the string address.

        # To summarize:
        #   - msmpifec: CHARACTER length passed after the parameter list and using cdecl calling convention
        #   - msmpifmc: CHARACTER length passed directly after string address and using cdecl calling convention
        #   - msmpifes: CHARACTER length passed after the parameter list and using stdcall calling convention
        #   - msmpifms: CHARACTER length passed directly after string address and using stdcall calling convention
        # 32-bit MSMPI ships all four libraries, 64-bit MSMPI ships only the first two.

        # As is, Intel Fortran and PGI Fortran both use the 'ec' variant of the calling convention, whereas
        # the old Compaq Visual Fortran compiler defaulted to the 'ms' version. It's possible to make Intel Fortran
        # use the CVF calling convention using /iface:cvf, but we assume - and this is also assumed in FortranCInterface -
        # this isn't the case. It's also possible to make CVF use the 'ec' variant, using /iface=(cref,nomixed_str_len_arg).

        # Our strategy is now to locate all libraries, but enter msmpifec into the LIB_NAMES array.
        # Should this not be adequate it's a straightforward way for a user to change the LIB_NAMES array and
        # have his library found. Still, this should not be necessary outside of exceptional cases, as reasoned.
        if ("${LANG}" STREQUAL "Fortran")
          set(MPI_MSMPI_CALLINGCONVS c)
          if("${CMAKE_SIZEOF_VOID_P}" EQUAL 4)
            list(APPEND MPI_MSMPI_CALLINGCONVS s)
          endif()
          foreach(mpistrlenpos IN ITEMS e m)
            foreach(mpicallingconv IN LISTS MPI_MSMPI_CALLINGCONVS)
              find_library(MPI_msmpif${mpistrlenpos}${mpicallingconv}_LIBRARY
                NAMES msmpif${mpistrlenpos}${mpicallingconv}
                HINTS "${MPI_MSMPI_LIB_PATH}"
                DOC "Location of the msmpi${mpistrlenpos}${mpicallingconv} library for Microsoft MPI")
              mark_as_advanced(MPI_msmpif${mpistrlenpos}${mpicallingconv}_LIBRARY)
            endforeach()
          endforeach()
          if(NOT MPI_${LANG}_LIB_NAMES)
            set(MPI_${LANG}_LIB_NAMES "msmpi;msmpifec" CACHE STRING "MPI ${LANG} libraries to link against" FORCE)
          endif()

          # At this point we're *not* done. MSMPI requires an additional include file for Fortran giving the value
          # of MPI_AINT. This file is called mpifptr.h located in the x64 and x86 subfolders, respectively.
          find_path(MPI_mpifptr_INCLUDE_DIR
            NAMES "mpifptr.h"
            HINTS "${MPI_MSMPI_INC_PATH_EXTRA}"
            DOC "Location of the mpifptr.h extra header for Microsoft MPI")
          if(NOT MPI_${LANG}_ADDITIONAL_INCLUDE_VARS)
            set(MPI_${LANG}_ADDITIONAL_INCLUDE_VARS "mpifptr" CACHE STRING "MPI ${LANG} additional include directory variables, given in the form MPI_<name>_INCLUDE_DIR." FORCE)
          endif()
          mark_as_advanced(MPI_${LANG}_ADDITIONAL_INCLUDE_VARS MPI_mpifptr_INCLUDE_DIR)
        else()
          if(NOT MPI_${LANG}_LIB_NAMES)
            set(MPI_${LANG}_LIB_NAMES "msmpi" CACHE STRING "MPI ${LANG} libraries to link against" FORCE)
          endif()
        endif()
        mark_as_advanced(MPI_${LANG}_LIB_NAMES)
        set(MPI_GUESS_FOUND TRUE)
      endif()
    endif()

    # At this point there's not many MPIs that we could still consider.
    # OpenMPI 1.6.x and below supported Windows, but these ship compiler wrappers that still work.
    # The only other relevant MPI implementation without a wrapper is MPICH2, which had Windows support in 1.4.1p1 and older.
    if(NOT MPI_GUESS_LIBRARY_NAME OR "${MPI_GUESS_LIBRARY_NAME}" STREQUAL "MPICH2")
      set(MPI_MPICH_PREFIX_PATHS
        "$ENV{ProgramW6432}/MPICH2/lib"
        "[HKEY_LOCAL_MACHINE\\SOFTWARE\\MPICH\\SMPD;binary]/../lib"
        "[HKEY_LOCAL_MACHINE\\SOFTWARE\\MPICH2;Path]/lib"
      )

      # All of C, C++ and Fortran will need mpi.lib, so we'll look for this first
      find_library(MPI_mpi_LIBRARY
        NAMES mpi
        HINTS ${MPI_MPICH_PREFIX_PATHS})
      mark_as_advanced(MPI_mpi_LIBRARY)
      # If we found mpi.lib, we detect the rest of MPICH2
      if(MPI_mpi_LIBRARY)
        set(MPI_MPICH_LIB_NAMES "mpi")
        # If MPI-2 C++ bindings are requested, we need to locate cxx.lib as well.
        # Otherwise, MPICH_SKIP_MPICXX will be defined and these bindings aren't needed.
        if("${LANG}" STREQUAL "CXX" AND NOT MPI_CXX_SKIP_MPICXX)
          find_library(MPI_cxx_LIBRARY
            NAMES cxx
            HINTS ${MPI_MPICH_PREFIX_PATHS})
          mark_as_advanced(MPI_cxx_LIBRARY)
          list(APPEND MPI_MPICH_LIB_NAMES "cxx")
        # For Fortran, MPICH2 provides three different libraries:
        #   fmpich2.lib which uses uppercase symbols and cdecl,
        #   fmpich2s.lib which uses uppercase symbols and stdcall (32-bit only),
        #   fmpich2g.lib which uses lowercase symbols with double underscores and cdecl.
        # fmpich2s.lib would be useful for Compaq Visual Fortran, fmpich2g.lib has to be used with GNU g77 and is also
        # provided in the form of an .a archive for MinGW and Cygwin. From our perspective, fmpich2.lib is the only one
        # we need to try, and if it doesn't work with the given Fortran compiler we'd find out later on during validation
        elseif("${LANG}" STREQUAL "Fortran")
          find_library(MPI_fmpich2_LIBRARY
            NAMES fmpich2
            HINTS ${MPI_MPICH_PREFIX_PATHS})
          find_library(MPI_fmpich2s_LIBRARY
            NAMES fmpich2s
            HINTS ${MPI_MPICH_PREFIX_PATHS})
          find_library(MPI_fmpich2g_LIBRARY
            NAMES fmpich2g
            HINTS ${MPI_MPICH_PREFIX_PATHS})
          mark_as_advanced(MPI_fmpich2_LIBRARY MPI_fmpich2s_LIBRARY MPI_fmpich2g_LIBRARY)
          list(APPEND MPI_MPICH_LIB_NAMES "fmpich2")
        endif()

        if(NOT MPI_${LANG}_LIB_NAMES)
          set(MPI_${LANG}_LIB_NAMES "${MPI_MPICH_LIB_NAMES}" CACHE STRING "MPI ${LANG} libraries to link against" FORCE)
        endif()
        unset(MPI_MPICH_LIB_NAMES)

        if(NOT MPI_${LANG}_ADDITIONAL_INCLUDE_DIRS)
          # For MPICH2, the include folder would be in ../include relative to the library folder.
          get_filename_component(MPI_MPICH_ROOT_DIR "${MPI_mpi_LIBRARY}" DIRECTORY)
          get_filename_component(MPI_MPICH_ROOT_DIR "${MPI_MPICH_ROOT_DIR}" DIRECTORY)
          if(IS_DIRECTORY "${MPI_MPICH_ROOT_DIR}/include")
            set(MPI_${LANG}_ADDITIONAL_INCLUDE_DIRS "${MPI_MPICH_ROOT_DIR}/include" CACHE STRING "MPI ${LANG} additional include directory variables, given in the form MPI_<name>_INCLUDE_DIR." FORCE)
          endif()
          unset(MPI_MPICH_ROOT_DIR)
        endif()
        set(MPI_GUESS_FOUND TRUE)
      endif()
      unset(MPI_MPICH_PREFIX_PATHS)
    endif()
  endif()
  set(MPI_${LANG}_GUESS_FOUND "${MPI_GUESS_FOUND}" PARENT_SCOPE)
endfunction()

function(_MPI_adjust_compile_definitions LANG)
  if("${LANG}" STREQUAL "CXX")
    # To disable the C++ bindings, we need to pass some definitions since the mpi.h header has to deal with both C and C++
    # bindings in MPI-2.
    if(MPI_CXX_SKIP_MPICXX AND NOT MPI_${LANG}_COMPILE_DEFINITIONS MATCHES "SKIP_MPICXX")
      # MPICH_SKIP_MPICXX is being used in MPICH and derivatives like MVAPICH or Intel MPI
      # OMPI_SKIP_MPICXX is being used in Open MPI
      # _MPICC_H is being used for IBM Platform MPI
      list(APPEND MPI_${LANG}_COMPILE_DEFINITIONS "MPICH_SKIP_MPICXX" "OMPI_SKIP_MPICXX" "_MPICC_H")
      set(MPI_${LANG}_COMPILE_DEFINITIONS "${MPI_${LANG}_COMPILE_DEFINITIONS}" CACHE STRING "MPI ${LANG} compilation definitions" FORCE)
    endif()
  endif()
endfunction()

macro(_MPI_assemble_libraries LANG)
  set(MPI_${LANG}_LIBRARIES "")
  foreach(mpilib IN LISTS MPI_${LANG}_LIB_NAMES)
    list(APPEND MPI_${LANG}_LIBRARIES ${MPI_${mpilib}_LIBRARY})
  endforeach()
endmacro()

macro(_MPI_assemble_include_dirs LANG)
  set(MPI_${LANG}_INCLUDE_DIRS "${MPI_${LANG}_ADDITIONAL_INCLUDE_DIRS}")
  if("${LANG}" MATCHES "(C|CXX)")
    if(MPI_${LANG}_HEADER_DIR)
      list(APPEND MPI_${LANG}_INCLUDE_DIRS "${MPI_${LANG}_HEADER_DIR}")
    endif()
  else() # Fortran
    if(MPI_${LANG}_F77_HEADER_DIR)
      list(APPEND MPI_${LANG}_INCLUDE_DIRS "${MPI_${LANG}_F77_HEADER_DIR}")
    endif()
    if(MPI_${LANG}_MODULE_DIR AND NOT "${MPI_${LANG}_MODULE_DIR}" IN_LIST MPI_${LANG}_INCLUDE_DIRS)
      list(APPEND MPI_${LANG}_INCLUDE_DIRS "${MPI_${LANG}_MODULE_DIR}")
    endif()
  endif()
  if(MPI_${LANG}_ADDITIONAL_INCLUDE_VARS)
    foreach(mpiadditionalinclude IN LISTS MPI_${LANG}_ADDITIONAL_INCLUDE_VARS)
      list(APPEND MPI_${LANG}_INCLUDE_DIRS "${MPI_${mpiadditionalinclude}_INCLUDE_DIR}")
    endforeach()
  endif()
endmacro()

function(_MPI_split_include_dirs LANG)
  # Backwards compatibility: Search INCLUDE_PATH if given.
  if(MPI_${LANG}_INCLUDE_PATH)
    list(APPEND MPI_${LANG}_ADDITIONAL_INCLUDE_DIRS "${MPI_${LANG}_INCLUDE_PATH}")
  endif()

  # We try to find the headers/modules among those paths (and system paths)
  # For C/C++, we just need to have a look for mpi.h.
  if("${LANG}" MATCHES "(C|CXX)")
    find_path(MPI_${LANG}_HEADER_DIR "mpi.h"
      HINTS ${MPI_${LANG}_ADDITIONAL_INCLUDE_DIRS}
    )
    mark_as_advanced(MPI_${LANG}_HEADER_DIR)
    if(MPI_${LANG}_ADDITIONAL_INCLUDE_DIRS)
      list(REMOVE_ITEM MPI_${LANG}_ADDITIONAL_INCLUDE_DIRS "${MPI_${LANG}_HEADER_DIR}")
    endif()
  # Fortran is more complicated here: An implementation could provide
  # any of the Fortran 77/90/2008 APIs for MPI. For example, MSMPI
  # only provides Fortran 77 and - if mpi.f90 is built - potentially
  # a Fortran 90 module.
  elseif("${LANG}" STREQUAL "Fortran")
    find_path(MPI_${LANG}_F77_HEADER_DIR "mpif.h"
      HINTS ${MPI_${LANG}_ADDITIONAL_INCLUDE_DIRS}
    )
    find_path(MPI_${LANG}_MODULE_DIR
      NAMES "mpi.mod" "mpi_f08.mod"
      HINTS ${MPI_${LANG}_ADDITIONAL_INCLUDE_DIRS}
    )
    if(MPI_${LANG}_ADDITIONAL_INCLUDE_DIRS)
      list(REMOVE_ITEM MPI_${LANG}_ADDITIONAL_INCLUDE_DIRS
        "${MPI_${LANG}_F77_HEADER_DIR}"
        "${MPI_${LANG}_MODULE_DIR}"
      )
    endif()
    mark_as_advanced(MPI_${LANG}_F77_HEADER_DIR MPI_${LANG}_MODULE_DIR)
  endif()
  set(MPI_${LANG}_ADDITIONAL_INCLUDE_DIRS ${MPI_${LANG}_ADDITIONAL_INCLUDE_DIRS} CACHE STRING "MPI ${LANG} additional include directories" FORCE)
endfunction()

macro(_MPI_create_imported_target LANG)
  if(NOT TARGET MPI::MPI_${LANG})
    add_library(MPI::MPI_${LANG} INTERFACE IMPORTED)
  endif()

  set_property(TARGET MPI::MPI_${LANG} PROPERTY INTERFACE_COMPILE_OPTIONS "${MPI_${LANG}_COMPILE_OPTIONS}")
  set_property(TARGET MPI::MPI_${LANG} PROPERTY INTERFACE_COMPILE_DEFINITIONS "${MPI_${LANG}_COMPILE_DEFINITIONS}")

  set_property(TARGET MPI::MPI_${LANG} PROPERTY INTERFACE_LINK_LIBRARIES "")
  if(MPI_${LANG}_LINK_FLAGS)
    set_property(TARGET MPI::MPI_${LANG} APPEND PROPERTY INTERFACE_LINK_LIBRARIES "${MPI_${LANG}_LINK_FLAGS}")
  endif()
  # If the compiler links MPI implicitly, no libraries will be found as they're contained within
  # CMAKE_<LANG>_IMPLICIT_LINK_LIBRARIES already.
  if(MPI_${LANG}_LIBRARIES)
    set_property(TARGET MPI::MPI_${LANG} APPEND PROPERTY INTERFACE_LINK_LIBRARIES "${MPI_${LANG}_LIBRARIES}")
  endif()
  # Given the new design of FindMPI, INCLUDE_DIRS will always be located, even under implicit linking.
  set_property(TARGET MPI::MPI_${LANG} PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${MPI_${LANG}_INCLUDE_DIRS}")
endmacro()

function(_MPI_try_staged_settings LANG MPI_TEST_FILE_NAME MODE RUN_BINARY)
  set(WORK_DIR "${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/FindMPI")
  set(SRC_DIR "${CMAKE_CURRENT_LIST_DIR}/FindMPI")
  set(BIN_FILE "${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/FindMPI/${MPI_TEST_FILE_NAME}_${LANG}.bin")
  unset(MPI_TEST_COMPILE_DEFINITIONS)
  if("${LANG}" STREQUAL "Fortran")
    if("${MODE}" STREQUAL "F90_MODULE")
      set(MPI_Fortran_INCLUDE_LINE "use mpi\n      implicit none")
    elseif("${MODE}" STREQUAL "F08_MODULE")
      set(MPI_Fortran_INCLUDE_LINE "use mpi_f08\n      implicit none")
    else() # F77 header
      set(MPI_Fortran_INCLUDE_LINE "implicit none\n      include 'mpif.h'")
    endif()
    configure_file("${SRC_DIR}/${MPI_TEST_FILE_NAME}.f90.in" "${WORK_DIR}/${MPI_TEST_FILE_NAME}.f90" @ONLY)
    set(MPI_TEST_SOURCE_FILE "${WORK_DIR}/${MPI_TEST_FILE_NAME}.f90")
  elseif("${LANG}" STREQUAL "CXX")
    configure_file("${SRC_DIR}/${MPI_TEST_FILE_NAME}.c" "${WORK_DIR}/${MPI_TEST_FILE_NAME}.cpp" COPYONLY)
    set(MPI_TEST_SOURCE_FILE "${WORK_DIR}/${MPI_TEST_FILE_NAME}.cpp")
    if("${MODE}" STREQUAL "TEST_MPICXX")
      set(MPI_TEST_COMPILE_DEFINITIONS TEST_MPI_MPICXX)
    endif()
  else() # C
    set(MPI_TEST_SOURCE_FILE "${SRC_DIR}/${MPI_TEST_FILE_NAME}.c")
  endif()
  if(RUN_BINARY)
    try_run(MPI_RUN_RESULT_${LANG}_${MPI_TEST_FILE_NAME}_${MODE} MPI_RESULT_${LANG}_${MPI_TEST_FILE_NAME}_${MODE}
     "${CMAKE_BINARY_DIR}" SOURCES "${MPI_TEST_SOURCE_FILE}"
      COMPILE_DEFINITIONS ${MPI_TEST_COMPILE_DEFINITIONS}
      LINK_LIBRARIES MPI::MPI_${LANG}
      RUN_OUTPUT_VARIABLE MPI_RUN_OUTPUT_${LANG}_${MPI_TEST_FILE_NAME}_${MODE})
    set(MPI_RUN_OUTPUT_${LANG}_${MPI_TEST_FILE_NAME}_${MODE} "${MPI_RUN_OUTPUT_${LANG}_${MPI_TEST_FILE_NAME}_${MODE}}" PARENT_SCOPE)
  else()
    try_compile(MPI_RESULT_${LANG}_${MPI_TEST_FILE_NAME}_${MODE}
      "${CMAKE_BINARY_DIR}" SOURCES "${MPI_TEST_SOURCE_FILE}"
      COMPILE_DEFINITIONS ${MPI_TEST_COMPILE_DEFINITIONS}
      LINK_LIBRARIES MPI::MPI_${LANG}
      COPY_FILE "${BIN_FILE}")
  endif()
endfunction()

macro(_MPI_check_lang_works LANG)
  # For Fortran we may have by the MPI-3 standard an implementation that provides:
  #   - the mpi_f08 module
  #   - *both*, the mpi module and 'mpif.h'
  # Since older MPI standards (MPI-1) did not define anything but 'mpif.h', we need to check all three individually.
  if( NOT MPI_${LANG}_WORKS )
    if("${LANG}" STREQUAL "Fortran")
      set(MPI_Fortran_INTEGER_LINE "(kind=MPI_INTEGER_KIND)")
      _MPI_try_staged_settings(${LANG} test_mpi F77_HEADER FALSE)
      _MPI_try_staged_settings(${LANG} test_mpi F90_MODULE FALSE)
      _MPI_try_staged_settings(${LANG} test_mpi F08_MODULE FALSE)

      set(MPI_${LANG}_WORKS FALSE)

      foreach(mpimethod IN ITEMS F77_HEADER F08_MODULE F90_MODULE)
        if(MPI_RESULT_${LANG}_test_mpi_${mpimethod})
          set(MPI_${LANG}_WORKS TRUE)
          set(MPI_${LANG}_HAVE_${mpimethod} TRUE)
        else()
          set(MPI_${LANG}_HAVE_${mpimethod} FALSE)
        endif()
      endforeach()
      # MPI-1 versions had no MPI_INTGER_KIND defined, so we need to try without it.
      # However, MPI-1 also did not define the Fortran 90 and 08 modules, so we only try the F77 header.
      unset(MPI_Fortran_INTEGER_LINE)
      if(NOT MPI_${LANG}_WORKS)
        _MPI_try_staged_settings(${LANG} test_mpi F77_HEADER_NOKIND FALSE)
        if(MPI_RESULT_${LANG}_test_mpi_F77_HEADER_NOKIND)
          set(MPI_${LANG}_WORKS TRUE)
          set(MPI_${LANG}_HAVE_F77_HEADER TRUE)
        endif()
      endif()
    else()
      _MPI_try_staged_settings(${LANG} test_mpi normal FALSE)
      # If 'test_mpi' built correctly, we've found valid MPI settings. There might not be MPI-2 C++ support, but there can't
      # be MPI-2 C++ support without the C bindings being present, so checking for them is sufficient.
      set(MPI_${LANG}_WORKS "${MPI_RESULT_${LANG}_test_mpi_normal}")
    endif()
  endif()
endmacro()

# Some systems install various MPI implementations in separate folders in some MPI prefix
# This macro enumerates all such subfolders and adds them to the list of hints that will be searched.
macro(MPI_search_mpi_prefix_folder PREFIX_FOLDER)
  if(EXISTS "${PREFIX_FOLDER}")
    file(GLOB _MPI_folder_children RELATIVE "${PREFIX_FOLDER}" "${PREFIX_FOLDER}/*")
    foreach(_MPI_folder_child IN LISTS _MPI_folder_children)
      if(IS_DIRECTORY "${PREFIX_FOLDER}/${_MPI_folder_child}")
        list(APPEND MPI_HINT_DIRS "${PREFIX_FOLDER}/${_MPI_folder_child}")
      endif()
    endforeach()
  endif()
endmacro()

set(MPI_HINT_DIRS ${MPI_HOME} $ENV{MPI_HOME} $ENV{I_MPI_ROOT})
if("${CMAKE_HOST_SYSTEM_NAME}" STREQUAL "Linux")
  # SUSE Linux Enterprise Server stores its MPI implementations under /usr/lib64/mpi/gcc/<name>
  # We enumerate the subfolders and append each as a prefix
  MPI_search_mpi_prefix_folder("/usr/lib64/mpi/gcc")
elseif("${CMAKE_HOST_SYSTEM_NAME}" STREQUAL "Windows")
  # MSMPI stores its runtime in a special folder, this adds the possible locations to the hints.
  list(APPEND MPI_HINT_DIRS $ENV{MSMPI_BIN} "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\MPI;InstallRoot]")
elseif("${CMAKE_HOST_SYSTEM_NAME}" STREQUAL "FreeBSD")
  # FreeBSD ships mpich under the normal system paths - but available openmpi implementations
  # will be found in /usr/local/mpi/<name>
  MPI_search_mpi_prefix_folder("/usr/local/mpi/")
endif()

# Most MPI distributions have some form of mpiexec or mpirun which gives us something we can look for.
# The MPI standard does not mandate the existence of either, but instead only makes requirements if a distribution
# ships an mpiexec program (mpirun executables are not regulated by the standard).
find_program(MPIEXEC_EXECUTABLE
  NAMES ${_MPIEXEC_NAMES}
  PATH_SUFFIXES bin sbin
  HINTS ${MPI_HINT_DIRS}
  DOC "Executable for running MPI programs.")

# call get_filename_component twice to remove mpiexec and the directory it exists in (typically bin).
# This gives us a fairly reliable base directory to search for /bin /lib and /include from.
get_filename_component(_MPI_BASE_DIR "${MPIEXEC_EXECUTABLE}" PATH)
get_filename_component(_MPI_BASE_DIR "${_MPI_BASE_DIR}" PATH)

# According to the MPI standard, section 8.8 -n is a guaranteed, and the only guaranteed way to
# launch an MPI process using mpiexec if such a program exists.
set(MPIEXEC_NUMPROC_FLAG "-n"  CACHE STRING "Flag used by MPI to specify the number of processes for mpiexec; the next option will be the number of processes.")
set(MPIEXEC_PREFLAGS     ""    CACHE STRING "These flags will be directly before the executable that is being run by mpiexec.")
set(MPIEXEC_POSTFLAGS    ""    CACHE STRING "These flags will be placed after all flags passed to mpiexec.")

# Set the number of processes to the physical processor count
cmake_host_system_information(RESULT _MPIEXEC_NUMPROCS QUERY NUMBER_OF_PHYSICAL_CORES)
set(MPIEXEC_MAX_NUMPROCS "${_MPIEXEC_NUMPROCS}" CACHE STRING "Maximum number of processors available to run MPI applications.")
unset(_MPIEXEC_NUMPROCS)
mark_as_advanced(MPIEXEC_EXECUTABLE MPIEXEC_NUMPROC_FLAG MPIEXEC_PREFLAGS MPIEXEC_POSTFLAGS MPIEXEC_MAX_NUMPROCS)

#=============================================================================
# Backward compatibility input hacks.  Propagate the FindMPI hints to C and
# CXX if the respective new versions are not defined.  Translate the old
# MPI_LIBRARY and MPI_EXTRA_LIBRARY to respective MPI_${LANG}_LIBRARIES.
#
# Once we find the new variables, we translate them back into their old
# equivalents below.
foreach (LANG IN ITEMS C CXX)
  # Old input variables.
  set(_MPI_OLD_INPUT_VARS COMPILER COMPILE_FLAGS INCLUDE_PATH LINK_FLAGS)

  # Set new vars based on their old equivalents, if the new versions are not already set.
  foreach (var ${_MPI_OLD_INPUT_VARS})
    if (NOT MPI_${LANG}_${var} AND MPI_${var})
      set(MPI_${LANG}_${var} "${MPI_${var}}")
    endif()
  endforeach()

  # Chop the old compile flags into options and definitions
  if(MPI_${LANG}_COMPILE_FLAGS)
    unset(MPI_${LANG}_COMPILE_OPTIONS)
    unset(MPI_${LANG}_COMPILE_DEFINITIONS)
    separate_arguments(MPI_SEPARATE_FLAGS NATIVE_COMMAND "${MPI_${LANG}_COMPILE_FLAGS}")
    foreach(_MPI_FLAG IN LISTS MPI_SEPARATE_FLAGS)
      if("${_MPI_FLAG}" MATCHES "^ *[-/D]([^ ]+)")
        list(APPEND MPI_${LANG}_COMPILE_DEFINITIONS "${CMAKE_MATCH_1}")
      else()
        list(APPEND MPI_${LANG}_COMPILE_FLAGS "${_MPI_FLAG}")
      endif()
    endforeach()
    unset(MPI_SEPARATE_FLAGS)
  endif()

  # If a list of libraries was given, we'll split it into new-style cache variables
  if(NOT MPI_${LANG}_LIB_NAMES)
    foreach(_MPI_LIB IN LISTS MPI_${LANG}_LIBRARIES MPI_LIBRARY MPI_EXTRA_LIBRARY)
      get_filename_component(_MPI_PLAIN_LIB_NAME "${_MPI_LIB}" NAME_WE)
      get_filename_component(_MPI_LIB_NAME "${_MPI_LIB}" NAME)
      get_filename_component(_MPI_LIB_DIR "${_MPI_LIB}" DIRECTORY)
      list(APPEND MPI_PLAIN_LIB_NAMES_WORK "${_MPI_PLAIN_LIB_NAME}")
      find_library(MPI_${_MPI_PLAIN_LIB_NAME}_LIBRARY
        NAMES "${_MPI_LIB_NAME}" "lib${_MPI_LIB_NAME}"
        HINTS ${_MPI_LIB_DIR} $ENV{MPI_LIB}
        DOC "Location of the ${_MPI_PLAIN_LIB_NAME} library for MPI"
      )
      mark_as_advanced(MPI_${_MPI_PLAIN_LIB_NAME}_LIBRARY)
    endforeach()
  endif()
endforeach()
#=============================================================================

unset(MPI_VERSION)
unset(MPI_VERSION_MAJOR)
unset(MPI_VERSION_MINOR)

unset(_MPI_MIN_VERSION)

# This loop finds the compilers and sends them off for interrogation.
foreach(LANG IN ITEMS C CXX Fortran)
  if(CMAKE_${LANG}_COMPILER_LOADED)
    if(NOT MPI_FIND_COMPONENTS)
      set(_MPI_FIND_${LANG} TRUE)
    elseif( ${LANG} IN_LIST MPI_FIND_COMPONENTS)
      set(_MPI_FIND_${LANG} TRUE)
    elseif( ${LANG} STREQUAL CXX AND NOT MPI_CXX_SKIP_MPICXX AND MPICXX IN_LIST MPI_FIND_COMPONENTS )
      set(_MPI_FIND_${LANG} TRUE)
    else()
      set(_MPI_FIND_${LANG} FALSE)
    endif()
  else()
    set(_MPI_FIND_${LANG} FALSE)
  endif()
  if(_MPI_FIND_${LANG})
    if( ${LANG} STREQUAL CXX AND NOT MPICXX IN_LIST MPI_FIND_COMPONENTS )
      set(MPI_CXX_SKIP_MPICXX FALSE CACHE BOOL "If true, the MPI-2 C++ bindings are disabled using definitions.")
      mark_as_advanced(MPI_CXX_SKIP_MPICXX)
    endif()
    if(NOT (MPI_${LANG}_LIB_NAMES AND (MPI_${LANG}_INCLUDE_PATH OR MPI_${LANG}_INCLUDE_DIRS OR MPI_${LANG}_ADDITIONAL_INCLUDE_DIRS)))
      if(NOT MPI_${LANG}_COMPILER AND NOT MPI_ASSUME_NO_BUILTIN_MPI)
        # Should the imported targets be empty, we effectively try whether the compiler supports MPI on its own, which is the case on e.g.
        # Cray PrgEnv.
        _MPI_create_imported_target(${LANG})
        _MPI_check_lang_works(${LANG})

        # If the compiler can build MPI code on its own, it functions as an MPI compiler and we'll set the variable to point to it.
        if(MPI_${LANG}_WORKS)
          set(MPI_${LANG}_COMPILER "${CMAKE_${LANG}_COMPILER}" CACHE FILEPATH "MPI compiler for ${LANG}" FORCE)
        endif()
      endif()

      # If the user specified a library name we assume they prefer that library over a wrapper. If not, they can disable skipping manually.
      if(NOT DEFINED MPI_SKIP_COMPILER_WRAPPER AND MPI_GUESS_LIBRARY_NAME)
        set(MPI_SKIP_COMPILER_WRAPPER TRUE)
      endif()
      if(NOT MPI_SKIP_COMPILER_WRAPPER)
        if(MPI_${LANG}_COMPILER)
          # If the user supplies a compiler *name* instead of an absolute path, assume that we need to find THAT compiler.
          if (NOT IS_ABSOLUTE "${MPI_${LANG}_COMPILER}")
            # Get rid of our default list of names and just search for the name the user wants.
            set(_MPI_${LANG}_COMPILER_NAMES "${MPI_${LANG}_COMPILER}")
            unset(MPI_${LANG}_COMPILER CACHE)
          endif()
          # If the user specifies a compiler, we don't want to try to search libraries either.
          set(MPI_PINNED_COMPILER TRUE)
        else()
          set(MPI_PINNED_COMPILER FALSE)
        endif()

        # If we have an MPI base directory, we'll try all compiler names in that one first.
        # This should prevent mixing different MPI environments
        if(_MPI_BASE_DIR)
          find_program(MPI_${LANG}_COMPILER
            NAMES  ${_MPI_${LANG}_COMPILER_NAMES}
            PATH_SUFFIXES bin sbin
            HINTS  ${_MPI_BASE_DIR}
            NO_DEFAULT_PATH
            DOC    "MPI compiler for ${LANG}"
          )
        endif()

        # If the base directory did not help (for example because the mpiexec isn't in the same directory as the compilers),
        # we shall try searching in the default paths.
        find_program(MPI_${LANG}_COMPILER
          NAMES  ${_MPI_${LANG}_COMPILER_NAMES}
          PATH_SUFFIXES bin sbin
          DOC    "MPI compiler for ${LANG}"
        )

        if(MPI_${LANG}_COMPILER STREQUAL CMAKE_${LANG}_COMPILER)
          set(MPI_SKIP_GUESSING TRUE)
        elseif(MPI_${LANG}_COMPILER)
          _MPI_interrogate_compiler(${LANG})
        else()
          set(MPI_${LANG}_WRAPPER_FOUND FALSE)
        endif()
      else()
        set(MPI_${LANG}_WRAPPER_FOUND FALSE)
        set(MPI_PINNED_COMPILER FALSE)
      endif()

      if(NOT MPI_${LANG}_WRAPPER_FOUND AND NOT MPI_PINNED_COMPILER)
        # For C++, we may use the settings for C. Should a given compiler wrapper for C++ not exist, but one for C does, we copy over the
        # settings for C. An MPI distribution that is in this situation would be IBM Platform MPI.
        if("${LANG}" STREQUAL "CXX" AND MPI_C_WRAPPER_FOUND)
          set(MPI_${LANG}_COMPILE_OPTIONS          ${MPI_C_COMPILE_OPTIONS}     CACHE STRING "MPI ${LANG} compilation options"           )
          set(MPI_${LANG}_COMPILE_DEFINITIONS      ${MPI_C_COMPILE_DEFINITIONS} CACHE STRING "MPI ${LANG} compilation definitions"       )
          set(MPI_${LANG}_ADDITIONAL_INCLUDE_DIRS  ${MPI_C_INCLUDE_DIRS}        CACHE STRING "MPI ${LANG} additional include directories")
          set(MPI_${LANG}_LINK_FLAGS               ${MPI_C_LINK_FLAGS}          CACHE STRING "MPI ${LANG} linker flags"                  )
          set(MPI_${LANG}_LIB_NAMES                ${MPI_C_LIB_NAMES}           CACHE STRING "MPI ${LANG} libraries to link against"     )
          set(MPI_${LANG}_WRAPPER_FOUND TRUE)
        elseif(NOT MPI_SKIP_GUESSING)
          _MPI_guess_settings(${LANG})
        endif()
      endif()
    endif()

    _MPI_split_include_dirs(${LANG})
    if(NOT MPI_${LANG}_COMPILER STREQUAL CMAKE_${LANG}_COMPILER)
      _MPI_assemble_include_dirs(${LANG})
      _MPI_assemble_libraries(${LANG})
    endif()
    _MPI_adjust_compile_definitions(${LANG})
    # We always create imported targets even if they're empty
    _MPI_create_imported_target(${LANG})

    if(NOT MPI_${LANG}_WORKS)
      _MPI_check_lang_works(${LANG})
    endif()

    # Next, we'll initialize the MPI variables that have not been previously set.
    set(MPI_${LANG}_COMPILE_OPTIONS          "" CACHE STRING "MPI ${LANG} compilation flags"             )
    set(MPI_${LANG}_COMPILE_DEFINITIONS      "" CACHE STRING "MPI ${LANG} compilation definitions"       )
    set(MPI_${LANG}_ADDITIONAL_INCLUDE_DIRS  "" CACHE STRING "MPI ${LANG} additional include directories")
    set(MPI_${LANG}_LINK_FLAGS               "" CACHE STRING "MPI ${LANG} linker flags"                  )
    set(MPI_${LANG}_LIB_NAMES                "" CACHE STRING "MPI ${LANG} libraries to link against"     )
    mark_as_advanced(MPI_${LANG}_COMPILE_OPTIONS MPI_${LANG}_COMPILE_DEFINITIONS MPI_${LANG}_LINK_FLAGS
      MPI_${LANG}_LIB_NAMES MPI_${LANG}_ADDITIONAL_INCLUDE_DIRS MPI_${LANG}_COMPILER)

    # If we've found MPI, then we'll perform additional analysis: Determine the MPI version, MPI library version, supported
    # MPI APIs (i.e. MPI-2 C++ bindings). For Fortran we also need to find specific parameters if we're under MPI-3.
    if(MPI_${LANG}_WORKS)
      if("${LANG}" STREQUAL "CXX" AND NOT DEFINED MPI_MPICXX_FOUND)
        if(NOT MPI_CXX_SKIP_MPICXX AND NOT MPI_CXX_VALIDATE_SKIP_MPICXX)
          _MPI_try_staged_settings(${LANG} test_mpi MPICXX FALSE)
          if(MPI_RESULT_${LANG}_test_mpi_MPICXX)
            set(MPI_MPICXX_FOUND TRUE)
          else()
            set(MPI_MPICXX_FOUND FALSE)
          endif()
        else()
          set(MPI_MPICXX_FOUND FALSE)
        endif()
      endif()

      # At this point, we know the bindings present but not the MPI version or anything else.
      if(NOT DEFINED MPI_${LANG}_VERSION)
        unset(MPI_${LANG}_VERSION_MAJOR)
        unset(MPI_${LANG}_VERSION_MINOR)
      endif()
      set(MPI_BIN_FOLDER ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/FindMPI)

      # For Fortran, we'll want to use the most modern MPI binding to test capabilities other than the
      # Fortran parameters, since those depend on the method of consumption.
      # For C++, we can always use the C bindings, and should do so, since the C++ bindings do not exist in MPI-3
      # whereas the C bindings do, and the C++ bindings never offered any feature advantage over their C counterparts.
      if("${LANG}" STREQUAL "Fortran")
        if(MPI_${LANG}_HAVE_F08_MODULE)
          set(MPI_${LANG}_HIGHEST_METHOD F08_MODULE)
        elseif(MPI_${LANG}_HAVE_F90_MODULE)
          set(MPI_${LANG}_HIGHEST_METHOD F90_MODULE)
        else()
          set(MPI_${LANG}_HIGHEST_METHOD F77_HEADER)
        endif()

        # Another difference between C and Fortran is that we can't use the preprocessor to determine whether MPI_VERSION
        # and MPI_SUBVERSION are provided. These defines did not exist in MPI 1.0 and 1.1 and therefore might not
        # exist. For C/C++, test_mpi.c will handle the MPI_VERSION extraction, but for Fortran, we need mpiver.f90.
        if(NOT DEFINED MPI_${LANG}_VERSION)
          _MPI_try_staged_settings(${LANG} mpiver ${MPI_${LANG}_HIGHEST_METHOD} FALSE)
          if(MPI_RESULT_${LANG}_mpiver_${MPI_${LANG}_HIGHEST_METHOD})
            file(STRINGS ${MPI_BIN_FOLDER}/mpiver_${LANG}.bin _MPI_VERSION_STRING LIMIT_COUNT 1 REGEX "INFO:MPI-VER")
            if("${_MPI_VERSION_STRING}" MATCHES ".*INFO:MPI-VER\\[([0-9]+)\\.([0-9]+)\\].*")
              set(MPI_${LANG}_VERSION_MAJOR "${CMAKE_MATCH_1}")
              set(MPI_${LANG}_VERSION_MINOR "${CMAKE_MATCH_2}")
              set(MPI_${LANG}_VERSION "${MPI_${LANG}_VERSION_MAJOR}.${MPI_${LANG}_VERSION_MINOR}")
            endif()
          endif()
        endif()

        # Finally, we want to find out which capabilities a given interface supports, compare the MPI-3 standard.
        # This is determined by interface specific parameters MPI_SUBARRAYS_SUPPORTED and MPI_ASYNC_PROTECTS_NONBLOCKING
        # and might vary between the different methods of consumption.
        if(MPI_DETERMINE_Fortran_CAPABILITIES AND NOT MPI_Fortran_CAPABILITIES_DETERMINED)
          foreach(mpimethod IN ITEMS F08_MODULE F90_MODULE F77_HEADER)
            if(MPI_${LANG}_HAVE_${mpimethod})
              set(MPI_${LANG}_${mpimethod}_SUBARRAYS FALSE)
              set(MPI_${LANG}_${mpimethod}_ASYNCPROT FALSE)
              _MPI_try_staged_settings(${LANG} fortranparam_mpi ${mpimethod} TRUE)
              if(MPI_RESULT_${LANG}_fortranparam_mpi_${mpimethod} AND
                NOT "${MPI_RUN_RESULT_${LANG}_fortranparam_mpi_${mpimethod}}" STREQUAL "FAILED_TO_RUN")
                if("${MPI_RUN_OUTPUT_${LANG}_fortranparam_mpi_${mpimethod}}" MATCHES
                  ".*INFO:SUBARRAYS\\[ *([TF]) *\\]-ASYNCPROT\\[ *([TF]) *\\].*")
                  if("${CMAKE_MATCH_1}" STREQUAL "T")
                    set(MPI_${LANG}_${mpimethod}_SUBARRAYS TRUE)
                  endif()
                  if("${CMAKE_MATCH_2}" STREQUAL "T")
                    set(MPI_${LANG}_${mpimethod}_ASYNCPROT TRUE)
                  endif()
                endif()
              endif()
            endif()
          endforeach()
          set(MPI_Fortran_CAPABILITIES_DETERMINED TRUE)
        endif()
      else()
        set(MPI_${LANG}_HIGHEST_METHOD normal)

        # By the MPI-2 standard, MPI_VERSION and MPI_SUBVERSION are valid for both C and C++ bindings.
        if(NOT DEFINED MPI_${LANG}_VERSION)
          file(STRINGS ${MPI_BIN_FOLDER}/test_mpi_${LANG}.bin _MPI_VERSION_STRING LIMIT_COUNT 1 REGEX "INFO:MPI-VER")
          if("${_MPI_VERSION_STRING}" MATCHES ".*INFO:MPI-VER\\[([0-9]+)\\.([0-9]+)\\].*")
            set(MPI_${LANG}_VERSION_MAJOR "${CMAKE_MATCH_1}")
            set(MPI_${LANG}_VERSION_MINOR "${CMAKE_MATCH_2}")
            set(MPI_${LANG}_VERSION "${MPI_${LANG}_VERSION_MAJOR}.${MPI_${LANG}_VERSION_MINOR}")
          endif()
        endif()
      endif()

      unset(MPI_BIN_FOLDER)

      # At this point, we have dealt with determining the MPI version and parameters for each Fortran method available.
      # The one remaining issue is to determine which MPI library is installed.
      # Determining the version and vendor of the MPI library is only possible via MPI_Get_library_version() at runtime,
      # and therefore we cannot do this while cross-compiling (a user may still define MPI_<lang>_LIBRARY_VERSION_STRING
      # themselves and we'll attempt splitting it, which is equivalent to provide the try_run output).
      # It's also worth noting that the installed version string can depend on the language, or on the system the binary
      # runs on if MPI is not statically linked.
      if(MPI_DETERMINE_LIBRARY_VERSION AND NOT MPI_${LANG}_LIBRARY_VERSION_STRING)
        _MPI_try_staged_settings(${LANG} libver_mpi ${MPI_${LANG}_HIGHEST_METHOD} TRUE)
        if(MPI_RESULT_${LANG}_libver_mpi_${MPI_${LANG}_HIGHEST_METHOD} AND
          "${MPI_RUN_RESULT_${LANG}_libver_mpi_${MPI_${LANG}_HIGHEST_METHOD}}" EQUAL "0")
          string(STRIP "${MPI_RUN_OUTPUT_${LANG}_libver_mpi_${MPI_${LANG}_HIGHEST_METHOD}}"
            MPI_${LANG}_LIBRARY_VERSION_STRING)
        else()
          set(MPI_${LANG}_LIBRARY_VERSION_STRING "NOTFOUND")
        endif()
      endif()
    endif()

    set(MPI_${LANG}_FIND_QUIETLY ${MPI_FIND_QUIETLY})
    set(MPI_${LANG}_FIND_VERSION ${MPI_FIND_VERSION})
    set(MPI_${LANG}_FIND_VERSION_EXACT ${MPI_FIND_VERSION_EXACT})

    unset(MPI_${LANG}_REQUIRED_VARS)
    if (MPI_${LANG}_WRAPPER_FOUND OR MPI_${LANG}_GUESS_FOUND)
      foreach(mpilibname IN LISTS MPI_${LANG}_LIB_NAMES)
        list(APPEND MPI_${LANG}_REQUIRED_VARS "MPI_${mpilibname}_LIBRARY")
      endforeach()
      list(APPEND MPI_${LANG}_REQUIRED_VARS "MPI_${LANG}_LIB_NAMES")
      if("${LANG}" STREQUAL "Fortran")
        # For Fortran we only need one of the module or header directories to have *some* support for MPI.
        if(NOT MPI_${LANG}_MODULE_DIR)
          list(APPEND MPI_${LANG}_REQUIRED_VARS "MPI_${LANG}_F77_HEADER_DIR")
        endif()
        if(NOT MPI_${LANG}_F77_HEADER_DIR)
          list(APPEND MPI_${LANG}_REQUIRED_VARS "MPI_${LANG}_MODULE_DIR")
        endif()
      else()
        list(APPEND MPI_${LANG}_REQUIRED_VARS "MPI_${LANG}_HEADER_DIR")
      endif()
      if(MPI_${LANG}_ADDITIONAL_INCLUDE_VARS)
        foreach(mpiincvar IN LISTS MPI_${LANG}_ADDITIONAL_INCLUDE_VARS)
          list(APPEND MPI_${LANG}_REQUIRED_VARS "MPI_${mpiincvar}_INCLUDE_DIR")
        endforeach()
      endif()
      # Append the works variable now. If the settings did not work, this will show up properly.
      list(APPEND MPI_${LANG}_REQUIRED_VARS "MPI_${LANG}_WORKS")
    else()
      # If the compiler worked implicitly, use its path as output.
      # Should the compiler variable be set, we also require it to work.
      list(APPEND MPI_${LANG}_REQUIRED_VARS "MPI_${LANG}_COMPILER")
      if(MPI_${LANG}_COMPILER)
        list(APPEND MPI_${LANG}_REQUIRED_VARS "MPI_${LANG}_WORKS")
      endif()
    endif()
    find_package_handle_standard_args(MPI_${LANG} REQUIRED_VARS ${MPI_${LANG}_REQUIRED_VARS}
      VERSION_VAR MPI_${LANG}_VERSION)

    if(DEFINED MPI_${LANG}_VERSION)
      if(NOT _MPI_MIN_VERSION OR _MPI_MIN_VERSION VERSION_GREATER MPI_${LANG}_VERSION)
        set(_MPI_MIN_VERSION MPI_${LANG}_VERSION)
      endif()
    endif()
  endif()
endforeach()

unset(_MPI_REQ_VARS)
foreach(LANG IN ITEMS C CXX Fortran)
  if((NOT MPI_FIND_COMPONENTS AND CMAKE_${LANG}_COMPILER_LOADED) OR LANG IN_LIST MPI_FIND_COMPONENTS)
    list(APPEND _MPI_REQ_VARS "MPI_${LANG}_FOUND")
  endif()
endforeach()

if(MPICXX IN_LIST MPI_FIND_COMPONENTS)
  list(APPEND _MPI_REQ_VARS "MPI_MPICXX_FOUND")
endif()

find_package_handle_standard_args(MPI
    REQUIRED_VARS ${_MPI_REQ_VARS}
    VERSION_VAR ${_MPI_MIN_VERSION}
    HANDLE_COMPONENTS)

#=============================================================================
# More backward compatibility stuff

# For compatibility reasons, we also define MPIEXEC
set(MPIEXEC "${MPIEXEC_EXECUTABLE}")

# Copy over MPI_<LANG>_INCLUDE_PATH from the assembled INCLUDE_DIRS.
foreach(LANG IN ITEMS C CXX Fortran)
  if(MPI_${LANG}_FOUND)
    set(MPI_${LANG}_INCLUDE_PATH "${MPI_${LANG}_INCLUDE_DIRS}")
    unset(MPI_${LANG}_COMPILE_FLAGS)
    if(MPI_${LANG}_COMPILE_OPTIONS)
      set(MPI_${LANG}_COMPILE_FLAGS "${MPI_${LANG}_COMPILE_OPTIONS}")
    endif()
    if(MPI_${LANG}_COMPILE_DEFINITIONS)
      foreach(_MPI_DEF IN LISTS MPI_${LANG}_COMPILE_DEFINITIONS)
        string(APPEND MPI_${LANG}_COMPILE_FLAGS " -D${_MPI_DEF}")
      endforeach()
    endif()
  endif()
endforeach()

# Bare MPI sans ${LANG} vars are set to CXX then C, depending on what was found.
# This mimics the behavior of the old language-oblivious FindMPI.
set(_MPI_OLD_VARS COMPILER INCLUDE_PATH COMPILE_FLAGS LINK_FLAGS LIBRARIES)
if (MPI_CXX_FOUND)
  foreach (var ${_MPI_OLD_VARS})
    set(MPI_${var} ${MPI_CXX_${var}})
  endforeach()
elseif (MPI_C_FOUND)
  foreach (var ${_MPI_OLD_VARS})
    set(MPI_${var} ${MPI_C_${var}})
  endforeach()
endif()

# Chop MPI_LIBRARIES into the old-style MPI_LIBRARY and MPI_EXTRA_LIBRARY, and set them in cache.
if (MPI_LIBRARIES)
  list(GET MPI_LIBRARIES 0 MPI_LIBRARY_WORK)
  set(MPI_LIBRARY "${MPI_LIBRARY_WORK}")
  unset(MPI_LIBRARY_WORK)
else()
  set(MPI_LIBRARY "MPI_LIBRARY-NOTFOUND")
endif()

list(LENGTH MPI_LIBRARIES MPI_NUMLIBS)
if (MPI_NUMLIBS GREATER 1)
  set(MPI_EXTRA_LIBRARY_WORK "${MPI_LIBRARIES}")
  list(REMOVE_AT MPI_EXTRA_LIBRARY_WORK 0)
  set(MPI_EXTRA_LIBRARY "${MPI_EXTRA_LIBRARY_WORK}")
  unset(MPI_EXTRA_LIBRARY_WORK)
else()
  set(MPI_EXTRA_LIBRARY "MPI_EXTRA_LIBRARY-NOTFOUND")
endif()
#=============================================================================

# unset these vars to cleanup namespace
unset(_MPI_OLD_VARS)
unset(_MPI_PREFIX_PATH)
unset(_MPI_BASE_DIR)
foreach (lang C CXX Fortran)
  unset(_MPI_${LANG}_COMPILER_NAMES)
endforeach()

cmake_policy(POP)
