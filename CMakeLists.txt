cmake_minimum_required(VERSION 3.20)
project(freeusd VERSION 0.1.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

option(FREEUSD_BUILD_PYTHON "Build pybind11 Python extension" ON)
option(FREEUSD_BUILD_TESTS "Build and register C++ self-tests" ON)
option(FREEUSD_BUILD_C_ABI "Build libfreeusd_c C API wrapper" ON)

include(GNUInstallDirs)

add_subdirectory(src)

if(FREEUSD_BUILD_TESTS)
  enable_testing()

  add_executable(freeusd_selftest tests/selftest.cpp)
  target_link_libraries(freeusd_selftest PRIVATE freeusd::base)

  add_executable(freeusd_sdf_path_test tests/sdf_path_test.cpp)
  target_link_libraries(freeusd_sdf_path_test PRIVATE freeusd::sdf)

  add_executable(freeusd_usd_smoke_test tests/usd_smoke_test.cpp)
  target_link_libraries(freeusd_usd_smoke_test PRIVATE freeusd::usd)

  add_executable(freeusd_usda_roundtrip_test tests/usda_roundtrip_test.cpp)
  target_link_libraries(freeusd_usda_roundtrip_test PRIVATE freeusd::io freeusd::usd)

  add_executable(freeusd_pcp_compose_test tests/pcp_compose_test.cpp)
  target_link_libraries(freeusd_pcp_compose_test PRIVATE freeusd::pcp)

  add_executable(freeusd_stage_compose_test tests/stage_compose_test.cpp)
  target_link_libraries(freeusd_stage_compose_test PRIVATE freeusd::usd)

  add_executable(freeusd_schema_tokens_bundle_test tests/schema_tokens_bundle_test.cpp)
  target_link_libraries(freeusd_schema_tokens_bundle_test PRIVATE freeusd::usd_schemas)

  add_executable(freeusd_time_code_test tests/time_code_test.cpp)
  target_link_libraries(freeusd_time_code_test PRIVATE freeusd::usd)

  add_executable(freeusd_edit_target_test tests/edit_target_test.cpp)
  target_link_libraries(freeusd_edit_target_test PRIVATE freeusd::usd)

  add_executable(freeusd_sdf_layer_offset_test tests/sdf_layer_offset_test.cpp)
  target_include_directories(freeusd_sdf_layer_offset_test PRIVATE "${PROJECT_SOURCE_DIR}/include")

  add_executable(freeusd_sdf_asset_path_test tests/sdf_asset_path_test.cpp)
  target_include_directories(freeusd_sdf_asset_path_test PRIVATE "${PROJECT_SOURCE_DIR}/include")

  add_executable(freeusd_gf_range1d_test tests/gf_range1d_test.cpp)
  target_include_directories(freeusd_gf_range1d_test PRIVATE "${PROJECT_SOURCE_DIR}/include")

  add_executable(freeusd_gf_quatd_test tests/gf_quatd_test.cpp)
  target_include_directories(freeusd_gf_quatd_test PRIVATE "${PROJECT_SOURCE_DIR}/include")

  add_executable(freeusd_gf_bbox3d_test tests/gf_bbox3d_test.cpp)
  target_link_libraries(freeusd_gf_bbox3d_test PRIVATE freeusd::gf)

  add_executable(freeusd_ar_resolved_path_test tests/ar_resolved_path_test.cpp)
  target_include_directories(freeusd_ar_resolved_path_test PRIVATE "${PROJECT_SOURCE_DIR}/include")

  add_executable(freeusd_sdf_tokens_test tests/sdf_tokens_test.cpp)
  target_link_libraries(freeusd_sdf_tokens_test PRIVATE freeusd::tf)

  add_executable(freeusd_usd_tokens_test tests/usd_tokens_test.cpp)
  target_link_libraries(freeusd_usd_tokens_test PRIVATE freeusd::tf)

  add_test(NAME freeusd_selftest COMMAND freeusd_selftest)
  add_test(NAME freeusd_sdf_path_test COMMAND freeusd_sdf_path_test)
  add_test(NAME freeusd_usd_smoke_test COMMAND freeusd_usd_smoke_test)
  add_test(NAME freeusd_usda_roundtrip_test COMMAND freeusd_usda_roundtrip_test)
  add_test(NAME freeusd_pcp_compose_test COMMAND freeusd_pcp_compose_test)
  add_test(NAME freeusd_stage_compose_test COMMAND freeusd_stage_compose_test)
  add_test(NAME freeusd_schema_tokens_bundle_test COMMAND freeusd_schema_tokens_bundle_test)
  add_test(NAME freeusd_time_code_test COMMAND freeusd_time_code_test)
  add_test(NAME freeusd_edit_target_test COMMAND freeusd_edit_target_test)
  add_test(NAME freeusd_sdf_layer_offset_test COMMAND freeusd_sdf_layer_offset_test)
  add_test(NAME freeusd_sdf_asset_path_test COMMAND freeusd_sdf_asset_path_test)
  add_test(NAME freeusd_gf_range1d_test COMMAND freeusd_gf_range1d_test)
  add_test(NAME freeusd_gf_quatd_test COMMAND freeusd_gf_quatd_test)
  add_test(NAME freeusd_gf_bbox3d_test COMMAND freeusd_gf_bbox3d_test)
  add_test(NAME freeusd_ar_resolved_path_test COMMAND freeusd_ar_resolved_path_test)
  add_test(NAME freeusd_sdf_tokens_test COMMAND freeusd_sdf_tokens_test)
  add_test(NAME freeusd_usd_tokens_test COMMAND freeusd_usd_tokens_test)

  if(FREEUSD_BUILD_C_ABI)
    add_executable(freeusd_c_smoke_test tests/c_abi_smoke.c)
    target_include_directories(freeusd_c_smoke_test PRIVATE "${PROJECT_SOURCE_DIR}/include")
    target_link_libraries(freeusd_c_smoke_test PRIVATE freeusd::c)
    set_target_properties(freeusd_c_smoke_test PROPERTIES C_STANDARD 11 C_STANDARD_REQUIRED ON LINKER_LANGUAGE CXX)
    add_test(NAME freeusd_c_smoke_test COMMAND freeusd_c_smoke_test)
  endif()
endif()

if(FREEUSD_BUILD_PYTHON)
  include(FetchContent)
  FetchContent_Declare(
    pybind11
    GIT_REPOSITORY https://github.com/pybind/pybind11.git
    GIT_TAG v2.13.6
    EXCLUDE_FROM_ALL
  )
  FetchContent_MakeAvailable(pybind11)

  find_package(Python REQUIRED COMPONENTS Interpreter Development.Module)

  pybind11_add_module(_native MODULE python/bindings.cpp)
  target_link_libraries(_native PRIVATE freeusd::runtime)
  set_target_properties(_native PROPERTIES OUTPUT_NAME _native)
  # Dev/test: Python resolves `freeusd._native` from the `freeusd/` package directory.
  add_custom_command(
    TARGET _native
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:_native>
            ${CMAKE_SOURCE_DIR}/freeusd/$<TARGET_FILE_NAME:_native>
    COMMENT "Sync _native extension beside Python package (gitignored *.so)")

  install(TARGETS _native LIBRARY DESTINATION freeusd)
  install(DIRECTORY "${CMAKE_SOURCE_DIR}/freeusd/" DESTINATION freeusd FILES_MATCHING PATTERN "*.py")
endif()

if(FREEUSD_BUILD_PYTHON AND FREEUSD_BUILD_TESTS)
  execute_process(
    COMMAND "${Python_EXECUTABLE}" -c "import pytest"
    RESULT_VARIABLE _freeusd_have_pytest
    OUTPUT_QUIET
    ERROR_QUIET)
  if(_freeusd_have_pytest EQUAL 0)
    add_test(
      NAME freeusd_pytest
      COMMAND "${Python_EXECUTABLE}" -m pytest "${CMAKE_SOURCE_DIR}/tests/python" -q
      WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
    set_tests_properties(freeusd_pytest PROPERTIES ENVIRONMENT "PYTHONPATH=${CMAKE_SOURCE_DIR}" LABELS "python")
  endif()
endif()

install(DIRECTORY include/freeusd TYPE INCLUDE)

set(_freeusd_install_targets freeusd_base freeusd_tf freeusd_vt freeusd_ar freeusd_sdf freeusd_io freeusd_pcp
                             freeusd_usd freeusd_plug)
if(FREEUSD_BUILD_C_ABI)
  list(APPEND _freeusd_install_targets freeusd_c)
endif()
install(
  TARGETS ${_freeusd_install_targets}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
