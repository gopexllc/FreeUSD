module freeusd;

import core.stdc.string : strlen;
import freeusd.c;
import std.conv : to;
import std.exception : enforce;
import std.string : fromStringz, toStringz;

class FreeUsdException : Exception {
    this(string message, string file = __FILE__, size_t line = __LINE__) {
        super(message, file, line);
    }
}

struct PhysicsMassSample {
    float density;
    float[3] centerOfMass;
}

struct PhysicsSceneSample {
    float[3] gravityDirection;
    float gravityMagnitude;
}

struct PhysicsRigidBodySample {
    float mass;
    bool hasKinematicEnabled;
    bool kinematicEnabled;
}

struct PhysicsCollisionSample {
    bool collisionEnabled;
}

struct PhysicsFixedJointSample {
    string body0Path;
    string body1Path;
    bool jointEnabled;
}

struct LuxDistantLightSample {
    float intensity;
    float[3] color;
    float angle;
}

struct OpenVDBAssetInfo {
    string filePath;
    string fieldName;
}

struct PreviewSurfaceDiffuseColor {
    float[3] rgb;
}

struct Matrix4d {
    double[16] rowMajor;
}

struct Bounds3d {
    double[3] min;
    double[3] max;
}

enum PrimSpecifierKind {
    default_,
    def,
    class_,
    over
}

struct Stage {
    private FreeusdStage* handle;

    @disable this();
    @disable this(this);

    private this(FreeusdStage* raw) {
        handle = raw;
    }

    ~this() {
        close();
    }

    static Stage open(string path, int sublayerPolicy = RootSublayersDepthFirst) {
        auto raw = freeusd_stage_open_from_root_file_utf8(path.toStringz, sublayerPolicy);
        if (raw is null) {
            throw new FreeUsdException(lastErrorMessage());
        }
        return Stage(raw);
    }

    void close() {
        if (handle !is null) {
            freeusd_stage_free(handle);
            handle = null;
        }
    }

    bool isOpen() const {
        return handle !is null;
    }

    bool primIsValid(string primPath) const {
        ensureOpen();
        return freeusd_stage_prim_is_valid(handle, primPath.toStringz) == 1;
    }

    bool resolvePrimActive(string primPath) const {
        ensureOpen();
        int active = 0;
        auto rc = freeusd_stage_resolve_prim_active(handle, primPath.toStringz, &active);
        check(rc, "resolvePrimActive");
        return active != 0;
    }

    bool resolveHasPrimActiveOpinion(string primPath) const {
        ensureOpen();
        auto rc = freeusd_stage_resolve_has_prim_active_opinion(handle, primPath.toStringz);
        if (rc < 0) {
            check(-rc, "resolveHasPrimActiveOpinion");
        }
        return rc == 1;
    }

    PrimSpecifierKind resolvePrimSpecifierKind(string primPath) const {
        ensureOpen();
        auto rc = freeusd_stage_resolve_prim_specifier_kind(handle, primPath.toStringz);
        if (rc < 0) {
            check(-rc, "resolvePrimSpecifierKind");
        }
        final switch (rc) {
        case PrimSpecifierDefault:
            return PrimSpecifierKind.default_;
        case PrimSpecifierDef:
            return PrimSpecifierKind.def;
        case PrimSpecifierClass:
            return PrimSpecifierKind.class_;
        case PrimSpecifierOver:
            return PrimSpecifierKind.over;
        }
    }

    string resolvePrimKind(string primPath) const {
        ensureOpen();
        auto kind = freeusd_stage_resolve_prim_kind(handle, primPath.toStringz);
        return takeOwnedCString(kind);
    }

    bool resolveHasPrimKind(string primPath) const {
        ensureOpen();
        auto rc = freeusd_stage_resolve_has_prim_kind(handle, primPath.toStringz);
        if (rc < 0) {
            check(-rc, "resolveHasPrimKind");
        }
        return rc == 1;
    }

    Matrix4d computeLocalTransform(string primPath, double time = 1.0) const {
        ensureOpen();
        double[16] rowMajor;
        auto rc = freeusd_stage_compute_local_transform_matrix4d(handle, primPath.toStringz, time, rowMajor.ptr);
        check(rc, "computeLocalTransform");
        return Matrix4d(rowMajor);
    }

    Matrix4d computeLocalToWorldTransform(string primPath, double time = 1.0) const {
        ensureOpen();
        double[16] rowMajor;
        auto rc = freeusd_stage_compute_local_to_world_transform_matrix4d(handle, primPath.toStringz, time, rowMajor.ptr);
        check(rc, "computeLocalToWorldTransform");
        return Matrix4d(rowMajor);
    }

    bool computeImageableVisibility(string primPath, double time = 1.0) const {
        ensureOpen();
        int visible = 0;
        auto rc = freeusd_stage_compute_imageable_visibility(handle, primPath.toStringz, time, &visible);
        check(rc, "computeImageableVisibility");
        return visible != 0;
    }

    string computeImageablePurpose(string primPath, double time = 1.0) const {
        ensureOpen();
        char* purpose = null;
        auto rc = freeusd_stage_compute_imageable_purpose_utf8(handle, primPath.toStringz, time, &purpose);
        check(rc, "computeImageablePurpose");
        return takeOwnedCString(purpose);
    }

    Bounds3d computeBoundableLocalBounds(string primPath, double time = 1.0) const {
        ensureOpen();
        double minX, minY, minZ, maxX, maxY, maxZ;
        auto rc = freeusd_stage_compute_boundable_local_bounds(
            handle, primPath.toStringz, time, &minX, &minY, &minZ, &maxX, &maxY, &maxZ);
        check(rc, "computeBoundableLocalBounds");
        return Bounds3d([minX, minY, minZ], [maxX, maxY, maxZ]);
    }

    Bounds3d computeBoundableWorldBounds(string primPath, double time = 1.0) const {
        ensureOpen();
        double minX, minY, minZ, maxX, maxY, maxZ;
        auto rc = freeusd_stage_compute_boundable_world_bounds(
            handle, primPath.toStringz, time, &minX, &minY, &minZ, &maxX, &maxY, &maxZ);
        check(rc, "computeBoundableWorldBounds");
        return Bounds3d([minX, minY, minZ], [maxX, maxY, maxZ]);
    }

    double readFieldDouble(string primPath, string attrName, double time = 1.0) const {
        ensureOpen();
        double outValue = 0.0;
        auto rc = freeusd_stage_read_field_double(handle, primPath.toStringz, attrName.toStringz, time, &outValue);
        check(rc, "readFieldDouble");
        return outValue;
    }

    string readFieldString(string primPath, string attrName, double time = 1.0) const {
        ensureOpen();
        char* outString = null;
        auto rc = freeusd_stage_read_field_string(handle, primPath.toStringz, attrName.toStringz, time, &outString);
        check(rc, "readFieldString");
        return takeOwnedCString(outString);
    }

    string readMaterialSurfaceShaderPath(string materialPath) const {
        ensureOpen();
        char* shaderPath = null;
        auto rc = freeusd_stage_read_material_surface_shader_path(handle, materialPath.toStringz, &shaderPath);
        check(rc, "readMaterialSurfaceShaderPath");
        return takeOwnedCString(shaderPath);
    }

    PreviewSurfaceDiffuseColor readPreviewSurfaceDiffuseColor(string shaderPath, double time = 1.0) const {
        ensureOpen();
        float[3] rgb;
        auto rc = freeusd_stage_read_preview_surface_diffuse_color(handle, shaderPath.toStringz, time, rgb.ptr);
        check(rc, "readPreviewSurfaceDiffuseColor");
        return PreviewSurfaceDiffuseColor(rgb);
    }

    PreviewSurfaceDiffuseColor readPreviewSurfaceEmissiveColor(string shaderPath, double time = 1.0) const {
        ensureOpen();
        float[3] rgb;
        auto rc = freeusd_stage_read_preview_surface_emissive_color(handle, shaderPath.toStringz, time, rgb.ptr);
        check(rc, "readPreviewSurfaceEmissiveColor");
        return PreviewSurfaceDiffuseColor(rgb);
    }

    float readPreviewSurfaceMetallic(string shaderPath, double time = 1.0) const {
        ensureOpen();
        float value = 0.0f;
        auto rc = freeusd_stage_read_preview_surface_metallic(handle, shaderPath.toStringz, time, &value);
        check(rc, "readPreviewSurfaceMetallic");
        return value;
    }

    float readPreviewSurfaceRoughness(string shaderPath, double time = 1.0) const {
        ensureOpen();
        float value = 0.0f;
        auto rc = freeusd_stage_read_preview_surface_roughness(handle, shaderPath.toStringz, time, &value);
        check(rc, "readPreviewSurfaceRoughness");
        return value;
    }

    float readPreviewSurfaceOpacity(string shaderPath, double time = 1.0) const {
        ensureOpen();
        float value = 0.0f;
        auto rc = freeusd_stage_read_preview_surface_opacity(handle, shaderPath.toStringz, time, &value);
        check(rc, "readPreviewSurfaceOpacity");
        return value;
    }

    string readPreviewSurfaceDiffuseTextureAssetPath(string shaderPath, double time = 1.0) const {
        ensureOpen();
        char* assetPath = null;
        auto rc = freeusd_stage_read_preview_surface_diffuse_texture_asset_path(handle, shaderPath.toStringz, time, &assetPath);
        check(rc, "readPreviewSurfaceDiffuseTextureAssetPath");
        return takeOwnedCString(assetPath);
    }

    string readPreviewSurfaceNormalTextureAssetPath(string shaderPath, double time = 1.0) const {
        ensureOpen();
        char* assetPath = null;
        auto rc = freeusd_stage_read_preview_surface_normal_texture_asset_path(handle, shaderPath.toStringz, time, &assetPath);
        check(rc, "readPreviewSurfaceNormalTextureAssetPath");
        return takeOwnedCString(assetPath);
    }

    string readPreviewSurfaceOcclusionTextureAssetPath(string shaderPath, double time = 1.0) const {
        ensureOpen();
        char* assetPath = null;
        auto rc = freeusd_stage_read_preview_surface_occlusion_texture_asset_path(handle, shaderPath.toStringz, time, &assetPath);
        check(rc, "readPreviewSurfaceOcclusionTextureAssetPath");
        return takeOwnedCString(assetPath);
    }

    string readPreviewSurfaceMetallicTextureAssetPath(string shaderPath, double time = 1.0) const {
        ensureOpen();
        char* assetPath = null;
        auto rc = freeusd_stage_read_preview_surface_metallic_texture_asset_path(handle, shaderPath.toStringz, time, &assetPath);
        check(rc, "readPreviewSurfaceMetallicTextureAssetPath");
        return takeOwnedCString(assetPath);
    }

    string readPreviewSurfaceRoughnessTextureAssetPath(string shaderPath, double time = 1.0) const {
        ensureOpen();
        char* assetPath = null;
        auto rc = freeusd_stage_read_preview_surface_roughness_texture_asset_path(handle, shaderPath.toStringz, time, &assetPath);
        check(rc, "readPreviewSurfaceRoughnessTextureAssetPath");
        return takeOwnedCString(assetPath);
    }

    PhysicsMassSample readPhysicsMass(string primPath, double time = 1.0) const {
        ensureOpen();
        FreeusdPhysicsMassSample raw;
        auto rc = freeusd_stage_read_physics_mass_sample(handle, primPath.toStringz, time, &raw);
        check(rc, "readPhysicsMass");
        return PhysicsMassSample(raw.density, raw.center_of_mass);
    }

    PhysicsSceneSample readPhysicsScene(string scenePath, double time = 1.0) const {
        ensureOpen();
        FreeusdPhysicsSceneSample raw;
        auto rc = freeusd_stage_read_physics_scene_sample(handle, scenePath.toStringz, time, &raw);
        check(rc, "readPhysicsScene");
        return PhysicsSceneSample(raw.gravity_direction, raw.gravity_magnitude);
    }

    PhysicsRigidBodySample readPhysicsRigidBody(string primPath, double time = 1.0) const {
        ensureOpen();
        FreeusdPhysicsRigidBodySample raw;
        auto rc = freeusd_stage_read_physics_rigid_body_sample(handle, primPath.toStringz, time, &raw);
        check(rc, "readPhysicsRigidBody");
        return PhysicsRigidBodySample(raw.mass, raw.has_kinematic_enabled != 0, raw.kinematic_enabled != 0);
    }

    PhysicsCollisionSample readPhysicsCollision(string primPath, double time = 1.0) const {
        ensureOpen();
        FreeusdPhysicsCollisionSample raw;
        auto rc = freeusd_stage_read_physics_collision_sample(handle, primPath.toStringz, time, &raw);
        check(rc, "readPhysicsCollision");
        return PhysicsCollisionSample(raw.collision_enabled != 0);
    }

    PhysicsFixedJointSample readPhysicsFixedJoint(string jointPath, double time = 1.0) const {
        ensureOpen();
        FreeusdPhysicsFixedJointSample raw;
        auto rc = freeusd_stage_read_physics_fixed_joint_sample(handle, jointPath.toStringz, time, &raw);
        check(rc, "readPhysicsFixedJoint");
        scope(exit) {
            if (raw.body0_path_utf8 !is null) {
                freeusd_string_free(raw.body0_path_utf8);
            }
            if (raw.body1_path_utf8 !is null) {
                freeusd_string_free(raw.body1_path_utf8);
            }
        }
        return PhysicsFixedJointSample(
            raw.body0_path_utf8 is null ? "" : fromStringz(raw.body0_path_utf8).idup,
            raw.body1_path_utf8 is null ? "" : fromStringz(raw.body1_path_utf8).idup,
            raw.joint_enabled != 0);
    }

    LuxDistantLightSample readLuxDistantLight(string lightPath, double time = 1.0) const {
        ensureOpen();
        FreeusdLuxDistantLightSample raw;
        auto rc = freeusd_stage_read_lux_distant_light_sample(handle, lightPath.toStringz, time, &raw);
        check(rc, "readLuxDistantLight");
        return LuxDistantLightSample(raw.intensity, raw.color, raw.angle);
    }

    OpenVDBAssetInfo readOpenVDBAsset(string assetPath, double time = 1.0) const {
        ensureOpen();
        char* filePath = null;
        char* fieldName = null;
        auto rc = freeusd_stage_read_openvdb_asset_info(handle, assetPath.toStringz, time, &filePath, &fieldName);
        check(rc, "readOpenVDBAsset");
        scope(exit) {
            if (filePath !is null) {
                freeusd_string_free(filePath);
            }
            if (fieldName !is null) {
                freeusd_string_free(fieldName);
            }
        }
        return OpenVDBAssetInfo(
            filePath is null ? "" : fromStringz(filePath).idup,
            fieldName is null ? "" : fromStringz(fieldName).idup);
    }

    private void ensureOpen() const {
        enforce(handle !is null, new FreeUsdException("FreeUSD stage is closed"));
    }
}

string versionString() {
    return borrowedCString(freeusd_version_string());
}

string usdcCrateIdentifier() {
    return borrowedCString(freeusd_usdc_crate_identifier_utf8());
}

string lastErrorMessage() {
    return borrowedCString(freeusd_last_error_message());
}

private void check(int rc, string operation) {
    if (rc != FREEUSD_OK) {
        throw new FreeUsdException(operation ~ " failed (rc=" ~ rc.to!string ~ "): " ~ lastErrorMessage());
    }
}

private string borrowedCString(const(char)* value) {
    if (value is null) {
        return "";
    }
    return fromStringz(value).idup;
}

private string takeOwnedCString(char* value) {
    if (value is null) {
        return "";
    }
    scope(exit) freeusd_string_free(value);
    return fromStringz(value).idup;
}

unittest {
    assert(versionString().length > 0);
    assert(usdcCrateIdentifier() == "PXR-USDC");
}

unittest {
    auto stage = Stage.open("../../tests/fixtures/usd_cross_language.usda");
    assert(stage.isOpen);
    assert(stage.primIsValid("/Scene/Child"));
    assert(!stage.primIsValid("/Scene/Missing"));
    assert(stage.readFieldDouble("/Scene/Child", "mass", 1.0) == 2.5);
    assert(stage.readFieldString("/Scene/Child", "label", 1.0) == "hello");
}

unittest {
    auto stage = Stage.open("../../tests/fixtures/parity_kind_active_refs.usda");
    assert(stage.resolveHasPrimKind("/World/RefHost"));
    assert(stage.resolvePrimKind("/World/RefHost") == "component");
    assert(stage.resolveHasPrimActiveOpinion("/World/RefHost"));
    assert(!stage.resolvePrimActive("/World/RefHost"));
    assert(stage.resolvePrimKind("/World/PayloadHost") == "group");
    assert(stage.resolvePrimKind("/World/InheritHost") == "assembly");
    assert(stage.resolvePrimActive("/World/InheritHost"));
    assert(!stage.resolveHasPrimActiveOpinion("/World/InheritHost"));
}

unittest {
    auto stage = Stage.open("../../tests/fixtures/parity_imageable.usda");
    auto l2w = stage.computeLocalToWorldTransform("/World/Cube", 1.0);
    assert(l2w.rowMajor[12] == 1.0);
    assert(l2w.rowMajor[13] == 2.0);
    assert(l2w.rowMajor[14] == 3.0);
    assert(l2w.rowMajor[15] == 1.0);
    assert(!stage.computeImageableVisibility("/World/Cube", 1.0));
    assert(stage.computeImageablePurpose("/World/Cube", 1.0) == "render");
    auto bounds = stage.computeBoundableWorldBounds("/World/Cube", 1.0);
    assert(bounds.min == [0.0, 1.0, 2.0]);
    assert(bounds.max == [2.0, 3.0, 4.0]);
}

unittest {
    auto stage = Stage.open("../../tests/fixtures/parity_physics_mass.usda");
    auto mass = stage.readPhysicsMass("/World/Prop", 1.0);
    assert(mass.density == 2.0f);
    assert(mass.centerOfMass[0] == 0.0f);
    assert(mass.centerOfMass[1] == 0.5f);
    assert(mass.centerOfMass[2] == 0.0f);
}

unittest {
    auto stage = Stage.open("../../tests/fixtures/parity_physics_scene.usda");
    auto scene = stage.readPhysicsScene("/World/Physics", 1.0);
    assert(scene.gravityDirection[0] == 0.0f);
    assert(scene.gravityDirection[1] == 0.0f);
    assert(scene.gravityDirection[2] == -1.0f);
    assert(scene.gravityMagnitude == 981.0f);
}

unittest {
    auto stage = Stage.open("../../tests/fixtures/parity_physics_rigid_body_kinematic.usda");
    auto body = stage.readPhysicsRigidBody("/World/Body", 1.0);
    assert(body.mass == 1.0f);
    assert(body.hasKinematicEnabled);
    assert(body.kinematicEnabled);
}

unittest {
    auto stage = Stage.open("../../tests/fixtures/parity_physics_collision.usda");
    auto collision = stage.readPhysicsCollision("/World/Collider", 1.0);
    assert(!collision.collisionEnabled);
}

unittest {
    auto stage = Stage.open("../../tests/fixtures/parity_physics_fixed_joint.usda");
    auto joint = stage.readPhysicsFixedJoint("/World/Anchor", 1.0);
    assert(joint.body0Path == "/World/BodyA");
    assert(joint.body1Path == "/World/BodyB");
    assert(joint.jointEnabled);
}

unittest {
    auto stage = Stage.open("../../tests/fixtures/parity_lux_distant.usda");
    auto light = stage.readLuxDistantLight("/World/Sun", 1.0);
    assert(light.intensity == 1200.0f);
    assert(light.color[0] == 1.0f);
    assert(light.color[1] == 0.95f);
    assert(light.color[2] == 0.8f);
    assert(light.angle == 0.53f);
}

unittest {
    auto stage = Stage.open("../../tests/fixtures/parity_vol_openvdb.usda");
    auto asset = stage.readOpenVDBAsset("/World/Smoke", 1.0);
    assert(asset.filePath == "volumes/smoke.vdb");
    assert(asset.fieldName == "density");
}

unittest {
    auto stage = Stage.open("../../tests/fixtures/parity_shade_preview.usda");
    auto shaderPath = stage.readMaterialSurfaceShaderPath("/World/Looks/Material");
    assert(shaderPath == "/World/Looks/Material/PreviewSurface");
    auto diffuse = stage.readPreviewSurfaceDiffuseColor(shaderPath, 1.0);
    assert(diffuse.rgb[0] == 0.8f);
    assert(diffuse.rgb[1] == 0.2f);
    assert(diffuse.rgb[2] == 0.1f);
    assert(stage.readPreviewSurfaceMetallic(shaderPath, 1.0) == 0.5f);
    assert(stage.readPreviewSurfaceRoughness(shaderPath, 1.0) == 0.3f);
    assert(stage.readPreviewSurfaceOpacity(shaderPath, 1.0) == 1.0f);
}

unittest {
    auto stage = Stage.open("../../tests/fixtures/parity_shade_texture.usda");
    auto shaderPath = stage.readMaterialSurfaceShaderPath("/World/Looks/Material");
    auto texturePath = stage.readPreviewSurfaceDiffuseTextureAssetPath(shaderPath, 1.0);
    assert(texturePath == "textures/albedo.png");
}

unittest {
    auto stage = Stage.open("../../tests/fixtures/parity_shade_pbr_textures.usda");
    auto shaderPath = "/World/Looks/Material/PreviewSurface";
    auto emissive = stage.readPreviewSurfaceEmissiveColor(shaderPath, 1.0);
    assert(emissive.rgb[0] == 0.1f);
    assert(stage.readPreviewSurfaceNormalTextureAssetPath(shaderPath, 1.0) == "textures/normal.png");
    assert(stage.readPreviewSurfaceOcclusionTextureAssetPath(shaderPath, 1.0) == "textures/ao.png");
    assert(stage.readPreviewSurfaceMetallicTextureAssetPath(shaderPath, 1.0) == "textures/metallic.png");
    assert(stage.readPreviewSurfaceRoughnessTextureAssetPath(shaderPath, 1.0) == "textures/roughness.png");
}
