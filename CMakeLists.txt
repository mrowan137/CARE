######################################################################################
# Copyright 2020 Lawrence Livermore National Security, LLC and other CARE developers.
# See the top-level LICENSE file for details.
#
# SPDX-License-Identifier: BSD-3-Clause
######################################################################################

cmake_minimum_required(VERSION 3.9)

project(care LANGUAGES CXX)

################################
# Build options
################################

# Configuration options
option(ENABLE_MPI "Build MPI support" OFF)
option(ENABLE_CUDA "Build CUDA support" OFF)
option(ENABLE_OPENMP "Build OpenMP support" OFF)
option(ENABLE_HIP "Build HIP support" OFF)

# Advanced configuration options
# TODO: If these are disabled, the code will not compile or run correctly.
#       Fix those issues so that these options are actually configurable.
option(ENABLE_PICK "Enable pick and set methods on ManagedArrays" ON)
option(ENABLE_PINNED "Enable pinned memory space" ON)
option(CARE_ENABLE_PINNED_MEMORY_FOR_SCANS "Use pinned memory for scan lengths" ON)
# Option to disable implicit conversion between host_device_ptr and raw arrays in CARE.
option(CARE_ENABLE_IMPLICIT_CONVERSIONS "Enable implicit conversions to-from raw pointers" ON)
# CHAI must also be configured with the same settings for implicit conversions.
set(ENABLE_IMPLICIT_CONVERSIONS ${CARE_ENABLE_IMPLICIT_CONVERSIONS} CACHE BOOL "Enable implicit conversions to-from raw pointers")
option(CARE_LEGACY_COMPATIBILITY_MODE "Enable legacy compatibility mode" OFF)
option(CARE_ENABLE_MANAGED_PTR "Enable managed_ptr aliases, tests, and reproducer" ON)
option(CARE_DISABLE_RAJAPLUGIN "Disable use of the RAJA plugin. WILL ALSO DISABLE MEMORY MOTION." OFF)
option(CARE_ENABLE_EXTERN_INSTANTIATE "Enable extern instantiation of template functions" OFF)
# Bounds checking only checks for negative indices
# if the resource manager is disabled (CHAI_DISABLE_RM defined).
option(CARE_ENABLE_BOUNDS_CHECKING "Enable bounds checking for host_device_ptr on the host" OFF)
option(CARE_ENABLE_GPU_SIMULATION_MODE "Enable GPU simulation mode." OFF )
option(CARE_NEVER_USE_RAJA_PARALLEL_SCAN "Disable RAJA parallel scans in SCAN loops." OFF )
#  Known to not compile for Debug configurations, have low probability of working in 
#  practice for Release / RelWithDebInfo configurations.
option(CARE_ENABLE_FUSER_BIN_32 "Enable the 32 register fusible loop bin." OFF)
option(CARE_ENABLE_PARALLEL_LOOP_BACKWARDS "Reverse the start and end for parallel loops." OFF)
option(CARE_ENABLE_STALE_DATA_CHECK "Enable checking for stale host data. Only applicable for GPU (or GPU simulation) builds." OFF)

# Extra components
option(CARE_ENABLE_TESTS "Build CARE tests" ON)
option(CARE_ENABLE_BENCHMARKS "Build CARE benchmarks" ON)
option(CARE_ENABLE_DOCS "Build CARE documentation" ON)
option(CARE_ENABLE_EXAMPLES "Build CARE examples" ON)
option(CARE_ENABLE_REPRODUCERS "Build CARE reproducers" OFF)

# Extra submodule components
option(CARE_ENABLE_SUBMODULE_TESTS "Build submodule tests" OFF)
option(CARE_ENABLE_SUBMODULE_BENCHMARKS "Build submodule benchmarks" OFF)
option(CARE_ENABLE_SUBMODULE_DOCS "Build submodule documentation" OFF)
option(CARE_ENABLE_SUBMODULE_EXAMPLES "Build submodule examples" OFF)
option(CARE_ENABLE_SUBMODULE_TOOLS "Build submodule tools" OFF)
option(CARE_ENABLE_SUBMODULE_EXERCISES "Build submodule exercises" OFF)
option(CARE_ENABLE_SUBMODULE_REPRODUCERS "Build submodule reproducers" OFF)

# Enable tests
if (CARE_ENABLE_TESTS OR CARE_ENABLE_SUBMODULE_TESTS)
   option(ENABLE_TESTS "Build tests" ON)
endif ()

# Enable benchmarks
if (CARE_ENABLE_BENCHMARKS OR CARE_ENABLE_SUBMODULE_BENCHMARKS)
   option(ENABLE_BENCHMARKS "Build benchmarks" ON)
endif ()

# Enable documentation
if (CARE_ENABLE_DOCS OR CARE_ENABLE_SUBMODULE_DOCS)
   option(ENABLE_DOCS    "Build documentation" ON)
   option(ENABLE_DOXYGEN "Enables Doxygen support" ON)
   option(ENABLE_SPHINX  "Enables Sphinx support" ON)
else ()
   option(ENABLE_DOCS    "Build documentation" OFF)
   option(ENABLE_DOXYGEN "Enables Doxygen support" OFF)
   option(ENABLE_SPHINX  "Enables Sphinx support" OFF)
endif ()

# Enable examples
if (CARE_ENABLE_EXAMPLES OR CARE_ENABLE_SUBMODULE_EXAMPLES)
   option(ENABLE_EXAMPLES "Build examples" ON)
endif ()

# Enable reproducers
if (CARE_ENABLE_REPRODUCERS OR CARE_ENABLE_SUBMODULE_REPRODUCERS)
   option(ENABLE_REPRODUCERS "Build reproducers" ON)
endif ()

# BLT is required to build the code
set(BLT_SOURCE_DIR "" CACHE PATH "Path to external BLT")

# Required dependencies (if the following variables are not set, uses submodules)
# The lower case variables point to the cmake config file (if applicable).
# The upper case variables simply point to the root install location.
# Whichever form is most convenient can be used to specify the dependencies.

# These variables point to the installed config files
set(camp_DIR "" CACHE PATH "Path to campConfig.cmake")
set(umpire_DIR "" CACHE PATH "Path to umpire-config.cmake")
set(raja_DIR "" CACHE PATH "Path to raja-config.cmake")
set(chai_DIR "" CACHE PATH "Path to chai-config.cmake")

# These variables point to the root install location
set(CAMP_DIR "" CACHE PATH "Path to installed CAMP")
set(UMPIRE_DIR "" CACHE PATH "Path to installed Umpire")
set(RAJA_DIR "" CACHE PATH "Path to installed RAJA")
set(CHAI_DIR "" CACHE PATH "Path to installed CHAI")

# Optional dependencies without submodules (not open source)
set(LLNL_GLOBALID_DIR "" CACHE PATH "Path to external LLNL_GlobalID")

# CUDA specific options
if (ENABLE_CUDA)
   # CUDA configuration
   set(CUDA_TOOLKIT_ROOT_DIR "" CACHE PATH "Set the CUDA toolkit path")
   set(CUDA_ARCH "sm_35" CACHE STRING "Set the CUDA virtual architecture")
   set(CUDA_CODE "compute_35" CACHE STRING "Set the CUDA actual architecture")
   option(CUDA_SEPARABLE_COMPILATION "Enable CUDA separable compilation" ON)
   option(CUDA_LINK_WITH_NVCC "Enable linking with NVCC" OFF)

   # CUDA required dependencies
   set(CUB_DIR "" CACHE PATH "Path to CUB")

   # CUDA optional dependencies without submodules (not open source)
   set(NVTOOLSEXT_DIR "" CACHE PATH "Path to external NVTOOLSEXT")

   # Optional features
   set(CARE_ENABLE_LOOP_FUSER ON CACHE STRING "Enable the loop fuser")

   # This is needed for the loop fuser to work
   option(ENABLE_RAJA_PLUGIN "Build plugin to set RAJA execution spaces" ON)
else()
   set(CARE_ENABLE_LOOP_FUSER OFF CACHE STRING "Enable the loop fuser")
endif()

# HIP specific options
if (ENABLE_HIP)
   option(RAJA_ENABLE_HIP_INDIRECT_FUNCTION_CALL "Enable use of device function pointers in hip backend" OFF)
endif()

################################
# BLT
################################

# Prevent BLT from looking for the following unused executables
option(ENABLE_CLANGQUERY   "Enables clang-query support" OFF)
option(ENABLE_CLANGTIDY    "Enables clang-tidy support" OFF)
option(ENABLE_CPPCHECK     "Enables Cppcheck support" OFF)
option(ENABLE_VALGRIND     "Enables Valgrind support" OFF)

option(ENABLE_ASTYLE       "Enables AStyle support" OFF)
option(ENABLE_CLANGFORMAT  "Enables ClangFormat support" OFF)
option(ENABLE_CMAKEFORMAT  "Enables CMakeFormat support" OFF)
option(ENABLE_UNCRUSTIFY   "Enables Uncrustify support" OFF)
option(ENABLE_YAPF         "Enables Yapf support" OFF)

set(BLT_CXX_STD "c++11" CACHE STRING "Set the c++ standard to use")

if (BLT_SOURCE_DIR)
    message(STATUS "CARE: Using external BLT")

    if (NOT EXISTS ${BLT_SOURCE_DIR}/SetupBLT.cmake)
        message(FATAL_ERROR "CARE: Given BLT_SOURCE_DIR does not contain SetupBLT.cmake")
    endif()
else()
    message(STATUS "CARE: Using BLT submodule")

    set(BLT_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/blt")

    if (NOT EXISTS ${BLT_SOURCE_DIR}/SetupBLT.cmake)
       message(FATAL_ERROR "CARE: BLT submodule is not initialized. Run `git submodule update --init` in git repository or set BLT_SOURCE_DIR to external BLT.")
    endif()
endif()

include(${BLT_SOURCE_DIR}/SetupBLT.cmake)

################################
# Set up compiler and flags
################################
if (ENABLE_CUDA)
   set(CMAKE_CUDA_COMPILER "${CUDA_TOOLKIT_ROOT_DIR}/bin/nvcc" CACHE PATH "")
   set(CMAKE_CUDA_HOST_COMPILER ${CMAKE_CXX_COMPILER} CACHE PATH "")

   # nvcc dies if compiler flags are duplicated, and RAJA adds all these compiler flags
   set(RAJA_CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS}")

   # Only add these flags if they are not already present or nvcc will die
   if (NOT CMAKE_CUDA_FLAGS MATCHES "--expt-extended-lambda")
      set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} --expt-extended-lambda")
   endif ()

   if (NOT CMAKE_CUDA_FLAGS MATCHES "-arch")
      set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} -arch ${CUDA_CODE}")
   endif ()

   if (NOT CMAKE_CUDA_FLAGS MATCHES "-code")
      set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} -code ${CUDA_ARCH}")
   endif ()

   set(CARE_CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS}")
endif()

### needed if we use hip_runtime as a dependency instead of just hip
### this is kind of a hack
#if (ENABLE_HIP)
#   set(HIP_ROOT_DIR "/opt/rocm-3.5.0/hip" CACHE PATH "")
#   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --hip-device-lib-path=/opt/rocm-3.5.0/lib  -I/opt/rocm-3.5.0/llvm/lib/clang/11.0.0 -x hip")
#   set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} --hip-device-lib-path=/opt/rocm-3.5.0/lib -I/opt/rocm-3.5.0/llvm/lib/clang/11.0.0 -x hip")
#endif()

################################
# Set up dependencies
################################
include(cmake/SetupLibraries.cmake)

################################
# Build CARE library
################################
add_subdirectory(src)

################################
# Build CARE extras
################################
if (CARE_ENABLE_TESTS)
   add_subdirectory(test)
endif()

if (CARE_ENABLE_BENCHMARKS)
   add_subdirectory(benchmarks)
endif()

if (CARE_ENABLE_DOCS)
   add_subdirectory(docs)
endif ()

if (CARE_ENABLE_EXAMPLES)
   add_subdirectory(examples)
endif ()

if (CARE_ENABLE_REPRODUCERS)
   add_subdirectory(reproducers)
endif ()
