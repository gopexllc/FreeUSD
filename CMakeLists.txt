cmake_minimum_required(VERSION 3.20)
project(freeusd VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

option(FREEUSD_BUILD_PYTHON "Build pybind11 Python extension" ON)
option(FREEUSD_BUILD_TESTS "Build and register C++ self-tests" ON)

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
  target_link_libraries(freeusd_usda_roundtrip_test PRIVATE freeusd::io)

  add_test(NAME freeusd_selftest COMMAND freeusd_selftest)
  add_test(NAME freeusd_sdf_path_test COMMAND freeusd_sdf_path_test)
  add_test(NAME freeusd_usd_smoke_test COMMAND freeusd_usd_smoke_test)
  add_test(NAME freeusd_usda_roundtrip_test COMMAND freeusd_usda_roundtrip_test)
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

  install(TARGETS _native LIBRARY DESTINATION freeusd)
endif()

install(DIRECTORY include/freeusd TYPE INCLUDE)

install(
  TARGETS freeusd_base freeusd_tf freeusd_vt freeusd_ar freeusd_sdf freeusd_io freeusd_pcp freeusd_usd freeusd_plug
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
