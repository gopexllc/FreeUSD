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

add_library(freeusd_gf STATIC gf/matrix4d.cpp)
add_library(freeusd::gf ALIAS freeusd_gf)
target_include_directories(freeusd_gf PUBLIC "$<BUILD_INTERFACE:${FREEUSD_INC}>" "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>")
target_link_libraries(freeusd_gf PUBLIC freeusd_build_settings)

add_library(freeusd_vt STATIC vt/value.cpp)
add_library(freeusd::vt ALIAS freeusd_vt)
target_include_directories(freeusd_vt PUBLIC "$<BUILD_INTERFACE:${FREEUSD_INC}>" "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>")
target_link_libraries(freeusd_vt PUBLIC freeusd_tf freeusd_gf freeusd_build_settings)

add_library(freeusd_ar STATIC ar/defaultResolver.cpp ar/pathSecurity.cpp)
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
target_link_libraries(freeusd_io PUBLIC freeusd_sdf freeusd_ar freeusd_build_settings)

add_library(freeusd_pcp STATIC pcp/layerStack.cpp pcp/compose.cpp pcp/resolve.cpp)
add_library(freeusd::pcp ALIAS freeusd_pcp)
target_include_directories(freeusd_pcp PUBLIC "$<BUILD_INTERFACE:${FREEUSD_INC}>" "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>")
target_link_libraries(freeusd_pcp PUBLIC freeusd_sdf freeusd_build_settings)

find_package(ZLIB REQUIRED)
find_library(LZ4_LIBRARY NAMES lz4 REQUIRED)

add_library(freeusd_usd STATIC usd/stage.cpp usd/prim.cpp usd/crateFile.cpp)
add_library(freeusd::usd ALIAS freeusd_usd)
target_include_directories(freeusd_usd PUBLIC "$<BUILD_INTERFACE:${FREEUSD_INC}>" "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>")
target_link_libraries(freeusd_usd PUBLIC freeusd_sdf freeusd_vt freeusd_pcp freeusd_ar freeusd_io freeusd_build_settings)
target_include_directories(freeusd_usd PRIVATE ${ZLIB_INCLUDE_DIRS})
target_link_libraries(freeusd_usd PRIVATE ${ZLIB_LIBRARIES} ${LZ4_LIBRARY})

add_library(freeusd_usdUtils STATIC usdUtils/pipeline.cpp usdUtils/engineScene.cpp)
add_library(freeusd::usdUtils ALIAS freeusd_usdUtils)
target_include_directories(freeusd_usdUtils PUBLIC "$<BUILD_INTERFACE:${FREEUSD_INC}>" "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>")
target_link_libraries(freeusd_usdUtils PUBLIC freeusd_usd freeusd_sdf freeusd_build_settings)

add_library(freeusd_usdGeom STATIC usdGeom/xformable.cpp usdGeom/imageable.cpp usdGeom/boundable.cpp usdGeom/mesh.cpp)
add_library(freeusd::usdGeom ALIAS freeusd_usdGeom)
target_include_directories(freeusd_usdGeom PUBLIC "$<BUILD_INTERFACE:${FREEUSD_INC}>" "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>")

target_link_libraries(freeusd_usdGeom PUBLIC freeusd_usd freeusd_gf freeusd_build_settings)

add_library(freeusd_usdSkel STATIC usdSkel/skeleton.cpp usdSkel/skelAnimation.cpp usdSkel/gltfMapping.cpp
    usdSkel/skelBinding.cpp usdSkel/skelRoot.cpp usdSkel/blendShape.cpp usdSkel/skelBlendShapes.cpp
    usdSkel/morphTargets.cpp usdSkel/skinning.cpp)
add_library(freeusd::usdSkel ALIAS freeusd_usdSkel)
target_include_directories(freeusd_usdSkel PUBLIC "$<BUILD_INTERFACE:${FREEUSD_INC}>" "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>")
target_link_libraries(freeusd_usdSkel PUBLIC freeusd_usd freeusd_gf freeusd_vt freeusd_tf freeusd_build_settings)
target_link_libraries(freeusd_usdSkel INTERFACE freeusd_usdGeom)

add_library(freeusd_usdShade STATIC usdShade/material.cpp usdShade/shader.cpp usdShade/previewSurface.cpp)
add_library(freeusd::usdShade ALIAS freeusd_usdShade)
target_include_directories(freeusd_usdShade PUBLIC "$<BUILD_INTERFACE:${FREEUSD_INC}>" "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>")
target_link_libraries(freeusd_usdShade PUBLIC freeusd_usd freeusd_gf freeusd_vt freeusd_tf freeusd_build_settings)

add_library(freeusd_usdLux STATIC usdLux/distantLight.cpp usdLux/sphereLight.cpp usdLux/rectLight.cpp
    usdLux/diskLight.cpp usdLux/cylinderLight.cpp usdLux/domeLight.cpp)
add_library(freeusd::usdLux ALIAS freeusd_usdLux)
target_include_directories(freeusd_usdLux PUBLIC "$<BUILD_INTERFACE:${FREEUSD_INC}>" "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>")
target_link_libraries(freeusd_usdLux PUBLIC freeusd_usd freeusd_gf freeusd_vt freeusd_tf freeusd_build_settings)
target_link_libraries(freeusd_usdLux INTERFACE freeusd_usdGeom)
set_target_properties(freeusd_usdLux PROPERTIES EXPORT_NAME usdLux)

target_link_libraries(freeusd_usdUtils PUBLIC freeusd_usdSkel freeusd_usdShade freeusd_usdLux freeusd_usdPhysics
    freeusd_usdVol)

add_library(freeusd_usdPhysics STATIC usdPhysics/physicsScene.cpp usdPhysics/rigidBodyAPI.cpp
    usdPhysics/collisionAPI.cpp usdPhysics/massAPI.cpp usdPhysics/fixedJoint.cpp)
add_library(freeusd::usdPhysics ALIAS freeusd_usdPhysics)
target_include_directories(freeusd_usdPhysics PUBLIC "$<BUILD_INTERFACE:${FREEUSD_INC}>" "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>")
target_link_libraries(freeusd_usdPhysics PUBLIC freeusd_usd freeusd_gf freeusd_vt freeusd_tf freeusd_build_settings)
target_link_libraries(freeusd_usdPhysics INTERFACE freeusd_usdGeom)
set_target_properties(freeusd_usdPhysics PROPERTIES EXPORT_NAME usdPhysics)

add_library(freeusd_usdVol STATIC usdVol/openVdbAsset.cpp usdVol/volume.cpp)
add_library(freeusd::usdVol ALIAS freeusd_usdVol)
target_include_directories(freeusd_usdVol PUBLIC "$<BUILD_INTERFACE:${FREEUSD_INC}>" "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>")
target_link_libraries(freeusd_usdVol PUBLIC freeusd_usd freeusd_vt freeusd_tf freeusd_build_settings)
target_link_libraries(freeusd_usdVol INTERFACE freeusd_usdGeom)
set_target_properties(freeusd_usdVol PROPERTIES EXPORT_NAME usdVol)

# Header-only schema stubs (OpenUSD-shaped package names; clean-room tokens only).
foreach(
  _pkg
  IN
  ITEMS
  usdMedia
  usdRender
  usdRi
  usdHydra
  usdMtlx
  usdProc
  usdUI)
  add_library(freeusd_${_pkg} INTERFACE)
  add_library(freeusd::${_pkg} ALIAS freeusd_${_pkg})
  target_include_directories(freeusd_${_pkg} INTERFACE "$<BUILD_INTERFACE:${FREEUSD_INC}>" "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>")
  target_link_libraries(freeusd_${_pkg} INTERFACE freeusd_usd freeusd_tf)
  set_target_properties(freeusd_${_pkg} PROPERTIES EXPORT_NAME ${_pkg})
endforeach()

add_library(freeusd_usdSemantics STATIC usdSemantics/labelsAPI.cpp)
add_library(freeusd::usdSemantics ALIAS freeusd_usdSemantics)
target_include_directories(freeusd_usdSemantics PUBLIC "$<BUILD_INTERFACE:${FREEUSD_INC}>" "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>")
target_link_libraries(freeusd_usdSemantics PUBLIC freeusd_usd freeusd_tf freeusd_build_settings)
set_target_properties(freeusd_usdSemantics PROPERTIES EXPORT_NAME usdSemantics)

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
    freeusd_usdUtils
    freeusd_usdGeom
    freeusd_usdSkel
    freeusd_usdShade
    freeusd_usdLux
    freeusd_usdPhysics
    freeusd_usdVol
    freeusd_usdSemantics
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
set_target_properties(freeusd_usdSkel PROPERTIES EXPORT_NAME usdSkel)
set_target_properties(freeusd_usdShade PROPERTIES EXPORT_NAME usdShade)
set_target_properties(freeusd_usdLux PROPERTIES EXPORT_NAME usdLux)
set_target_properties(freeusd_usdPhysics PROPERTIES EXPORT_NAME usdPhysics)
set_target_properties(freeusd_usdVol PROPERTIES EXPORT_NAME usdVol)
set_target_properties(freeusd_usdSemantics PROPERTIES EXPORT_NAME usdSemantics)
set_target_properties(freeusd_usd_schemas PROPERTIES EXPORT_NAME usd_schemas)
set_target_properties(freeusd_plug PROPERTIES EXPORT_NAME plug)
set_target_properties(freeusd_trace PROPERTIES EXPORT_NAME trace)
set_target_properties(freeusd_work PROPERTIES EXPORT_NAME work)
set_target_properties(freeusd_runtime PROPERTIES EXPORT_NAME runtime)
