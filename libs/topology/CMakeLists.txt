# Copyright (c) 2019-2020 The STE||AR-Group
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

cmake_minimum_required(VERSION 3.13 FATAL_ERROR)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Default location is $HPX_ROOT/libs/topology/include
set(topology_headers
  hpx/topology/cpu_mask.hpp
  hpx/topology/topology.hpp
)

# Default location is $HPX_ROOT/libs/topology/include_compatibility
set(topology_compat_headers
  hpx/runtime/threads/cpu_mask.hpp
  hpx/runtime/threads/topology.hpp
)

# Default location is $HPX_ROOT/libs/topology/src
set(topology_sources
  cpu_mask.cpp
  topology.cpp
)

include(HPX_AddModule)
add_hpx_module(topology
  COMPATIBILITY_HEADERS ON    # Added in 1.4.0
  DEPRECATION_WARNINGS
  FORCE_LINKING_GEN
  GLOBAL_HEADER_GEN ON
  SOURCES ${topology_sources}
  HEADERS ${topology_headers}
  COMPAT_HEADERS ${topology_compat_headers}
  DEPENDENCIES
    hpx_assertion
    hpx_config
    hpx_concurrency
    hpx_errors
    hpx_format
    hpx_logging
    hpx_type_support
    hpx_util
    Hwloc::hwloc
  CMAKE_SUBDIRS examples tests
)

