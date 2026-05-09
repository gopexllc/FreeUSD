# Modular layout inspired by OpenUSD (pxr): tf, gf, vt, ar, sdf, pcp, usd, usdGeom, plus plug/trace/work stubs.
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
target_include_directories(freeusd_base PUBLIC "$<BUILD_INTERFACE:${FREEUSD_INC}>")
target_link_libraries(freeusd_base PUBLIC freeusd_build_settings)

add_library(freeusd_tf STATIC tf/token.cpp tf/hash.cpp)
add_library(freeusd::tf ALIAS freeusd_tf)
target_include_directories(freeusd_tf PUBLIC "$<BUILD_INTERFACE:${FREEUSD_INC}>")
target_link_libraries(freeusd_tf PUBLIC freeusd_build_settings)

add_library(freeusd_gf INTERFACE)
add_library(freeusd::gf ALIAS freeusd_gf)
target_include_directories(freeusd_gf INTERFACE "$<BUILD_INTERFACE:${FREEUSD_INC}>")

add_library(freeusd_vt STATIC vt/value.cpp)
add_library(freeusd::vt ALIAS freeusd_vt)
target_include_directories(freeusd_vt PUBLIC "$<BUILD_INTERFACE:${FREEUSD_INC}>")
target_link_libraries(freeusd_vt PUBLIC freeusd_tf freeusd_gf freeusd_build_settings)

add_library(freeusd_ar STATIC ar/defaultResolver.cpp)
add_library(freeusd::ar ALIAS freeusd_ar)
target_include_directories(freeusd_ar PUBLIC "$<BUILD_INTERFACE:${FREEUSD_INC}>")
target_link_libraries(freeusd_ar PUBLIC freeusd_build_settings)

add_library(freeusd_sdf STATIC sdf/path.cpp sdf/layer.cpp sdf/fieldOpinion.cpp sdf/primReference.cpp)
add_library(freeusd::sdf ALIAS freeusd_sdf)
target_include_directories(freeusd_sdf PUBLIC "$<BUILD_INTERFACE:${FREEUSD_INC}>")
target_link_libraries(freeusd_sdf PUBLIC freeusd_tf freeusd_vt freeusd_build_settings)

add_library(freeusd_io STATIC io/usda.cpp)
add_library(freeusd::io ALIAS freeusd_io)
target_include_directories(freeusd_io PUBLIC "$<BUILD_INTERFACE:${FREEUSD_INC}>")
target_link_libraries(freeusd_io PUBLIC freeusd_sdf freeusd_build_settings)

add_library(freeusd_pcp STATIC pcp/layerStack.cpp pcp/compose.cpp pcp/resolve.cpp)
add_library(freeusd::pcp ALIAS freeusd_pcp)
target_include_directories(freeusd_pcp PUBLIC "$<BUILD_INTERFACE:${FREEUSD_INC}>")
target_link_libraries(freeusd_pcp PUBLIC freeusd_sdf freeusd_build_settings)

add_library(freeusd_usd STATIC usd/stage.cpp usd/prim.cpp)
add_library(freeusd::usd ALIAS freeusd_usd)
target_include_directories(freeusd_usd PUBLIC "$<BUILD_INTERFACE:${FREEUSD_INC}>")
target_link_libraries(freeusd_usd PUBLIC freeusd_sdf freeusd_vt freeusd_pcp freeusd_ar freeusd_build_settings)

add_library(freeusd_usdGeom INTERFACE)
add_library(freeusd::usdGeom ALIAS freeusd_usdGeom)
target_link_libraries(freeusd_usdGeom INTERFACE freeusd_usd)

add_library(freeusd_plug STATIC plug/registry.cpp)
add_library(freeusd::plug ALIAS freeusd_plug)
target_include_directories(freeusd_plug PUBLIC "$<BUILD_INTERFACE:${FREEUSD_INC}>")
target_link_libraries(freeusd_plug PUBLIC freeusd_build_settings)

add_library(freeusd_trace INTERFACE)
add_library(freeusd::trace ALIAS freeusd_trace)
target_include_directories(freeusd_trace INTERFACE "$<BUILD_INTERFACE:${FREEUSD_INC}>")

add_library(freeusd_work INTERFACE)
add_library(freeusd::work ALIAS freeusd_work)
target_include_directories(freeusd_work INTERFACE "$<BUILD_INTERFACE:${FREEUSD_INC}>")

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
  target_include_directories(freeusd_c PUBLIC "$<BUILD_INTERFACE:${FREEUSD_INC}>")
  target_link_libraries(freeusd_c PUBLIC freeusd::runtime)
endif()
