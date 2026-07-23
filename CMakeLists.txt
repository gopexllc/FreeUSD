cmake_minimum_required(VERSION 3.20)
project(freeusd VERSION 0.1.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

option(FREEUSD_BUILD_PYTHON "Build pybind11 Python extension" ON)
option(FREEUSD_BUILD_TESTS "Build and register C++ self-tests" ON)
option(FREEUSD_BUILD_C_ABI "Build libfreeusd_c C API wrapper" ON)
option(FREEUSD_TEST_INSTALL_INTEGRATION "CTest: full configure+install+find_package smoke (slow)" OFF)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

add_subdirectory(src)

if(FREEUSD_BUILD_TESTS)
  enable_testing()

  function(freeusd_enable_test_asserts target_name)
    if(NOT TARGET "${target_name}")
      return()
    endif()
    if(MSVC)
      target_compile_options("${target_name}" PRIVATE /UNDEBUG)
    else()
      target_compile_options("${target_name}" PRIVATE -UNDEBUG)
    endif()
  endfunction()

  add_executable(freeusd_selftest tests/selftest.cpp)
  target_link_libraries(freeusd_selftest PRIVATE freeusd::base)

  add_executable(freeusd_sdf_path_test tests/sdf_path_test.cpp)
  target_link_libraries(freeusd_sdf_path_test PRIVATE freeusd::sdf)

  add_executable(freeusd_usd_smoke_test tests/usd_smoke_test.cpp)
  target_link_libraries(freeusd_usd_smoke_test PRIVATE freeusd::usd freeusd::usdUtils freeusd::usdGeom freeusd::plug)

  add_executable(freeusd_usd_cross_language_test tests/usd_cross_language_test.cpp)
  target_link_libraries(freeusd_usd_cross_language_test PRIVATE freeusd::io freeusd::usd)
  target_compile_definitions(
    freeusd_usd_cross_language_test
    PRIVATE "FREEUSD_TEST_FIXTURES_DIR=\"${CMAKE_SOURCE_DIR}/tests/fixtures\"")

  add_executable(freeusd_usda_roundtrip_test tests/usda_roundtrip_test.cpp)
  target_link_libraries(freeusd_usda_roundtrip_test PRIVATE freeusd::io freeusd::usd)

  add_executable(freeusd_pcp_compose_test tests/pcp_compose_test.cpp)
  target_link_libraries(freeusd_pcp_compose_test PRIVATE freeusd::pcp)

  add_executable(freeusd_stage_compose_test tests/stage_compose_test.cpp)
  target_link_libraries(freeusd_stage_compose_test PRIVATE freeusd::usd)

  add_executable(freeusd_stage_open_file_test tests/stage_open_file_test.cpp)
  target_link_libraries(freeusd_stage_open_file_test PRIVATE freeusd::usd)

  add_executable(freeusd_parity_fixtures_test tests/parity_fixtures_test.cpp)
  target_link_libraries(freeusd_parity_fixtures_test PRIVATE freeusd::usd freeusd::usdUtils freeusd::usdGeom freeusd::usdSkel
                                                                 freeusd::usdShade freeusd::usdLux freeusd::usdPhysics
                                                                 freeusd::usdVol)
  target_compile_definitions(
    freeusd_parity_fixtures_test PRIVATE "FREEUSD_TEST_FIXTURES_DIR=\"${CMAKE_SOURCE_DIR}/tests/fixtures\"")

  add_executable(freeusd_usdc_fields_test tests/usdc_fields_test.cpp)
  target_link_libraries(freeusd_usdc_fields_test PRIVATE freeusd::usd)
  target_compile_definitions(
    freeusd_usdc_fields_test PRIVATE "FREEUSD_TEST_FIXTURES_DIR=\"${CMAKE_SOURCE_DIR}/tests/fixtures\"")

  add_executable(freeusd_usdc_specs_test tests/usdc_specs_test.cpp)
  target_link_libraries(freeusd_usdc_specs_test PRIVATE freeusd::usd)
  target_compile_definitions(
    freeusd_usdc_specs_test PRIVATE "FREEUSD_TEST_FIXTURES_DIR=\"${CMAKE_SOURCE_DIR}/tests/fixtures\"")

  add_executable(freeusd_usdc_fieldsets_test tests/usdc_fieldsets_test.cpp)
  target_link_libraries(freeusd_usdc_fieldsets_test PRIVATE freeusd::usd)
  target_compile_definitions(
    freeusd_usdc_fieldsets_test PRIVATE "FREEUSD_TEST_FIXTURES_DIR=\"${CMAKE_SOURCE_DIR}/tests/fixtures\"")

  add_executable(freeusd_usdc_values_test tests/usdc_values_test.cpp)
  target_link_libraries(freeusd_usdc_values_test PRIVATE freeusd::usd)
  target_compile_definitions(
    freeusd_usdc_values_test PRIVATE "FREEUSD_TEST_FIXTURES_DIR=\"${CMAKE_SOURCE_DIR}/tests/fixtures\"")

  add_executable(freeusd_usd_geom_mesh_test tests/usd_geom_mesh_test.cpp)
  target_link_libraries(freeusd_usd_geom_mesh_test PRIVATE freeusd::usd freeusd::usdGeom)
  target_compile_definitions(
    freeusd_usd_geom_mesh_test PRIVATE "FREEUSD_TEST_FIXTURES_DIR=\"${CMAKE_SOURCE_DIR}/tests/fixtures\"")

  add_executable(freeusd_usd_geom_boundable_mesh_test tests/usd_geom_boundable_mesh_test.cpp)
  target_link_libraries(freeusd_usd_geom_boundable_mesh_test PRIVATE freeusd::usd freeusd::usdGeom freeusd::usdUtils)
  target_compile_definitions(
    freeusd_usd_geom_boundable_mesh_test PRIVATE "FREEUSD_TEST_FIXTURES_DIR=\"${CMAKE_SOURCE_DIR}/tests/fixtures\"")

  add_executable(freeusd_usd_skel_smoke_test tests/usd_skel_smoke_test.cpp)
  target_link_libraries(freeusd_usd_skel_smoke_test PRIVATE freeusd::usd freeusd::usdSkel)
  target_compile_definitions(
    freeusd_usd_skel_smoke_test PRIVATE "FREEUSD_TEST_FIXTURES_DIR=\"${CMAKE_SOURCE_DIR}/tests/fixtures\"")

  add_executable(freeusd_usd_shade_smoke_test tests/usd_shade_smoke_test.cpp)
  target_link_libraries(freeusd_usd_shade_smoke_test PRIVATE freeusd::usd freeusd::usdShade)
  target_compile_definitions(
    freeusd_usd_shade_smoke_test PRIVATE "FREEUSD_TEST_FIXTURES_DIR=\"${CMAKE_SOURCE_DIR}/tests/fixtures\"")

  add_executable(freeusd_usd_skel_blend_shapes_test tests/usd_skel_blend_shapes_test.cpp)
  target_link_libraries(freeusd_usd_skel_blend_shapes_test PRIVATE freeusd::usd freeusd::usdSkel)
  target_compile_definitions(
    freeusd_usd_skel_blend_shapes_test PRIVATE "FREEUSD_TEST_FIXTURES_DIR=\"${CMAKE_SOURCE_DIR}/tests/fixtures\"")

  add_executable(freeusd_usd_skel_skinning_matrices_test tests/usd_skel_skinning_matrices_test.cpp)
  target_link_libraries(freeusd_usd_skel_skinning_matrices_test PRIVATE freeusd::usdSkel freeusd::gf)

  add_executable(freeusd_stage_kind_active_test tests/stage_kind_active_test.cpp)
  target_link_libraries(freeusd_stage_kind_active_test PRIVATE freeusd::usd)
  target_compile_definitions(
    freeusd_stage_kind_active_test PRIVATE "FREEUSD_TEST_FIXTURES_DIR=\"${CMAKE_SOURCE_DIR}/tests/fixtures\"")

  add_executable(freeusd_usd_lux_distant_light_test tests/usd_lux_distant_light_test.cpp)
  target_link_libraries(freeusd_usd_lux_distant_light_test PRIVATE freeusd::usd freeusd::usdLux)
  target_compile_definitions(
    freeusd_usd_lux_distant_light_test PRIVATE "FREEUSD_TEST_FIXTURES_DIR=\"${CMAKE_SOURCE_DIR}/tests/fixtures\"")

  add_executable(freeusd_usd_lux_sphere_light_test tests/usd_lux_sphere_light_test.cpp)
  target_link_libraries(freeusd_usd_lux_sphere_light_test PRIVATE freeusd::usd freeusd::usdLux)
  target_compile_definitions(
    freeusd_usd_lux_sphere_light_test PRIVATE "FREEUSD_TEST_FIXTURES_DIR=\"${CMAKE_SOURCE_DIR}/tests/fixtures\"")

  add_executable(freeusd_usd_lux_rect_light_test tests/usd_lux_rect_light_test.cpp)
  target_link_libraries(freeusd_usd_lux_rect_light_test PRIVATE freeusd::usd freeusd::usdLux)
  target_compile_definitions(
    freeusd_usd_lux_rect_light_test PRIVATE "FREEUSD_TEST_FIXTURES_DIR=\"${CMAKE_SOURCE_DIR}/tests/fixtures\"")

  add_executable(freeusd_usd_shade_texture_test tests/usd_shade_texture_test.cpp)
  target_link_libraries(freeusd_usd_shade_texture_test PRIVATE freeusd::usd freeusd::usdShade)
  target_compile_definitions(
    freeusd_usd_shade_texture_test PRIVATE "FREEUSD_TEST_FIXTURES_DIR=\"${CMAKE_SOURCE_DIR}/tests/fixtures\"")

  add_executable(freeusd_usd_shade_pbr_textures_test tests/usd_shade_pbr_textures_test.cpp)
  target_link_libraries(freeusd_usd_shade_pbr_textures_test PRIVATE freeusd::usd freeusd::usdShade)
  target_compile_definitions(
    freeusd_usd_shade_pbr_textures_test PRIVATE "FREEUSD_TEST_FIXTURES_DIR=\"${CMAKE_SOURCE_DIR}/tests/fixtures\"")

  add_executable(freeusd_usd_lux_disk_light_test tests/usd_lux_disk_light_test.cpp)
  target_link_libraries(freeusd_usd_lux_disk_light_test PRIVATE freeusd::usd freeusd::usdLux)
  target_compile_definitions(
    freeusd_usd_lux_disk_light_test PRIVATE "FREEUSD_TEST_FIXTURES_DIR=\"${CMAKE_SOURCE_DIR}/tests/fixtures\"")

  add_executable(freeusd_usd_lux_cylinder_light_test tests/usd_lux_cylinder_light_test.cpp)
  target_link_libraries(freeusd_usd_lux_cylinder_light_test PRIVATE freeusd::usd freeusd::usdLux)
  target_compile_definitions(
    freeusd_usd_lux_cylinder_light_test PRIVATE "FREEUSD_TEST_FIXTURES_DIR=\"${CMAKE_SOURCE_DIR}/tests/fixtures\"")

  add_executable(freeusd_usd_lux_dome_light_test tests/usd_lux_dome_light_test.cpp)
  target_link_libraries(freeusd_usd_lux_dome_light_test PRIVATE freeusd::usd freeusd::usdLux)
  target_compile_definitions(
    freeusd_usd_lux_dome_light_test PRIVATE "FREEUSD_TEST_FIXTURES_DIR=\"${CMAKE_SOURCE_DIR}/tests/fixtures\"")

  add_executable(freeusd_usd_physics_scene_test tests/usd_physics_scene_test.cpp)
  target_link_libraries(freeusd_usd_physics_scene_test PRIVATE freeusd::usd freeusd::usdPhysics)
  target_compile_definitions(
    freeusd_usd_physics_scene_test PRIVATE "FREEUSD_TEST_FIXTURES_DIR=\"${CMAKE_SOURCE_DIR}/tests/fixtures\"")

  add_executable(freeusd_usd_physics_rigid_body_test tests/usd_physics_rigid_body_test.cpp)
  target_link_libraries(freeusd_usd_physics_rigid_body_test PRIVATE freeusd::usd freeusd::usdPhysics)
  target_compile_definitions(
    freeusd_usd_physics_rigid_body_test PRIVATE "FREEUSD_TEST_FIXTURES_DIR=\"${CMAKE_SOURCE_DIR}/tests/fixtures\"")

  add_executable(freeusd_usd_physics_collision_test tests/usd_physics_collision_test.cpp)
  target_link_libraries(freeusd_usd_physics_collision_test PRIVATE freeusd::usd freeusd::usdPhysics)
  target_compile_definitions(
    freeusd_usd_physics_collision_test PRIVATE "FREEUSD_TEST_FIXTURES_DIR=\"${CMAKE_SOURCE_DIR}/tests/fixtures\"")

  add_executable(freeusd_usd_physics_mass_test tests/usd_physics_mass_test.cpp)
  target_link_libraries(freeusd_usd_physics_mass_test PRIVATE freeusd::usd freeusd::usdPhysics)
  target_compile_definitions(
    freeusd_usd_physics_mass_test PRIVATE "FREEUSD_TEST_FIXTURES_DIR=\"${CMAKE_SOURCE_DIR}/tests/fixtures\"")

  add_executable(freeusd_usd_physics_fixed_joint_test tests/usd_physics_fixed_joint_test.cpp)
  target_link_libraries(freeusd_usd_physics_fixed_joint_test PRIVATE freeusd::usd freeusd::usdPhysics)
  target_compile_definitions(
    freeusd_usd_physics_fixed_joint_test PRIVATE "FREEUSD_TEST_FIXTURES_DIR=\"${CMAKE_SOURCE_DIR}/tests/fixtures\"")

  add_executable(freeusd_usd_vol_openvdb_test tests/usd_vol_openvdb_test.cpp)
  target_link_libraries(freeusd_usd_vol_openvdb_test PRIVATE freeusd::usd freeusd::usdVol)
  target_compile_definitions(
    freeusd_usd_vol_openvdb_test PRIVATE "FREEUSD_TEST_FIXTURES_DIR=\"${CMAKE_SOURCE_DIR}/tests/fixtures\"")

  add_executable(freeusd_usd_vol_volume_test tests/usd_vol_volume_test.cpp)
  target_link_libraries(freeusd_usd_vol_volume_test PRIVATE freeusd::usd freeusd::usdVol)
  target_compile_definitions(
    freeusd_usd_vol_volume_test PRIVATE "FREEUSD_TEST_FIXTURES_DIR=\"${CMAKE_SOURCE_DIR}/tests/fixtures\"")

  add_executable(freeusd_ar_path_security_test tests/ar_path_security_test.cpp)
  target_link_libraries(freeusd_ar_path_security_test PRIVATE freeusd::ar)

  add_executable(freeusd_engine_integration_test tests/engine_integration_test.cpp)
  target_link_libraries(freeusd_engine_integration_test PRIVATE freeusd::usd freeusd::usdUtils freeusd::usdGeom freeusd::usdSkel
                                                                 freeusd::usdShade)
  target_compile_definitions(
    freeusd_engine_integration_test PRIVATE "FREEUSD_TEST_FIXTURES_DIR=\"${CMAKE_SOURCE_DIR}/tests/fixtures\"")

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
  target_link_libraries(freeusd_gf_quatd_test PRIVATE freeusd::gf)

  add_executable(freeusd_gf_bbox3d_test tests/gf_bbox3d_test.cpp)
  target_link_libraries(freeusd_gf_bbox3d_test PRIVATE freeusd::gf)

  add_executable(freeusd_gf_matrix4d_test tests/gf_matrix4d_test.cpp)
  target_link_libraries(freeusd_gf_matrix4d_test PRIVATE freeusd::gf)

  add_executable(freeusd_ar_resolved_path_test tests/ar_resolved_path_test.cpp)
  target_include_directories(freeusd_ar_resolved_path_test PRIVATE "${PROJECT_SOURCE_DIR}/include")

  add_executable(freeusd_sdf_tokens_test tests/sdf_tokens_test.cpp)
  target_link_libraries(freeusd_sdf_tokens_test PRIVATE freeusd::tf)

  add_executable(freeusd_usd_tokens_test tests/usd_tokens_test.cpp)
  target_link_libraries(freeusd_usd_tokens_test PRIVATE freeusd::tf)

  add_test(NAME freeusd_selftest COMMAND freeusd_selftest)
  add_test(NAME freeusd_sdf_path_test COMMAND freeusd_sdf_path_test)
  add_test(NAME freeusd_usd_smoke_test COMMAND freeusd_usd_smoke_test)
  add_test(NAME freeusd_usd_cross_language_test COMMAND freeusd_usd_cross_language_test)
  add_test(NAME freeusd_usda_roundtrip_test COMMAND freeusd_usda_roundtrip_test)
  add_test(NAME freeusd_pcp_compose_test COMMAND freeusd_pcp_compose_test)
  add_test(NAME freeusd_stage_compose_test COMMAND freeusd_stage_compose_test)
  add_test(NAME freeusd_stage_open_file_test COMMAND freeusd_stage_open_file_test)
  add_test(NAME freeusd_parity_fixtures_test COMMAND freeusd_parity_fixtures_test)
  add_test(NAME freeusd_usdc_fields_test COMMAND freeusd_usdc_fields_test)
  add_test(NAME freeusd_usdc_specs_test COMMAND freeusd_usdc_specs_test)
  add_test(NAME freeusd_usdc_fieldsets_test COMMAND freeusd_usdc_fieldsets_test)
  add_test(NAME freeusd_usdc_values_test COMMAND freeusd_usdc_values_test)
  add_test(NAME freeusd_usd_geom_mesh_test COMMAND freeusd_usd_geom_mesh_test)
  add_test(NAME freeusd_usd_geom_boundable_mesh_test COMMAND freeusd_usd_geom_boundable_mesh_test)
  add_test(NAME freeusd_usd_skel_smoke_test COMMAND freeusd_usd_skel_smoke_test)
  add_test(NAME freeusd_usd_shade_smoke_test COMMAND freeusd_usd_shade_smoke_test)
  add_test(NAME freeusd_usd_skel_blend_shapes_test COMMAND freeusd_usd_skel_blend_shapes_test)
  add_test(NAME freeusd_usd_skel_skinning_matrices_test COMMAND freeusd_usd_skel_skinning_matrices_test)
  add_test(NAME freeusd_stage_kind_active_test COMMAND freeusd_stage_kind_active_test)
  add_test(NAME freeusd_usd_lux_distant_light_test COMMAND freeusd_usd_lux_distant_light_test)
  add_test(NAME freeusd_usd_lux_sphere_light_test COMMAND freeusd_usd_lux_sphere_light_test)
  add_test(NAME freeusd_usd_lux_rect_light_test COMMAND freeusd_usd_lux_rect_light_test)
  add_test(NAME freeusd_usd_shade_texture_test COMMAND freeusd_usd_shade_texture_test)
  add_test(NAME freeusd_usd_shade_pbr_textures_test COMMAND freeusd_usd_shade_pbr_textures_test)
  add_test(NAME freeusd_usd_lux_disk_light_test COMMAND freeusd_usd_lux_disk_light_test)
  add_test(NAME freeusd_usd_lux_cylinder_light_test COMMAND freeusd_usd_lux_cylinder_light_test)
  add_test(NAME freeusd_usd_lux_dome_light_test COMMAND freeusd_usd_lux_dome_light_test)
  add_test(NAME freeusd_usd_physics_scene_test COMMAND freeusd_usd_physics_scene_test)
  add_test(NAME freeusd_usd_physics_rigid_body_test COMMAND freeusd_usd_physics_rigid_body_test)
  add_test(NAME freeusd_usd_physics_collision_test COMMAND freeusd_usd_physics_collision_test)
  add_test(NAME freeusd_usd_physics_mass_test COMMAND freeusd_usd_physics_mass_test)
  add_test(NAME freeusd_usd_physics_fixed_joint_test COMMAND freeusd_usd_physics_fixed_joint_test)
  add_test(NAME freeusd_usd_vol_openvdb_test COMMAND freeusd_usd_vol_openvdb_test)
  add_test(NAME freeusd_usd_vol_volume_test COMMAND freeusd_usd_vol_volume_test)
  add_test(NAME freeusd_ar_path_security_test COMMAND freeusd_ar_path_security_test)
  add_test(NAME freeusd_engine_integration_test COMMAND freeusd_engine_integration_test)
  add_test(NAME freeusd_schema_tokens_bundle_test COMMAND freeusd_schema_tokens_bundle_test)
  add_test(NAME freeusd_time_code_test COMMAND freeusd_time_code_test)
  add_test(NAME freeusd_edit_target_test COMMAND freeusd_edit_target_test)
  add_test(NAME freeusd_sdf_layer_offset_test COMMAND freeusd_sdf_layer_offset_test)
  add_test(NAME freeusd_sdf_asset_path_test COMMAND freeusd_sdf_asset_path_test)
  add_test(NAME freeusd_gf_range1d_test COMMAND freeusd_gf_range1d_test)
  add_test(NAME freeusd_gf_quatd_test COMMAND freeusd_gf_quatd_test)
  add_test(NAME freeusd_gf_bbox3d_test COMMAND freeusd_gf_bbox3d_test)
  add_test(NAME freeusd_gf_matrix4d_test COMMAND freeusd_gf_matrix4d_test)
  add_test(NAME freeusd_ar_resolved_path_test COMMAND freeusd_ar_resolved_path_test)
  add_test(NAME freeusd_sdf_tokens_test COMMAND freeusd_sdf_tokens_test)
  add_test(NAME freeusd_usd_tokens_test COMMAND freeusd_usd_tokens_test)

  if(FREEUSD_BUILD_C_ABI)
    add_executable(freeusd_c_smoke_test tests/c_abi_smoke.c)
    target_include_directories(freeusd_c_smoke_test PRIVATE "${PROJECT_SOURCE_DIR}/include")
    target_link_libraries(freeusd_c_smoke_test PRIVATE freeusd::c)
    target_compile_definitions(
      freeusd_c_smoke_test PRIVATE "FREEUSD_TEST_FIXTURES_DIR=\"${CMAKE_SOURCE_DIR}/tests/fixtures\"")
    set_target_properties(freeusd_c_smoke_test PROPERTIES C_STANDARD 11 C_STANDARD_REQUIRED ON LINKER_LANGUAGE CXX)
    add_test(NAME freeusd_c_smoke_test COMMAND freeusd_c_smoke_test)

    add_executable(freeusd_c_usd_cross_language tests/c_usd_cross_language.c)
    target_include_directories(freeusd_c_usd_cross_language PRIVATE "${PROJECT_SOURCE_DIR}/include")
    target_link_libraries(freeusd_c_usd_cross_language PRIVATE freeusd::c)
    target_compile_definitions(
      freeusd_c_usd_cross_language PRIVATE "FREEUSD_TEST_FIXTURES_DIR=\"${CMAKE_SOURCE_DIR}/tests/fixtures\"")
    set_target_properties(freeusd_c_usd_cross_language PROPERTIES C_STANDARD 11 C_STANDARD_REQUIRED ON LINKER_LANGUAGE CXX)
    add_test(NAME freeusd_c_usd_cross_language COMMAND freeusd_c_usd_cross_language)

    add_executable(freeusd_c_stage_composition_arcs tests/c_stage_composition_arcs.c)
    target_include_directories(freeusd_c_stage_composition_arcs PRIVATE "${PROJECT_SOURCE_DIR}/include")
    target_link_libraries(freeusd_c_stage_composition_arcs PRIVATE freeusd::c)
    set_target_properties(freeusd_c_stage_composition_arcs PROPERTIES C_STANDARD 11 C_STANDARD_REQUIRED ON LINKER_LANGUAGE CXX)
    add_test(NAME freeusd_c_stage_composition_arcs COMMAND freeusd_c_stage_composition_arcs)

    add_executable(freeusd_c_stage_open_from_file tests/c_stage_open_from_file.c)
    target_include_directories(freeusd_c_stage_open_from_file PRIVATE "${PROJECT_SOURCE_DIR}/include")
    target_link_libraries(freeusd_c_stage_open_from_file PRIVATE freeusd::c)
    target_compile_definitions(
      freeusd_c_stage_open_from_file PRIVATE "FREEUSD_TEST_FIXTURES_DIR=\"${CMAKE_SOURCE_DIR}/tests/fixtures\"")
    set_target_properties(freeusd_c_stage_open_from_file PROPERTIES C_STANDARD 11 C_STANDARD_REQUIRED ON LINKER_LANGUAGE CXX)
    add_test(NAME freeusd_c_stage_open_from_file COMMAND freeusd_c_stage_open_from_file)

    add_executable(freeusd_c_stage_relocate tests/c_stage_relocate.c)
    target_include_directories(freeusd_c_stage_relocate PRIVATE "${PROJECT_SOURCE_DIR}/include")
    target_link_libraries(freeusd_c_stage_relocate PRIVATE freeusd::c)
    set_target_properties(freeusd_c_stage_relocate PROPERTIES C_STANDARD 11 C_STANDARD_REQUIRED ON LINKER_LANGUAGE CXX)
    add_test(NAME freeusd_c_stage_relocate COMMAND freeusd_c_stage_relocate)

    add_executable(freeusd_c_stage_prefix_substitution tests/c_stage_prefix_substitution.c)
    target_include_directories(freeusd_c_stage_prefix_substitution PRIVATE "${PROJECT_SOURCE_DIR}/include")
    target_link_libraries(freeusd_c_stage_prefix_substitution PRIVATE freeusd::c)
    set_target_properties(
      freeusd_c_stage_prefix_substitution
      PROPERTIES C_STANDARD 11 C_STANDARD_REQUIRED ON LINKER_LANGUAGE CXX)
    add_test(NAME freeusd_c_stage_prefix_substitution COMMAND freeusd_c_stage_prefix_substitution)

    add_executable(freeusd_c_stage_custom_layer_data tests/c_stage_custom_layer_data.c)
    target_include_directories(freeusd_c_stage_custom_layer_data PRIVATE "${PROJECT_SOURCE_DIR}/include")
    target_link_libraries(freeusd_c_stage_custom_layer_data PRIVATE freeusd::c)
    set_target_properties(
      freeusd_c_stage_custom_layer_data
      PROPERTIES C_STANDARD 11 C_STANDARD_REQUIRED ON LINKER_LANGUAGE CXX)
    add_test(NAME freeusd_c_stage_custom_layer_data COMMAND freeusd_c_stage_custom_layer_data)

    add_executable(freeusd_c_stage_variant tests/c_stage_variant.c)
    target_include_directories(freeusd_c_stage_variant PRIVATE "${PROJECT_SOURCE_DIR}/include")
    target_compile_definitions(
      freeusd_c_stage_variant PRIVATE "FREEUSD_TEST_FIXTURES_DIR=\"${CMAKE_SOURCE_DIR}/tests/fixtures\"")
    target_link_libraries(freeusd_c_stage_variant PRIVATE freeusd::c)
    set_target_properties(freeusd_c_stage_variant PROPERTIES C_STANDARD 11 C_STANDARD_REQUIRED ON LINKER_LANGUAGE CXX)
    add_test(NAME freeusd_c_stage_variant COMMAND freeusd_c_stage_variant)

    add_executable(freeusd_c_stage_prim_specifier tests/c_stage_prim_specifier.c)
    target_include_directories(freeusd_c_stage_prim_specifier PRIVATE "${PROJECT_SOURCE_DIR}/include")
    target_link_libraries(freeusd_c_stage_prim_specifier PRIVATE freeusd::c)
    set_target_properties(
      freeusd_c_stage_prim_specifier
      PROPERTIES C_STANDARD 11 C_STANDARD_REQUIRED ON LINKER_LANGUAGE CXX)
    add_test(NAME freeusd_c_stage_prim_specifier COMMAND freeusd_c_stage_prim_specifier)

    add_executable(freeusd_c_stage_layer_hints tests/c_stage_layer_hints.c)
    target_include_directories(freeusd_c_stage_layer_hints PRIVATE "${PROJECT_SOURCE_DIR}/include")
    target_link_libraries(freeusd_c_stage_layer_hints PRIVATE freeusd::c)
    set_target_properties(freeusd_c_stage_layer_hints PROPERTIES C_STANDARD 11 C_STANDARD_REQUIRED ON LINKER_LANGUAGE CXX)
    add_test(NAME freeusd_c_stage_layer_hints COMMAND freeusd_c_stage_layer_hints)

    add_executable(freeusd_c_engine_integration tests/c_engine_integration.c)
    target_include_directories(freeusd_c_engine_integration PRIVATE "${PROJECT_SOURCE_DIR}/include")
    target_link_libraries(freeusd_c_engine_integration PRIVATE freeusd::c)
    target_compile_definitions(
      freeusd_c_engine_integration PRIVATE "FREEUSD_TEST_FIXTURES_DIR=\"${CMAKE_SOURCE_DIR}/tests/fixtures\"")
    set_target_properties(freeusd_c_engine_integration PROPERTIES C_STANDARD 11 C_STANDARD_REQUIRED ON LINKER_LANGUAGE CXX)
    add_test(NAME freeusd_c_engine_integration COMMAND freeusd_c_engine_integration)

    add_executable(freeusd_c_usd_skel tests/c_usd_skel.c)
    target_include_directories(freeusd_c_usd_skel PRIVATE "${PROJECT_SOURCE_DIR}/include")
    target_link_libraries(freeusd_c_usd_skel PRIVATE freeusd::c)
    target_compile_definitions(freeusd_c_usd_skel PRIVATE "FREEUSD_TEST_FIXTURES_DIR=\"${CMAKE_SOURCE_DIR}/tests/fixtures\"")
    set_target_properties(freeusd_c_usd_skel PROPERTIES C_STANDARD 11 C_STANDARD_REQUIRED ON LINKER_LANGUAGE CXX)
    add_test(NAME freeusd_c_usd_skel COMMAND freeusd_c_usd_skel)

    add_executable(freeusd_c_usd_shade tests/c_usd_shade.c)
    target_include_directories(freeusd_c_usd_shade PRIVATE "${PROJECT_SOURCE_DIR}/include")
    target_link_libraries(freeusd_c_usd_shade PRIVATE freeusd::c)
    target_compile_definitions(freeusd_c_usd_shade PRIVATE "FREEUSD_TEST_FIXTURES_DIR=\"${CMAKE_SOURCE_DIR}/tests/fixtures\"")
    set_target_properties(freeusd_c_usd_shade PROPERTIES C_STANDARD 11 C_STANDARD_REQUIRED ON LINKER_LANGUAGE CXX)
    add_test(NAME freeusd_c_usd_shade COMMAND freeusd_c_usd_shade)

    add_executable(freeusd_c_usd_lux tests/c_usd_lux.c)
    target_include_directories(freeusd_c_usd_lux PRIVATE "${PROJECT_SOURCE_DIR}/include")
    target_link_libraries(freeusd_c_usd_lux PRIVATE freeusd::c)
    target_compile_definitions(freeusd_c_usd_lux PRIVATE "FREEUSD_TEST_FIXTURES_DIR=\"${CMAKE_SOURCE_DIR}/tests/fixtures\"")
    set_target_properties(freeusd_c_usd_lux PROPERTIES C_STANDARD 11 C_STANDARD_REQUIRED ON LINKER_LANGUAGE CXX)
    add_test(NAME freeusd_c_usd_lux COMMAND freeusd_c_usd_lux)

    add_executable(freeusd_c_usd_vol tests/c_usd_vol.c)
    target_include_directories(freeusd_c_usd_vol PRIVATE "${PROJECT_SOURCE_DIR}/include")
    target_link_libraries(freeusd_c_usd_vol PRIVATE freeusd::c)
    target_compile_definitions(freeusd_c_usd_vol PRIVATE "FREEUSD_TEST_FIXTURES_DIR=\"${CMAKE_SOURCE_DIR}/tests/fixtures\"")
    set_target_properties(freeusd_c_usd_vol PROPERTIES C_STANDARD 11 C_STANDARD_REQUIRED ON LINKER_LANGUAGE CXX)
    add_test(NAME freeusd_c_usd_vol COMMAND freeusd_c_usd_vol)

    add_executable(freeusd_c_usd_physics tests/c_usd_physics.c)
    target_include_directories(freeusd_c_usd_physics PRIVATE "${PROJECT_SOURCE_DIR}/include")
    target_link_libraries(freeusd_c_usd_physics PRIVATE freeusd::c)
    target_compile_definitions(freeusd_c_usd_physics PRIVATE "FREEUSD_TEST_FIXTURES_DIR=\"${CMAKE_SOURCE_DIR}/tests/fixtures\"")
    set_target_properties(freeusd_c_usd_physics PROPERTIES C_STANDARD 11 C_STANDARD_REQUIRED ON LINKER_LANGUAGE CXX)
    add_test(NAME freeusd_c_usd_physics COMMAND freeusd_c_usd_physics)

    add_executable(freeusd_c_usd_physics_rigid_body tests/c_usd_physics_rigid_body.c)
    target_include_directories(freeusd_c_usd_physics_rigid_body PRIVATE "${PROJECT_SOURCE_DIR}/include")
    target_link_libraries(freeusd_c_usd_physics_rigid_body PRIVATE freeusd::c)
    target_compile_definitions(
      freeusd_c_usd_physics_rigid_body PRIVATE "FREEUSD_TEST_FIXTURES_DIR=\"${CMAKE_SOURCE_DIR}/tests/fixtures\"")
    set_target_properties(freeusd_c_usd_physics_rigid_body PROPERTIES C_STANDARD 11 C_STANDARD_REQUIRED ON LINKER_LANGUAGE CXX)
    add_test(NAME freeusd_c_usd_physics_rigid_body COMMAND freeusd_c_usd_physics_rigid_body)

    add_executable(freeusd_c_usd_physics_collision tests/c_usd_physics_collision.c)
    target_include_directories(freeusd_c_usd_physics_collision PRIVATE "${PROJECT_SOURCE_DIR}/include")
    target_link_libraries(freeusd_c_usd_physics_collision PRIVATE freeusd::c)
    target_compile_definitions(
      freeusd_c_usd_physics_collision PRIVATE "FREEUSD_TEST_FIXTURES_DIR=\"${CMAKE_SOURCE_DIR}/tests/fixtures\"")
    set_target_properties(freeusd_c_usd_physics_collision PROPERTIES C_STANDARD 11 C_STANDARD_REQUIRED ON LINKER_LANGUAGE CXX)
    add_test(NAME freeusd_c_usd_physics_collision COMMAND freeusd_c_usd_physics_collision)

    add_executable(freeusd_c_usd_physics_mass tests/c_usd_physics_mass.c)
    target_include_directories(freeusd_c_usd_physics_mass PRIVATE "${PROJECT_SOURCE_DIR}/include")
    target_link_libraries(freeusd_c_usd_physics_mass PRIVATE freeusd::c)
    target_compile_definitions(
      freeusd_c_usd_physics_mass PRIVATE "FREEUSD_TEST_FIXTURES_DIR=\"${CMAKE_SOURCE_DIR}/tests/fixtures\"")
    set_target_properties(freeusd_c_usd_physics_mass PROPERTIES C_STANDARD 11 C_STANDARD_REQUIRED ON LINKER_LANGUAGE CXX)
    add_test(NAME freeusd_c_usd_physics_mass COMMAND freeusd_c_usd_physics_mass)

    add_executable(freeusd_c_usd_physics_fixed_joint tests/c_usd_physics_fixed_joint.c)
    target_include_directories(freeusd_c_usd_physics_fixed_joint PRIVATE "${PROJECT_SOURCE_DIR}/include")
    target_link_libraries(freeusd_c_usd_physics_fixed_joint PRIVATE freeusd::c)
    target_compile_definitions(
      freeusd_c_usd_physics_fixed_joint PRIVATE "FREEUSD_TEST_FIXTURES_DIR=\"${CMAKE_SOURCE_DIR}/tests/fixtures\"")
    set_target_properties(freeusd_c_usd_physics_fixed_joint PROPERTIES C_STANDARD 11 C_STANDARD_REQUIRED ON LINKER_LANGUAGE CXX)
    add_test(NAME freeusd_c_usd_physics_fixed_joint COMMAND freeusd_c_usd_physics_fixed_joint)
  endif()

  set(
    _freeusd_assert_enabled_test_targets
    freeusd_selftest
    freeusd_sdf_path_test
    freeusd_usd_smoke_test
    freeusd_usd_cross_language_test
    freeusd_usda_roundtrip_test
    freeusd_pcp_compose_test
    freeusd_stage_compose_test
    freeusd_stage_open_file_test
    freeusd_parity_fixtures_test
    freeusd_usdc_fields_test
    freeusd_usdc_specs_test
    freeusd_usdc_fieldsets_test
    freeusd_usdc_values_test
    freeusd_usd_geom_mesh_test
    freeusd_usd_geom_boundable_mesh_test
    freeusd_usd_lux_sphere_light_test
    freeusd_usd_lux_rect_light_test
    freeusd_usd_shade_texture_test
    freeusd_usd_shade_pbr_textures_test
    freeusd_usd_lux_disk_light_test
    freeusd_usd_lux_cylinder_light_test
    freeusd_usd_lux_dome_light_test
    freeusd_usd_physics_scene_test
    freeusd_usd_physics_rigid_body_test
    freeusd_usd_physics_collision_test
    freeusd_usd_physics_mass_test
    freeusd_usd_physics_fixed_joint_test
    freeusd_usd_vol_openvdb_test
    freeusd_usd_vol_volume_test
    freeusd_usd_skel_smoke_test
    freeusd_usd_skel_skinning_matrices_test
    freeusd_usd_shade_smoke_test
    freeusd_stage_kind_active_test
    freeusd_usd_lux_distant_light_test
    freeusd_ar_path_security_test
    freeusd_engine_integration_test
    freeusd_schema_tokens_bundle_test
    freeusd_time_code_test
    freeusd_edit_target_test
    freeusd_sdf_layer_offset_test
    freeusd_sdf_asset_path_test
    freeusd_gf_range1d_test
    freeusd_gf_quatd_test
    freeusd_gf_bbox3d_test
    freeusd_gf_matrix4d_test
    freeusd_ar_resolved_path_test
    freeusd_sdf_tokens_test
    freeusd_usd_tokens_test)
  if(FREEUSD_BUILD_C_ABI)
    list(
      APPEND
      _freeusd_assert_enabled_test_targets
      freeusd_c_smoke_test
      freeusd_c_usd_cross_language
      freeusd_c_stage_composition_arcs
      freeusd_c_stage_open_from_file
      freeusd_c_stage_relocate
      freeusd_c_stage_prefix_substitution
      freeusd_c_stage_custom_layer_data
      freeusd_c_stage_variant
      freeusd_c_stage_prim_specifier
      freeusd_c_stage_layer_hints
      freeusd_c_engine_integration
      freeusd_c_usd_skel
      freeusd_c_usd_shade
      freeusd_c_usd_lux
      freeusd_c_usd_vol
      freeusd_c_usd_physics
      freeusd_c_usd_physics_rigid_body
      freeusd_c_usd_physics_collision
      freeusd_c_usd_physics_mass
      freeusd_c_usd_physics_fixed_joint)
  endif()
  foreach(_freeusd_test_target IN LISTS _freeusd_assert_enabled_test_targets)
    freeusd_enable_test_asserts("${_freeusd_test_target}")
  endforeach()
endif()

if(FREEUSD_BUILD_TESTS AND FREEUSD_TEST_INSTALL_INTEGRATION)
  # Use cmake -E env (not PROPERTIES ENVIRONMENT) so paths are not split on ';'.
  add_test(
    NAME freeusd_install_integration
    COMMAND
      "${CMAKE_COMMAND}" -E env "FREEUSD_SOURCE_DIR=${CMAKE_SOURCE_DIR}"
      "FREEUSD_SCRATCH=${CMAKE_SOURCE_DIR}/.install_integration_scratch"
      "FREEUSD_CMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}" "${CMAKE_COMMAND}" -P
      "${CMAKE_SOURCE_DIR}/tests/install_integration.cmake")
  set_tests_properties(
    freeusd_install_integration
    PROPERTIES LABELS "install_integration" TIMEOUT 600 RESOURCE_LOCK freeusd_install_scratch)
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
  target_include_directories(_native PRIVATE "${CMAKE_SOURCE_DIR}/python")
  target_link_libraries(_native PRIVATE freeusd::runtime)
  set_target_properties(_native PROPERTIES OUTPUT_NAME _native)
  # Dev/test: sync built extension next to the in-repo package (gitignored *.so).
  add_custom_command(
    TARGET _native
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:_native>
            ${CMAKE_SOURCE_DIR}/freeusd/$<TARGET_FILE_NAME:_native>
    COMMENT "Sync _native extension beside Python package (gitignored *.so)")
  # Editable installs resolve `freeusd._native` from site-packages; sync when that tree exists (see cmake/sync script).
  add_custom_command(
    TARGET _native
    POST_BUILD
    COMMAND ${Python_EXECUTABLE} ${CMAKE_CURRENT_LIST_DIR}/cmake/sync_native_post_build.py $<TARGET_FILE:_native>
    COMMENT "Sync _native into site-packages/freeusd when present (editable / venv)")

  install(TARGETS _native LIBRARY DESTINATION freeusd)
  install(DIRECTORY "${CMAKE_SOURCE_DIR}/freeusd/" DESTINATION freeusd FILES_MATCHING PATTERN "*.py")
endif()

if(FREEUSD_BUILD_PYTHON AND FREEUSD_BUILD_TESTS)
  # Prefer CMake's Python; fall back to repo .venv when the system interpreter has no pytest (common dev setup).
  set(_FREEUSD_PYTEST_PYTHON "${Python_EXECUTABLE}")
  execute_process(
    COMMAND "${Python_EXECUTABLE}" -c "import pytest"
    RESULT_VARIABLE _freeusd_have_pytest
    OUTPUT_QUIET
    ERROR_QUIET)
  if(NOT _freeusd_have_pytest EQUAL 0)
    set(_FREEUSD_VENV_PY "${CMAKE_SOURCE_DIR}/.venv/bin/python")
    if(EXISTS "${_FREEUSD_VENV_PY}")
      execute_process(
        COMMAND "${_FREEUSD_VENV_PY}" -c "import pytest"
        RESULT_VARIABLE _freeusd_have_pytest
        OUTPUT_QUIET
        ERROR_QUIET)
      if(_freeusd_have_pytest EQUAL 0)
        set(_FREEUSD_PYTEST_PYTHON "${_FREEUSD_VENV_PY}")
      endif()
    endif()
  endif()
  if(_freeusd_have_pytest EQUAL 0)
    # pytest < 9.0.3 is affected by CVE-2025-71176 (tmpdir); match pyproject.toml dev extra.
    execute_process(
      COMMAND "${_FREEUSD_PYTEST_PYTHON}" -m pytest --version
      OUTPUT_VARIABLE _freeusd_pytest_ver_out
      ERROR_VARIABLE _freeusd_pytest_ver_err
      OUTPUT_STRIP_TRAILING_WHITESPACE
      ERROR_STRIP_TRAILING_WHITESPACE)
    set(_freeusd_pytest_ver_line "${_freeusd_pytest_ver_out}${_freeusd_pytest_ver_err}")
    set(_freeusd_pytest_semver "")
    if(_freeusd_pytest_ver_line MATCHES "pytest ([0-9]+\\.[0-9]+\\.[0-9]+)")
      set(_freeusd_pytest_semver "${CMAKE_MATCH_1}")
    endif()
    if(_freeusd_pytest_semver STREQUAL "")
      message(
        WARNING
          "pytest is importable but its version could not be parsed from '${_freeusd_pytest_ver_line}'; "
          "skipping freeusd_pytest (expected pytest>=9.0.3).")
    elseif(_freeusd_pytest_semver VERSION_LESS "9.0.3")
      message(
        WARNING
          "pytest ${_freeusd_pytest_semver} is below 9.0.3 (CVE-2025-71176); skipping freeusd_pytest. "
          "Install pytest>=9.0.3 for this interpreter (e.g. pip install -e \".[dev]\").")
    else()
      add_test(
        NAME freeusd_pytest
        COMMAND "${_FREEUSD_PYTEST_PYTHON}" -m pytest "${CMAKE_SOURCE_DIR}/tests/python" -q
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
      set_tests_properties(freeusd_pytest PROPERTIES ENVIRONMENT "PYTHONPATH=${CMAKE_SOURCE_DIR}" LABELS "python")
    endif()
    unset(_freeusd_pytest_ver_out)
    unset(_freeusd_pytest_ver_err)
    unset(_freeusd_pytest_ver_line)
    unset(_freeusd_pytest_semver)
  endif()
  unset(_FREEUSD_VENV_PY)
  unset(_FREEUSD_PYTEST_PYTHON)
endif()

install(DIRECTORY include/freeusd TYPE INCLUDE)
install(FILES "${CMAKE_CURRENT_LIST_DIR}/LICENSE"
        DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/doc/${PROJECT_NAME}")

# Targets exported as freeusd::<name> for find_package(FreeUSD) (see EXPORT_NAME in src/CMakeLists.txt).
set(
  _freeusd_export_targets
  freeusd_build_settings
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
  freeusd_usdUI
  freeusd_usd_schemas
  freeusd_plug
  freeusd_trace
  freeusd_work
  freeusd_runtime)
if(FREEUSD_BUILD_C_ABI)
  list(APPEND _freeusd_export_targets freeusd_c)
endif()

install(
  TARGETS ${_freeusd_export_targets}
  EXPORT FreeUsdTargets
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(
  EXPORT FreeUsdTargets
  FILE FreeUsdTargets.cmake
  NAMESPACE freeusd::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/FreeUSD)

set(_freeusd_cmake_dir "${CMAKE_INSTALL_LIBDIR}/cmake/FreeUSD")
configure_package_config_file(
  "${CMAKE_CURRENT_LIST_DIR}/cmake/FreeUSDConfig.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/FreeUSDConfig.cmake"
  INSTALL_DESTINATION "${_freeusd_cmake_dir}")

write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/FreeUSDConfigVersion.cmake"
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/FreeUSDConfig.cmake" "${CMAKE_CURRENT_BINARY_DIR}/FreeUSDConfigVersion.cmake"
        DESTINATION "${_freeusd_cmake_dir}")

# pkg-config (static archives; link order matches typical CMake consumer for freeusd::runtime + optional C ABI).
set(_freeusd_pc_arch base usd ar io pcp sdf vt tf plug)
set(FREEUSD_PC_ARCHIVE_LIBS "")
foreach(_s IN LISTS _freeusd_pc_arch)
  string(APPEND FREEUSD_PC_ARCHIVE_LIBS " -lfreeusd_${_s}")
endforeach()
if(FREEUSD_BUILD_C_ABI)
  set(FREEUSD_PC_ARCHIVE_LIBS " -lfreeusd_c${FREEUSD_PC_ARCHIVE_LIBS}")
endif()
# GNU ld accepts --start-group/--end-group for static cycles; Apple ld does not.
if(CMAKE_SYSTEM_NAME STREQUAL "Linux" OR MINGW)
  set(FREEUSD_PC_LINK_WRAP_START "-Wl,--start-group")
  set(FREEUSD_PC_LINK_WRAP_END "-Wl,--end-group")
else()
  set(FREEUSD_PC_LINK_WRAP_START "")
  set(FREEUSD_PC_LINK_WRAP_END "")
endif()
configure_file("${CMAKE_CURRENT_LIST_DIR}/cmake/freeusd.pc.in" "${CMAKE_CURRENT_BINARY_DIR}/freeusd.pc" @ONLY)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/freeusd.pc" DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig")

# Binary archives (run from build dir after configure: cpack [-G TGZ]).
set(CPACK_PACKAGE_NAME "FreeUSD")
set(CPACK_PACKAGE_VENDOR "FreeUSD")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "GPL-3.0-or-later USD-shaped scene libraries (clean-room)")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_LIST_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_LIST_DIR}/README.md")
if(WIN32)
  set(CPACK_GENERATOR "ZIP")
else()
  set(CPACK_GENERATOR "TGZ")
endif()
include(CPack)
