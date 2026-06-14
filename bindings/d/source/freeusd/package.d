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
        if (outString is null) {
            return "";
        }
        scope(exit) freeusd_string_free(outString);
        return fromStringz(outString).idup;
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
