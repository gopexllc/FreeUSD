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
