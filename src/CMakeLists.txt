# Modular layout inspired by OpenUSD (pxr): tf, gf, vt, ar, sdf, pcp, usd, usdGeom, usdUtils, plus plug/trace/work stubs.
# All code here is clean-room; it does not incorporate Pixar or AOUSD source.

set(FREEUSD_INC "${PROJECT_SOURCE_DIR}/include")

add_library(freeusd_build_settings INTERFACE)
target_compile_definitions(
  freeusd_build_settings
  INTERFACE
    FREEUSD_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
    FREEUSD_VERSION_MINOR=${PROJECT_VERSION_MINOR}
    FREEUSD_VERSION_PATCH=${PROJECT_VERSION_PATCH}
    $<$<BOOL:${BUILD_SHARED_LIBS}>:FREEUSD_SHARED>
)

add_library(freeusd_base STATIC "${PROJECT_SOURCE_DIR}/src/version.cpp")
add_library(freeusd::base ALIAS freeusd_base)
target_include_directories(freeusd_base PUBLIC "$<BUILD_INTERFACE:${FREEUSD_INC}>" "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>")
target_link_libraries(freeusd_base PUBLIC freeusd_build_settings)

add_library(freeusd_tf STATIC tf/token.cpp tf/hash.cpp)
add_library(freeusd::tf ALIAS freeusd_tf)
target_include_directories(freeusd_tf PUBLIC "$<BUILD_INTERFACE:${FREEUSD_INC}>" "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>")
target_link_libraries(freeusd_tf PUBLIC freeusd_build_settings)

add_library(freeusd_gf INTERFACE)
add_library(freeusd::gf ALIAS freeusd_gf)
target_include_directories(freeusd_gf INTERFACE "$<BUILD_INTERFACE:${FREEUSD_INC}>" "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>")

add_library(freeusd_vt STATIC vt/value.cpp)
add_library(freeusd::vt ALIAS freeusd_vt)
target_include_directories(freeusd_vt PUBLIC "$<BUILD_INTERFACE:${FREEUSD_INC}>" "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>")
target_link_libraries(freeusd_vt PUBLIC freeusd_tf freeusd_gf freeusd_build_settings)

add_library(freeusd_ar STATIC ar/defaultResolver.cpp)
add_library(freeusd::ar ALIAS freeusd_ar)
target_include_directories(freeusd_ar PUBLIC "$<BUILD_INTERFACE:${FREEUSD_INC}>" "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>")
target_link_libraries(freeusd_ar PUBLIC freeusd_build_settings)

add_library(freeusd_sdf STATIC sdf/path.cpp sdf/layer.cpp sdf/fieldOpinion.cpp sdf/primReference.cpp)
add_library(freeusd::sdf ALIAS freeusd_sdf)
target_include_directories(freeusd_sdf PUBLIC "$<BUILD_INTERFACE:${FREEUSD_INC}>" "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>")
target_link_libraries(freeusd_sdf PUBLIC freeusd_tf freeusd_vt freeusd_build_settings)

add_library(freeusd_io STATIC io/usda.cpp)
add_library(freeusd::io ALIAS freeusd_io)
target_include_directories(freeusd_io PUBLIC "$<BUILD_INTERFACE:${FREEUSD_INC}>" "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>")
target_link_libraries(freeusd_io PUBLIC freeusd_sdf freeusd_build_settings)

add_library(freeusd_pcp STATIC pcp/layerStack.cpp pcp/compose.cpp pcp/resolve.cpp)
add_library(freeusd::pcp ALIAS freeusd_pcp)
target_include_directories(freeusd_pcp PUBLIC "$<BUILD_INTERFACE:${FREEUSD_INC}>" "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>")
target_link_libraries(freeusd_pcp PUBLIC freeusd_sdf freeusd_build_settings)

add_library(freeusd_usd STATIC usd/stage.cpp usd/prim.cpp usd/crateFile.cpp)
add_library(freeusd::usd ALIAS freeusd_usd)
target_include_directories(freeusd_usd PUBLIC "$<BUILD_INTERFACE:${FREEUSD_INC}>" "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>")
target_link_libraries(freeusd_usd PUBLIC freeusd_sdf freeusd_vt freeusd_pcp freeusd_ar freeusd_io freeusd_build_settings)

add_library(freeusd_usdUtils INTERFACE)
add_library(freeusd::usdUtils ALIAS freeusd_usdUtils)
target_include_directories(freeusd_usdUtils INTERFACE "$<BUILD_INTERFACE:${FREEUSD_INC}>" "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>")
target_link_libraries(freeusd_usdUtils INTERFACE freeusd_usd)

add_library(freeusd_usdGeom INTERFACE)
add_library(freeusd::usdGeom ALIAS freeusd_usdGeom)
target_link_libraries(freeusd_usdGeom INTERFACE freeusd_usd)

# Header-only schema stubs (OpenUSD-shaped package names; clean-room tokens only).
foreach(
  _pkg
  IN
  ITEMS
  usdShade
  usdLux
  usdPhysics
  usdVol
  usdSkel
  usdMedia
  usdRender
  usdRi
  usdHydra
  usdMtlx
  usdProc
  usdSemantics
  usdUI)
  add_library(freeusd_${_pkg} INTERFACE)
  add_library(freeusd::${_pkg} ALIAS freeusd_${_pkg})
  target_include_directories(freeusd_${_pkg} INTERFACE "$<BUILD_INTERFACE:${FREEUSD_INC}>" "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>")
  target_link_libraries(freeusd_${_pkg} INTERFACE freeusd_usd freeusd_tf)
  set_target_properties(freeusd_${_pkg} PROPERTIES EXPORT_NAME ${_pkg})
endforeach()
target_link_libraries(freeusd_usdLux INTERFACE freeusd_usdGeom)
target_link_libraries(freeusd_usdPhysics INTERFACE freeusd_usdGeom)
target_link_libraries(freeusd_usdVol INTERFACE freeusd_usdGeom)
target_link_libraries(freeusd_usdSkel INTERFACE freeusd_usdGeom)

add_library(freeusd_usd_schemas INTERFACE)
add_library(freeusd::usd_schemas ALIAS freeusd_usd_schemas)
target_link_libraries(
  freeusd_usd_schemas
  INTERFACE
  freeusd_usdGeom
  freeusd_usdShade
  freeusd_usdLux
  freeusd_usdPhysics
  freeusd_usdVol
  freeusd_usdSkel
  freeusd_usdMedia
  freeusd_usdRender
  freeusd_usdRi
  freeusd_usdHydra
  freeusd_usdMtlx
  freeusd_usdProc
  freeusd_usdSemantics
  freeusd_usdUI)

add_library(freeusd_plug STATIC plug/registry.cpp)
add_library(freeusd::plug ALIAS freeusd_plug)
target_include_directories(freeusd_plug PUBLIC "$<BUILD_INTERFACE:${FREEUSD_INC}>" "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>")
target_link_libraries(freeusd_plug PUBLIC freeusd_build_settings)

add_library(freeusd_trace INTERFACE)
add_library(freeusd::trace ALIAS freeusd_trace)
target_include_directories(freeusd_trace INTERFACE "$<BUILD_INTERFACE:${FREEUSD_INC}>" "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>")

add_library(freeusd_work INTERFACE)
add_library(freeusd::work ALIAS freeusd_work)
target_include_directories(freeusd_work INTERFACE "$<BUILD_INTERFACE:${FREEUSD_INC}>" "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>")

add_library(freeusd_runtime INTERFACE)
add_library(freeusd::runtime ALIAS freeusd_runtime)
target_link_libraries(
  freeusd_runtime
  INTERFACE
    freeusd_base
    freeusd_tf
    freeusd_gf
    freeusd_vt
    freeusd_ar
    freeusd_sdf
    freeusd_io
    freeusd_pcp
    freeusd_usd
    freeusd_usdGeom
    freeusd_plug
    freeusd_trace
    freeusd_work
)

if(FREEUSD_BUILD_C_ABI)
  add_library(freeusd_c STATIC c/api.cpp)
  add_library(freeusd::c ALIAS freeusd_c)
  target_include_directories(freeusd_c PUBLIC "$<BUILD_INTERFACE:${FREEUSD_INC}>" "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>")
  target_link_libraries(freeusd_c PUBLIC freeusd::runtime)
  set_target_properties(freeusd_c PROPERTIES EXPORT_NAME c)
endif()

# For install(EXPORT): imported targets are freeusd::<EXPORT_NAME> (matches ALIAS names).
set_target_properties(
  freeusd_build_settings
  PROPERTIES
  EXPORT_NAME build_settings)
set_target_properties(freeusd_base PROPERTIES EXPORT_NAME base)
set_target_properties(freeusd_tf PROPERTIES EXPORT_NAME tf)
set_target_properties(freeusd_gf PROPERTIES EXPORT_NAME gf)
set_target_properties(freeusd_vt PROPERTIES EXPORT_NAME vt)
set_target_properties(freeusd_ar PROPERTIES EXPORT_NAME ar)
set_target_properties(freeusd_sdf PROPERTIES EXPORT_NAME sdf)
set_target_properties(freeusd_io PROPERTIES EXPORT_NAME io)
set_target_properties(freeusd_pcp PROPERTIES EXPORT_NAME pcp)
set_target_properties(freeusd_usd PROPERTIES EXPORT_NAME usd)
set_target_properties(freeusd_usdUtils PROPERTIES EXPORT_NAME usdUtils)
set_target_properties(freeusd_usdGeom PROPERTIES EXPORT_NAME usdGeom)
set_target_properties(freeusd_usd_schemas PROPERTIES EXPORT_NAME usd_schemas)
set_target_properties(freeusd_plug PROPERTIES EXPORT_NAME plug)
set_target_properties(freeusd_trace PROPERTIES EXPORT_NAME trace)
set_target_properties(freeusd_work PROPERTIES EXPORT_NAME work)
set_target_properties(freeusd_runtime PROPERTIES EXPORT_NAME runtime)
