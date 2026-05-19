# Standalone install + find_package smoke (invoked by CTest when FREEUSD_TEST_INSTALL_INTEGRATION=ON).
# Paths: set FREEUSD_SOURCE_DIR and FREEUSD_SCRATCH (CTest passes them via cmake -E env).
if(NOT DEFINED FREEUSD_SOURCE_DIR OR FREEUSD_SOURCE_DIR STREQUAL "")
  if(DEFINED ENV{FREEUSD_SOURCE_DIR} AND NOT "$ENV{FREEUSD_SOURCE_DIR}" STREQUAL "")
    set(FREEUSD_SOURCE_DIR "$ENV{FREEUSD_SOURCE_DIR}")
  endif()
endif()
if(NOT DEFINED FREEUSD_SCRATCH OR FREEUSD_SCRATCH STREQUAL "")
  if(DEFINED ENV{FREEUSD_SCRATCH} AND NOT "$ENV{FREEUSD_SCRATCH}" STREQUAL "")
    set(FREEUSD_SCRATCH "$ENV{FREEUSD_SCRATCH}")
  endif()
endif()
if(NOT FREEUSD_SOURCE_DIR)
  message(FATAL_ERROR "install_integration.cmake: set FREEUSD_SOURCE_DIR (env or -D)")
endif()
if(NOT FREEUSD_SCRATCH)
  message(FATAL_ERROR "install_integration.cmake: set FREEUSD_SCRATCH (env or -D)")
endif()
if(NOT CMAKE_COMMAND)
  find_program(CMAKE_COMMAND NAMES cmake cmake3 REQUIRED)
endif()

set(_build "${FREEUSD_SCRATCH}/_build")
set(_prefix "${FREEUSD_SCRATCH}/_prefix")
# Multi-config generators (Visual Studio) need an explicit configuration; single-config ignores it.
set(_buildcfg Release)

file(REMOVE_RECURSE "${FREEUSD_SCRATCH}")
file(MAKE_DIRECTORY "${FREEUSD_SCRATCH}")
execute_process(COMMAND "${CMAKE_COMMAND}" -E sleep 0.2)

function(_freeusd_run)
  execute_process(COMMAND ${ARGV} RESULT_VARIABLE _rv OUTPUT_VARIABLE _out ERROR_VARIABLE _err)
  if(NOT _rv EQUAL 0)
    message(FATAL_ERROR "Command failed (${_rv}): ${ARGV}\n${_out}\n${_err}")
  endif()
endfunction()

message(STATUS "FreeUSD install integration: configure (prefix=${_prefix})")
_freeusd_run("${CMAKE_COMMAND}" -S "${FREEUSD_SOURCE_DIR}" -B "${_build}" -DCMAKE_BUILD_TYPE=Release
             "-DCMAKE_INSTALL_PREFIX=${_prefix}" -DFREEUSD_BUILD_PYTHON=OFF -DFREEUSD_BUILD_TESTS=OFF)

set(_cache_ready FALSE)
foreach(_wait RANGE 30)
  if(EXISTS "${_build}/CMakeCache.txt")
    set(_cache_ready TRUE)
    break()
  endif()
  execute_process(COMMAND "${CMAKE_COMMAND}" -E sleep 0.2)
endforeach()
if(NOT _cache_ready)
  message(FATAL_ERROR "Configure did not produce ${_build}/CMakeCache.txt")
endif()

message(STATUS "FreeUSD install integration: build")
set(_build_rv 1)
set(_build_out "")
set(_build_err "")
foreach(_attempt RANGE 4)
  if(NOT EXISTS "${_build}/CMakeCache.txt")
    execute_process(COMMAND "${CMAKE_COMMAND}" -E sleep 0.5)
    continue()
  endif()
  execute_process(COMMAND "${CMAKE_COMMAND}" --build "${_build}" --config "${_buildcfg}" --parallel 1
                  RESULT_VARIABLE _build_rv OUTPUT_VARIABLE _build_out ERROR_VARIABLE _build_err)
  if(_build_rv EQUAL 0)
    break()
  endif()
  execute_process(COMMAND "${CMAKE_COMMAND}" -E sleep 0.5)
endforeach()
if(NOT _build_rv EQUAL 0)
  message(FATAL_ERROR "Command failed (${_build_rv}): ${CMAKE_COMMAND} --build ${_build}\n${_build_out}\n${_build_err}")
endif()

message(STATUS "FreeUSD install integration: install")
_freeusd_run("${CMAKE_COMMAND}" --install "${_build}" --config "${_buildcfg}")

set(_consumer "${FREEUSD_SCRATCH}/_consumer")
message(STATUS "FreeUSD install integration: consumer find_package")
_freeusd_run("${CMAKE_COMMAND}" -S "${FREEUSD_SOURCE_DIR}/tests/cmake_find_consumer" -B "${_consumer}"
             "-DCMAKE_PREFIX_PATH=${_prefix}")

set(_consumer_ready FALSE)
foreach(_wait RANGE 30)
  if(EXISTS "${_consumer}/CMakeCache.txt")
    set(_consumer_ready TRUE)
    break()
  endif()
  execute_process(COMMAND "${CMAKE_COMMAND}" -E sleep 0.2)
endforeach()
if(NOT _consumer_ready)
  message(FATAL_ERROR "Consumer configure did not produce ${_consumer}/CMakeCache.txt")
endif()

set(_consumer_rv 1)
set(_consumer_out "")
set(_consumer_err "")
foreach(_attempt RANGE 4)
  if(NOT EXISTS "${_consumer}/CMakeCache.txt")
    execute_process(COMMAND "${CMAKE_COMMAND}" -E sleep 0.5)
    continue()
  endif()
  execute_process(COMMAND "${CMAKE_COMMAND}" --build "${_consumer}" --config "${_buildcfg}" --parallel 1
                  RESULT_VARIABLE _consumer_rv OUTPUT_VARIABLE _consumer_out ERROR_VARIABLE _consumer_err)
  if(_consumer_rv EQUAL 0)
    break()
  endif()
  execute_process(COMMAND "${CMAKE_COMMAND}" -E sleep 0.5)
endforeach()
if(NOT _consumer_rv EQUAL 0)
  message(FATAL_ERROR "Command failed (${_consumer_rv}): ${CMAKE_COMMAND} --build ${_consumer}\n${_consumer_out}\n${_consumer_err}")
endif()

file(
  GLOB _candidates
  "${_consumer}/freeusd_find_consumer"
  "${_consumer}/freeusd_find_consumer.exe"
  "${_consumer}/Release/freeusd_find_consumer.exe"
  "${_consumer}/Debug/freeusd_find_consumer.exe")
if(NOT _candidates)
  message(FATAL_ERROR "Could not find freeusd_find_consumer under ${_consumer}")
endif()
list(GET _candidates 0 _exe)

message(STATUS "FreeUSD install integration: run ${_exe}")
_freeusd_run("${_exe}")

message(STATUS "FreeUSD install integration: OK")
