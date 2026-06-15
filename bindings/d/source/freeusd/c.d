module freeusd.c;

import core.stdc.config : c_ulong;

extern(C):

enum FREEUSD_OK = 0;
enum FREEUSD_ERR_INVALID_ARGUMENT = 1;
enum FREEUSD_ERR_PARSE = 2;
enum FREEUSD_ERR_NOT_FOUND = 3;
enum FREEUSD_ERR_INTERNAL = 4;

enum RootSublayersNone = 0;
enum RootSublayersShallow = 1;
enum RootSublayersDepthFirst = 2;

enum PrimSpecifierDefault = 0;
enum PrimSpecifierDef = 1;
enum PrimSpecifierClass = 2;
enum PrimSpecifierOver = 3;

struct FreeusdStage;

struct FreeusdPhysicsMassSample {
    float density;
    float[3] center_of_mass;
}

struct FreeusdPhysicsSceneSample {
    float[3] gravity_direction;
    float gravity_magnitude;
}

struct FreeusdPhysicsRigidBodySample {
    float mass;
    int has_kinematic_enabled;
    int kinematic_enabled;
}

struct FreeusdPhysicsCollisionSample {
    int collision_enabled;
}

struct FreeusdLuxDistantLightSample {
    float intensity;
    float[3] color;
    float angle;
}

const(char)* freeusd_version_string();
const(char)* freeusd_usdc_crate_identifier_utf8();
const(char)* freeusd_last_error_message();

void freeusd_string_free(char* s);

FreeusdStage* freeusd_stage_open_from_root_file_utf8(const(char)* layer_path_utf8, int sublayer_policy);
void freeusd_stage_free(FreeusdStage* stage);
int freeusd_stage_prim_is_valid(const(FreeusdStage)* stage, const(char)* prim_path_utf8);

int freeusd_stage_resolve_prim_active(
    const(FreeusdStage)* stage,
    const(char)* prim_path_utf8,
    int* out_active);

int freeusd_stage_resolve_has_prim_active_opinion(
    const(FreeusdStage)* stage,
    const(char)* prim_path_utf8);

int freeusd_stage_resolve_prim_specifier_kind(
    const(FreeusdStage)* stage,
    const(char)* prim_path_utf8);

char* freeusd_stage_resolve_prim_kind(
    const(FreeusdStage)* stage,
    const(char)* prim_path_utf8);

int freeusd_stage_resolve_has_prim_kind(
    const(FreeusdStage)* stage,
    const(char)* prim_path_utf8);

int freeusd_stage_compute_local_transform_matrix4d(
    const(FreeusdStage)* stage,
    const(char)* prim_path_utf8,
    double time,
    double* out_row_major);

int freeusd_stage_compute_local_to_world_transform_matrix4d(
    const(FreeusdStage)* stage,
    const(char)* prim_path_utf8,
    double time,
    double* out_row_major);

int freeusd_stage_compute_imageable_visibility(
    const(FreeusdStage)* stage,
    const(char)* prim_path_utf8,
    double time,
    int* out_visible);

int freeusd_stage_compute_imageable_purpose_utf8(
    const(FreeusdStage)* stage,
    const(char)* prim_path_utf8,
    double time,
    char** out_purpose_utf8);

int freeusd_stage_compute_boundable_local_bounds(
    const(FreeusdStage)* stage,
    const(char)* prim_path_utf8,
    double time,
    double* out_min_x,
    double* out_min_y,
    double* out_min_z,
    double* out_max_x,
    double* out_max_y,
    double* out_max_z);

int freeusd_stage_compute_boundable_world_bounds(
    const(FreeusdStage)* stage,
    const(char)* prim_path_utf8,
    double time,
    double* out_min_x,
    double* out_min_y,
    double* out_min_z,
    double* out_max_x,
    double* out_max_y,
    double* out_max_z);

int freeusd_stage_read_field_double(
    const(FreeusdStage)* stage,
    const(char)* prim_path_utf8,
    const(char)* attr_name_utf8,
    double time,
    double* out_value);

int freeusd_stage_read_field_string(
    const(FreeusdStage)* stage,
    const(char)* prim_path_utf8,
    const(char)* attr_name_utf8,
    double time,
    char** out_string);

int freeusd_stage_read_material_surface_shader_path(
    const(FreeusdStage)* stage,
    const(char)* material_path_utf8,
    char** out_shader_path_utf8);

int freeusd_stage_read_preview_surface_diffuse_color(
    const(FreeusdStage)* stage,
    const(char)* shader_path_utf8,
    double time,
    float* out_rgb);

int freeusd_stage_read_preview_surface_diffuse_texture_asset_path(
    const(FreeusdStage)* stage,
    const(char)* shader_path_utf8,
    double time,
    char** out_path_utf8);

int freeusd_stage_read_physics_mass_sample(
    const(FreeusdStage)* stage,
    const(char)* prim_path_utf8,
    double time,
    FreeusdPhysicsMassSample* out_sample);

int freeusd_stage_read_physics_scene_sample(
    const(FreeusdStage)* stage,
    const(char)* scene_path_utf8,
    double time,
    FreeusdPhysicsSceneSample* out_sample);

int freeusd_stage_read_physics_rigid_body_sample(
    const(FreeusdStage)* stage,
    const(char)* prim_path_utf8,
    double time,
    FreeusdPhysicsRigidBodySample* out_sample);

int freeusd_stage_read_physics_collision_sample(
    const(FreeusdStage)* stage,
    const(char)* prim_path_utf8,
    double time,
    FreeusdPhysicsCollisionSample* out_sample);

int freeusd_stage_read_lux_distant_light_sample(
    const(FreeusdStage)* stage,
    const(char)* light_path_utf8,
    double time,
    FreeusdLuxDistantLightSample* out_sample);

int freeusd_stage_read_openvdb_asset_info(
    const(FreeusdStage)* stage,
    const(char)* asset_path_utf8,
    double time,
    char** out_file_path_utf8,
    char** out_field_name_utf8);
