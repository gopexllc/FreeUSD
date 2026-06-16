module freeusd.nativeusd;

import std.algorithm : canFind, endsWith, startsWith;
import std.array : split;
import std.conv : to;
import std.file : readText;
import std.path : buildNormalizedPath, dirName;
import std.string : indexOf, lastIndexOf, lineSplitter, strip, stripLeft, stripRight;

struct Vec3d {
    double x;
    double y;
    double z;
}

alias Vec3f = Vec3d;

struct Matrix4d {
    double[16] rowMajor;

    static Matrix4d identity() {
        Matrix4d matrix;
        matrix.rowMajor[0] = 1.0;
        matrix.rowMajor[5] = 1.0;
        matrix.rowMajor[10] = 1.0;
        matrix.rowMajor[15] = 1.0;
        return matrix;
    }
}

struct Bounds3d {
    Vec3d min;
    Vec3d max;
}

struct Value {
    enum Kind {
        none,
        double_,
        float_,
        bool_,
        int_,
        string_,
        asset,
        token,
        tokenArray,
        vec3d,
        vec3f
    }

    Kind kind;
    double doubleValue;
    float floatValue;
    bool boolValue;
    long intValue;
    string stringValue;
    string[] tokens;
    Vec3d vec3Value;

    static Value makeDouble(double value) {
        Value outValue;
        outValue.kind = Kind.double_;
        outValue.doubleValue = value;
        return outValue;
    }

    static Value makeFloat(float value) {
        Value outValue;
        outValue.kind = Kind.float_;
        outValue.floatValue = value;
        return outValue;
    }

    static Value makeBool(bool value) {
        Value outValue;
        outValue.kind = Kind.bool_;
        outValue.boolValue = value;
        return outValue;
    }

    static Value makeInt(long value) {
        Value outValue;
        outValue.kind = Kind.int_;
        outValue.intValue = value;
        return outValue;
    }

    static Value makeString(string value) {
        Value outValue;
        outValue.kind = Kind.string_;
        outValue.stringValue = value;
        return outValue;
    }

    static Value makeAsset(string value) {
        Value outValue;
        outValue.kind = Kind.asset;
        outValue.stringValue = value;
        return outValue;
    }

    static Value makeToken(string value) {
        Value outValue;
        outValue.kind = Kind.token;
        outValue.stringValue = value;
        return outValue;
    }

    static Value makeTokenArray(string[] values) {
        Value outValue;
        outValue.kind = Kind.tokenArray;
        outValue.tokens = values;
        return outValue;
    }

    static Value makeVec3(Vec3d value, bool asFloat) {
        Value outValue;
        outValue.kind = asFloat ? Kind.vec3f : Kind.vec3d;
        outValue.vec3Value = value;
        return outValue;
    }

    double asDouble() const {
        final switch (kind) {
        case Kind.double_:
            return doubleValue;
        case Kind.float_:
            return floatValue;
        case Kind.int_:
            return cast(double) intValue;
        case Kind.bool_:
            return boolValue ? 1.0 : 0.0;
        case Kind.none, Kind.string_, Kind.asset, Kind.token, Kind.tokenArray, Kind.vec3d, Kind.vec3f:
            throw new Exception("value is not numeric");
        }
    }
}

struct Prim {
    enum Specifier {
        def_,
        class_,
        over
    }

    string path;
    string typeName;
    Specifier specifier;
    string kind;
    bool hasKind;
    bool active = true;
    bool hasActiveOpinion;
    Value[string] attributes;
    Value[string] customData;
    Value[double][string] timeSamples;
    string[][string] relationships;
    string[] inherits;
    string[] specializes;
    Reference[] references;
    Reference[] payloads;
    string[string] variantSelections;
    VariantPayload[string] variants;

    bool hasAttribute(string name) const {
        return (name in attributes) !is null;
    }

    Value attribute(string name) const {
        auto found = name in attributes;
        if (found is null) {
            throw new Exception("missing attribute: " ~ name);
        }
        return cast(Value) *found;
    }
}

struct PrimMetadata {
    string kind;
    bool hasKind;
    bool active = true;
    bool hasActiveOpinion;
}

struct Reference {
    string assetPath;
    string primPath;
}

struct VariantPayload {
    string[] lines;
}

struct Sublayer {
    string assetPath;
    double offset;
    double scale = 1.0;
    Stage stage;

    double mapTime(double composedTime) const {
        return scale == 0.0 ? composedTime : (composedTime - offset) / scale;
    }
}

struct Stage {
    string defaultPrim;
    string documentation;
    double metersPerUnit;
    bool hasMetersPerUnit;
    string upAxis;
    double startTimeCode;
    bool hasStartTimeCode;
    double endTimeCode;
    bool hasEndTimeCode;
    string sourceDir;
    Prim[string] prims;
    Sublayer[] sublayers;
    string[string] relocates;
    string[string] prefixSubstitutions;

    static Stage open(string path) {
        return parseUsda(readText(path), dirName(path));
    }

    bool primIsValid(string path) const {
        auto authored = mapComposedToAuthored(path);
        if ((authored in prims) !is null) {
            return true;
        }
        foreach (sublayer; sublayers) {
            if (sublayer.stage.primIsValid(path)) {
                return true;
            }
        }
        return false;
    }

    const(Prim) primAt(string path) const {
        auto authored = mapComposedToAuthored(path);
        auto found = authored in prims;
        if (found !is null) {
            return cast(Prim) *found;
        }
        foreach (sublayer; sublayers) {
            if (sublayer.stage.primIsValid(path)) {
                return sublayer.stage.primAt(path);
            }
        }
        throw new Exception("missing prim: " ~ path);
    }

    double readDouble(string primPath, string attrName, double time = 1.0) const {
        return composedAttribute(primPath, attrName, time).asDouble();
    }

    string readString(string primPath, string attrName) const {
        auto value = composedAttribute(primPath, attrName, 1.0);
        if (value.kind != Value.Kind.string_ && value.kind != Value.Kind.asset && value.kind != Value.Kind.token) {
            throw new Exception("attribute is not string-like: " ~ attrName);
        }
        return value.stringValue;
    }

    string[] readTokenArray(string primPath, string attrName) const {
        auto value = composedAttribute(primPath, attrName, 1.0);
        if (value.kind != Value.Kind.tokenArray) {
            throw new Exception("attribute is not token[]: " ~ attrName);
        }
        return value.tokens;
    }

    Vec3d readVec3(string primPath, string attrName) const {
        auto value = composedAttribute(primPath, attrName, 1.0);
        if (value.kind != Value.Kind.vec3d && value.kind != Value.Kind.vec3f) {
            throw new Exception("attribute is not vec3: " ~ attrName);
        }
        return value.vec3Value;
    }

    string[] relationshipTargets(string primPath, string relName) const {
        return composedRelationshipTargets(primPath, relName, []);
    }

    Matrix4d computeLocalTransform(string primPath) const {
        auto translation = localTranslation(primPath);
        auto matrix = Matrix4d.identity();
        matrix.rowMajor[12] = translation.x;
        matrix.rowMajor[13] = translation.y;
        matrix.rowMajor[14] = translation.z;
        return matrix;
    }

    Matrix4d computeLocalToWorldTransform(string primPath) const {
        auto translation = Vec3d(0.0, 0.0, 0.0);
        foreach (path; ancestorPaths(primPath)) {
            auto local = localTranslation(path);
            translation.x += local.x;
            translation.y += local.y;
            translation.z += local.z;
        }
        auto matrix = Matrix4d.identity();
        matrix.rowMajor[12] = translation.x;
        matrix.rowMajor[13] = translation.y;
        matrix.rowMajor[14] = translation.z;
        return matrix;
    }

    bool computeImageableVisibility(string primPath) const {
        foreach_reverse (path; ancestorPaths(primPath)) {
            try {
                if (readString(path, "visibility") == "invisible") {
                    return false;
                }
            } catch (Exception) {
            }
        }
        return true;
    }

    string computeImageablePurpose(string primPath) const {
        foreach_reverse (path; ancestorPaths(primPath)) {
            try {
                return readString(path, "purpose");
            } catch (Exception) {
            }
        }
        return "default";
    }

    Bounds3d computeBoundableLocalBounds(string primPath) const {
        auto size = readDouble(primPath, "size");
        auto half = size / 2.0;
        return Bounds3d(Vec3d(-half, -half, -half), Vec3d(half, half, half));
    }

    Bounds3d computeBoundableWorldBounds(string primPath) const {
        auto local = computeBoundableLocalBounds(primPath);
        auto matrix = computeLocalToWorldTransform(primPath);
        auto tx = matrix.rowMajor[12];
        auto ty = matrix.rowMajor[13];
        auto tz = matrix.rowMajor[14];
        return Bounds3d(
            Vec3d(local.min.x + tx, local.min.y + ty, local.min.z + tz),
            Vec3d(local.max.x + tx, local.max.y + ty, local.max.z + tz));
    }

    Value customDataValue(string primPath, string key) const {
        return composedCustomData(primPath, key, []);
    }

    string customDataString(string primPath, string key) const {
        auto value = customDataValue(primPath, key);
        if (value.kind != Value.Kind.string_ && value.kind != Value.Kind.token) {
            throw new Exception("customData value is not string-like: " ~ key);
        }
        return value.stringValue;
    }

    long customDataInt(string primPath, string key) const {
        auto value = customDataValue(primPath, key);
        if (value.kind == Value.Kind.int_) {
            return value.intValue;
        }
        if (value.kind == Value.Kind.bool_) {
            return value.boolValue ? 1 : 0;
        }
        throw new Exception("customData value is not int-like: " ~ key);
    }

    string[] listPrimInherits(string primPath) const {
        return (cast(string[]) primAt(primPath).inherits).dup;
    }

    string[] listPrimSpecializes(string primPath) const {
        return (cast(string[]) primAt(primPath).specializes).dup;
    }

    Prim.Specifier resolvePrimSpecifier(string primPath) const {
        return primAt(primPath).specifier;
    }

    bool resolveHasPrimKind(string primPath) const {
        return composedPrimMetadata(primPath, []).hasKind;
    }

    string resolvePrimKind(string primPath) const {
        auto metadata = composedPrimMetadata(primPath, []);
        if (!metadata.hasKind) {
            throw new Exception("missing prim kind: " ~ primPath);
        }
        return metadata.kind;
    }

    bool resolveHasPrimActiveOpinion(string primPath) const {
        return composedPrimMetadata(primPath, []).hasActiveOpinion;
    }

    bool resolvePrimActive(string primPath) const {
        return composedPrimMetadata(primPath, []).active;
    }

    bool prefixSubstitutionKeyInAnyLayer(string fromPrefix) const {
        if ((fromPrefix in prefixSubstitutions) !is null) {
            return true;
        }
        foreach (sublayer; sublayers) {
            if (sublayer.stage.prefixSubstitutionKeyInAnyLayer(fromPrefix)) {
                return true;
            }
        }
        return false;
    }

    string composedPrefixSubstitution(string fromPrefix) const {
        if (auto local = fromPrefix in prefixSubstitutions) {
            return *local;
        }
        foreach (sublayer; sublayers) {
            if (sublayer.stage.prefixSubstitutionKeyInAnyLayer(fromPrefix)) {
                return sublayer.stage.composedPrefixSubstitution(fromPrefix);
            }
        }
        throw new Exception("missing prefix substitution: " ~ fromPrefix);
    }

    private Value composedAttribute(string primPath, string attrName) const {
        return composedAttribute(primPath, attrName, 1.0, []);
    }

    private Vec3d localTranslation(string primPath) const {
        try {
            return readVec3(primPath, "xformOp:translate");
        } catch (Exception) {
            return Vec3d(0.0, 0.0, 0.0);
        }
    }

    private Value composedAttribute(string primPath, string attrName, double time) const {
        return composedAttribute(primPath, attrName, time, []);
    }

    private Value composedAttribute(string primPath, string attrName, double time, string[] visiting) const {
        primPath = mapComposedToAuthored(primPath);
        foreach (visited; visiting) {
            if (visited == primPath) {
                throw new Exception("inherit cycle while reading: " ~ primPath);
            }
        }
        auto found = primPath in prims;
        if (found is null) {
            foreach (sublayer; sublayers) {
                try {
                    return sublayer.stage.composedAttribute(primPath, attrName, sublayer.mapTime(time), visiting);
                } catch (Exception) {
                    // Try weaker sublayers before reporting the prim as missing.
                }
            }
            throw new Exception("missing prim: " ~ primPath);
        }
        auto attr = attrName in found.attributes;
        if (auto samples = attrName in found.timeSamples) {
            if (auto sample = time in *samples) {
                return cast(Value) *sample;
            }
        }
        if (attr !is null) {
            return cast(Value) *attr;
        }
        auto nextVisiting = visiting ~ primPath;
        foreach (target; found.inherits) {
            if (!primIsValid(target)) {
                continue;
            }
            try {
                return composedAttribute(target, attrName, time, nextVisiting);
            } catch (Exception) {
                // Try weaker inherited targets before reporting the value as missing.
            }
        }
        foreach (target; found.specializes) {
            if (!primIsValid(target)) {
                continue;
            }
            try {
                return composedAttribute(target, attrName, time, nextVisiting);
            } catch (Exception) {
                // Try weaker specialize targets before reporting the value as missing.
            }
        }
        foreach (reference; found.references) {
            try {
                auto refStage = Stage.open(buildNormalizedPath(sourceDir, reference.assetPath));
                auto targetPath = reference.primPath.length != 0 ? reference.primPath : "/" ~ refStage.defaultPrim;
                return refStage.composedAttribute(targetPath, attrName, time, []);
            } catch (Exception) {
                // Try later references before reporting the value as missing.
            }
        }
        foreach (payload; found.payloads) {
            try {
                auto payloadStage = Stage.open(buildNormalizedPath(sourceDir, payload.assetPath));
                auto targetPath = payload.primPath.length != 0 ? payload.primPath : "/" ~ payloadStage.defaultPrim;
                return payloadStage.composedAttribute(targetPath, attrName, time, []);
            } catch (Exception) {
                // Try later payloads before reporting the value as missing.
            }
        }
        foreach (sublayer; sublayers) {
            try {
                return sublayer.stage.composedAttribute(primPath, attrName, sublayer.mapTime(time), visiting);
            } catch (Exception) {
                // Try weaker sublayers before reporting the value as missing.
            }
        }
        throw new Exception("missing attribute: " ~ attrName);
    }

    private string[] composedRelationshipTargets(string primPath, string relName, string[] visiting) const {
        primPath = mapComposedToAuthored(primPath);
        foreach (visited; visiting) {
            if (visited == primPath) {
                throw new Exception("relationship composition cycle while reading: " ~ primPath);
            }
        }
        auto found = primPath in prims;
        if (found is null) {
            foreach (sublayer; sublayers) {
                auto targets = sublayer.stage.composedRelationshipTargets(primPath, relName, visiting);
                if (targets.length != 0) {
                    return targets;
                }
            }
            return [];
        }
        if (auto local = relName in found.relationships) {
            return (*local).dup;
        }
        auto nextVisiting = visiting ~ primPath;
        foreach (target; found.inherits) {
            if (!primIsValid(target)) {
                continue;
            }
            auto targets = composedRelationshipTargets(target, relName, nextVisiting);
            if (targets.length != 0) {
                return targets;
            }
        }
        foreach (target; found.specializes) {
            if (!primIsValid(target)) {
                continue;
            }
            auto targets = composedRelationshipTargets(target, relName, nextVisiting);
            if (targets.length != 0) {
                return targets;
            }
        }
        foreach (reference; found.references) {
            try {
                auto refStage = Stage.open(buildNormalizedPath(sourceDir, reference.assetPath));
                auto targetPath = reference.primPath.length != 0 ? reference.primPath : "/" ~ refStage.defaultPrim;
                auto targets = refStage.composedRelationshipTargets(targetPath, relName, []);
                if (targets.length != 0) {
                    return targets;
                }
            } catch (Exception) {
            }
        }
        foreach (payload; found.payloads) {
            try {
                auto payloadStage = Stage.open(buildNormalizedPath(sourceDir, payload.assetPath));
                auto targetPath = payload.primPath.length != 0 ? payload.primPath : "/" ~ payloadStage.defaultPrim;
                auto targets = payloadStage.composedRelationshipTargets(targetPath, relName, []);
                if (targets.length != 0) {
                    return targets;
                }
            } catch (Exception) {
            }
        }
        foreach (sublayer; sublayers) {
            auto targets = sublayer.stage.composedRelationshipTargets(primPath, relName, visiting);
            if (targets.length != 0) {
                return targets;
            }
        }
        return [];
    }

    private Value composedCustomData(string primPath, string key, string[] visiting) const {
        primPath = mapComposedToAuthored(primPath);
        foreach (visited; visiting) {
            if (visited == primPath) {
                throw new Exception("customData composition cycle while reading: " ~ primPath);
            }
        }
        auto found = primPath in prims;
        if (found is null) {
            foreach (sublayer; sublayers) {
                try {
                    return sublayer.stage.composedCustomData(primPath, key, visiting);
                } catch (Exception) {
                }
            }
            throw new Exception("missing prim: " ~ primPath);
        }
        if (auto local = key in found.customData) {
            return cast(Value) *local;
        }
        auto nextVisiting = visiting ~ primPath;
        foreach (target; found.inherits) {
            if (!primIsValid(target)) {
                continue;
            }
            try {
                return composedCustomData(target, key, nextVisiting);
            } catch (Exception) {
            }
        }
        foreach (target; found.specializes) {
            if (!primIsValid(target)) {
                continue;
            }
            try {
                return composedCustomData(target, key, nextVisiting);
            } catch (Exception) {
            }
        }
        foreach (reference; found.references) {
            try {
                auto refStage = Stage.open(buildNormalizedPath(sourceDir, reference.assetPath));
                auto targetPath = reference.primPath.length != 0 ? reference.primPath : "/" ~ refStage.defaultPrim;
                return refStage.composedCustomData(targetPath, key, []);
            } catch (Exception) {
            }
        }
        foreach (payload; found.payloads) {
            try {
                auto payloadStage = Stage.open(buildNormalizedPath(sourceDir, payload.assetPath));
                auto targetPath = payload.primPath.length != 0 ? payload.primPath : "/" ~ payloadStage.defaultPrim;
                return payloadStage.composedCustomData(targetPath, key, []);
            } catch (Exception) {
            }
        }
        foreach (sublayer; sublayers) {
            try {
                return sublayer.stage.composedCustomData(primPath, key, visiting);
            } catch (Exception) {
            }
        }
        throw new Exception("missing customData key: " ~ key);
    }

    private PrimMetadata composedPrimMetadata(string primPath, string[] visiting) const {
        primPath = mapComposedToAuthored(primPath);
        foreach (visited; visiting) {
            if (visited == primPath) {
                throw new Exception("metadata composition cycle while reading: " ~ primPath);
            }
        }
        auto found = primPath in prims;
        if (found is null) {
            foreach (sublayer; sublayers) {
                try {
                    return sublayer.stage.composedPrimMetadata(primPath, visiting);
                } catch (Exception) {
                }
            }
            throw new Exception("missing prim: " ~ primPath);
        }
        PrimMetadata metadata;
        metadata.kind = found.kind;
        metadata.hasKind = found.hasKind;
        metadata.active = found.active;
        metadata.hasActiveOpinion = found.hasActiveOpinion;
        if (metadata.hasKind && metadata.hasActiveOpinion) {
            return metadata;
        }
        auto nextVisiting = visiting ~ primPath;
        foreach (target; found.inherits) {
            if (!primIsValid(target)) {
                continue;
            }
            try {
                auto inherited = composedPrimMetadata(target, nextVisiting);
                if (!metadata.hasKind && inherited.hasKind) {
                    metadata.kind = inherited.kind;
                    metadata.hasKind = true;
                }
                if (!metadata.hasActiveOpinion && inherited.hasActiveOpinion) {
                    metadata.active = inherited.active;
                    metadata.hasActiveOpinion = true;
                }
            } catch (Exception) {
            }
        }
        foreach (target; found.specializes) {
            if (!primIsValid(target)) {
                continue;
            }
            try {
                auto specialized = composedPrimMetadata(target, nextVisiting);
                if (!metadata.hasKind && specialized.hasKind) {
                    metadata.kind = specialized.kind;
                    metadata.hasKind = true;
                }
                if (!metadata.hasActiveOpinion && specialized.hasActiveOpinion) {
                    metadata.active = specialized.active;
                    metadata.hasActiveOpinion = true;
                }
            } catch (Exception) {
            }
        }
        foreach (reference; found.references) {
            try {
                auto refStage = Stage.open(buildNormalizedPath(sourceDir, reference.assetPath));
                auto targetPath = reference.primPath.length != 0 ? reference.primPath : "/" ~ refStage.defaultPrim;
                auto refMetadata = refStage.composedPrimMetadata(targetPath, []);
                if (!metadata.hasKind && refMetadata.hasKind) {
                    metadata.kind = refMetadata.kind;
                    metadata.hasKind = true;
                }
                if (!metadata.hasActiveOpinion && refMetadata.hasActiveOpinion) {
                    metadata.active = refMetadata.active;
                    metadata.hasActiveOpinion = true;
                }
            } catch (Exception) {
            }
        }
        foreach (payload; found.payloads) {
            try {
                auto payloadStage = Stage.open(buildNormalizedPath(sourceDir, payload.assetPath));
                auto targetPath = payload.primPath.length != 0 ? payload.primPath : "/" ~ payloadStage.defaultPrim;
                auto payloadMetadata = payloadStage.composedPrimMetadata(targetPath, []);
                if (!metadata.hasKind && payloadMetadata.hasKind) {
                    metadata.kind = payloadMetadata.kind;
                    metadata.hasKind = true;
                }
                if (!metadata.hasActiveOpinion && payloadMetadata.hasActiveOpinion) {
                    metadata.active = payloadMetadata.active;
                    metadata.hasActiveOpinion = true;
                }
            } catch (Exception) {
            }
        }
        foreach (sublayer; sublayers) {
            try {
                auto subMetadata = sublayer.stage.composedPrimMetadata(primPath, visiting);
                if (!metadata.hasKind && subMetadata.hasKind) {
                    metadata.kind = subMetadata.kind;
                    metadata.hasKind = true;
                }
                if (!metadata.hasActiveOpinion && subMetadata.hasActiveOpinion) {
                    metadata.active = subMetadata.active;
                    metadata.hasActiveOpinion = true;
                }
            } catch (Exception) {
            }
        }
        return metadata;
    }

    private string mapComposedToAuthored(string path) const {
        foreach (fromPath, toPath; relocates) {
            if (path == toPath) {
                return fromPath;
            }
            auto prefix = toPath ~ "/";
            if (path.startsWith(prefix)) {
                return fromPath ~ path[prefix.length - 1 .. $];
            }
        }
        return path;
    }
}

private Stage parseUsda(string text, string sourceDir = ".") {
    Stage stage;
    stage.sourceDir = sourceDir.length == 0 ? "." : sourceDir;
    string[] stack;
    string pendingPrimPath;
    bool inMetadataBlock;
    bool inIgnoredBraceBlock;
    bool inTimeSamplesBlock;
    bool inSublayerOffsetsBlock;
    bool inRelocatesBlock;
    bool inPrefixSubstitutionsBlock;
    bool inCustomDataBlock;
    string customDataPrimPath;
    string timeSamplePrimPath;
    string timeSampleAttrName;
    string timeSampleType;
    bool inVariantSetsBlock;
    bool inVariantSelectionBlock;
    string variantPrimPath;
    string currentVariantSet;
    string currentVariantName;
    int variantDepth;

    foreach (rawLine; text.lineSplitter()) {
        auto line = stripComment(rawLine).strip;
        if (line.length == 0) {
            continue;
        }
        if (inIgnoredBraceBlock) {
            if (line == "}") {
                inIgnoredBraceBlock = false;
            }
            continue;
        }
        if (inTimeSamplesBlock) {
            if (line == "}") {
                inTimeSamplesBlock = false;
                timeSamplePrimPath = "";
                timeSampleAttrName = "";
                timeSampleType = "";
            } else {
                addTimeSample(stage, timeSamplePrimPath, timeSampleAttrName, timeSampleType, line);
            }
            continue;
        }
        if (inSublayerOffsetsBlock) {
            if (line == "}") {
                inSublayerOffsetsBlock = false;
            } else {
                applySublayerOffset(stage, line);
            }
            continue;
        }
        if (inRelocatesBlock) {
            if (line == "}") {
                inRelocatesBlock = false;
            } else {
                addRelocate(stage, line);
            }
            continue;
        }
        if (inPrefixSubstitutionsBlock) {
            if (line == "}") {
                inPrefixSubstitutionsBlock = false;
            } else {
                addPrefixSubstitution(stage, line);
            }
            continue;
        }
        if (inCustomDataBlock) {
            if (line == "}") {
                inCustomDataBlock = false;
                customDataPrimPath = "";
            } else {
                addCustomData(stage, customDataPrimPath, line);
            }
            continue;
        }
        if (line == "(") {
            inMetadataBlock = true;
            continue;
        }
        if (line == ")") {
            inMetadataBlock = false;
            inVariantSetsBlock = false;
            inVariantSelectionBlock = false;
            variantPrimPath = "";
            currentVariantSet = "";
            currentVariantName = "";
            variantDepth = 0;
            continue;
        }
        if (inMetadataBlock) {
            if (inVariantSetsBlock) {
                if ((line == "}" || line == "},") && currentVariantSet.length == 0 && currentVariantName.length == 0) {
                    inVariantSetsBlock = false;
                    continue;
                }
                parseVariantSetMetadataLine(stage, variantPrimPath, currentVariantSet, currentVariantName, variantDepth, line);
            } else if (inVariantSelectionBlock) {
                if (line == "}") {
                    inVariantSelectionBlock = false;
                } else if (pendingPrimPath.length != 0 && line.canFind("=")) {
                    if (auto prim = pendingPrimPath in stage.prims) {
                        auto selection = parseVariantSelectionLine(line);
                        if (selection.setName.length != 0) {
                            prim.variantSelections[selection.setName] = selection.variantName;
                        }
                    }
                }
            } else if (line.startsWith("defaultPrim")) {
                stage.defaultPrim = parseQuotedOrToken(afterEquals(line));
            } else if (pendingPrimPath.length == 0 && line.startsWith("doc")) {
                stage.documentation = parseQuotedOrToken(afterEquals(line));
            } else if (pendingPrimPath.length == 0 && line.startsWith("metersPerUnit")) {
                stage.metersPerUnit = afterEquals(line).stripRight(",").to!double;
                stage.hasMetersPerUnit = true;
            } else if (pendingPrimPath.length == 0 && line.startsWith("upAxis")) {
                stage.upAxis = parseQuotedOrToken(afterEquals(line));
            } else if (pendingPrimPath.length == 0 && line.startsWith("startTimeCode")) {
                stage.startTimeCode = afterEquals(line).stripRight(",").to!double;
                stage.hasStartTimeCode = true;
            } else if (pendingPrimPath.length == 0 && line.startsWith("endTimeCode")) {
                stage.endTimeCode = afterEquals(line).stripRight(",").to!double;
                stage.hasEndTimeCode = true;
            } else if (line.startsWith("subLayers")) {
                foreach (assetPath; parseAssetList(afterEquals(line))) {
                    addSublayer(stage, assetPath);
                }
            } else if (line.startsWith("subLayerOffsets")) {
                inSublayerOffsetsBlock = true;
            } else if (line.startsWith("relocates")) {
                inRelocatesBlock = true;
            } else if (line.startsWith("prefixSubstitutions")) {
                inPrefixSubstitutionsBlock = true;
            } else if (pendingPrimPath.length != 0 && line.startsWith("kind")) {
                if (auto prim = pendingPrimPath in stage.prims) {
                    prim.kind = parseQuotedOrToken(afterEquals(line));
                    prim.hasKind = true;
                }
            } else if (pendingPrimPath.length != 0 && line.startsWith("active")) {
                if (auto prim = pendingPrimPath in stage.prims) {
                    prim.active = parseBoolToken(afterEquals(line));
                    prim.hasActiveOpinion = true;
                }
            } else if (pendingPrimPath.length != 0 && line.startsWith("customData")) {
                inCustomDataBlock = true;
                customDataPrimPath = pendingPrimPath;
            } else if (pendingPrimPath.length != 0 && line.startsWith("variantSets")) {
                inVariantSetsBlock = true;
                variantPrimPath = pendingPrimPath;
            } else if (pendingPrimPath.length != 0 && line.startsWith("variantSelection")) {
                inVariantSelectionBlock = true;
            } else if (pendingPrimPath.length != 0 && line.canFind("inherits")) {
                if (auto prim = pendingPrimPath in stage.prims) {
                    prim.inherits = parsePathList(afterEquals(line));
                }
            } else if (pendingPrimPath.length != 0 && line.canFind("specializes")) {
                if (auto prim = pendingPrimPath in stage.prims) {
                    prim.specializes = parsePathList(afterEquals(line));
                }
            } else if (pendingPrimPath.length != 0 && line.canFind("references")) {
                if (auto prim = pendingPrimPath in stage.prims) {
                    prim.references = parseReferenceList(afterEquals(line));
                }
            } else if (pendingPrimPath.length != 0 && line.canFind("payload")) {
                if (auto prim = pendingPrimPath in stage.prims) {
                    prim.payloads = parseReferenceList(afterEquals(line));
                }
            }
            continue;
        }
        if (line.canFind(".timeSamples") && line.canFind("{")) {
            auto header = parseTimeSamplesHeader(line);
            if (stack.length != 0 && header.attrName.length != 0) {
                inTimeSamplesBlock = true;
                timeSamplePrimPath = stack[$ - 1];
                timeSampleAttrName = header.attrName;
                timeSampleType = header.typeName;
            } else {
                inIgnoredBraceBlock = true;
            }
            continue;
        }
        if (line == "{") {
            if (pendingPrimPath.length != 0) {
                stack ~= pendingPrimPath;
                pendingPrimPath = "";
            }
            continue;
        }
        if (line == "}") {
            if (stack.length > 0) {
                stack = stack[0 .. $ - 1];
            }
            continue;
        }

        if (line.startsWith("defaultPrim")) {
            stage.defaultPrim = parseQuotedOrToken(afterEquals(line));
            continue;
        }
        if (line.startsWith("doc")) {
            stage.documentation = parseQuotedOrToken(afterEquals(line));
            continue;
        }
        if (line.startsWith("metersPerUnit")) {
            stage.metersPerUnit = afterEquals(line).stripRight(",").to!double;
            stage.hasMetersPerUnit = true;
            continue;
        }
        if (line.startsWith("upAxis")) {
            stage.upAxis = parseQuotedOrToken(afterEquals(line));
            continue;
        }
        if (line.startsWith("startTimeCode")) {
            stage.startTimeCode = afterEquals(line).stripRight(",").to!double;
            stage.hasStartTimeCode = true;
            continue;
        }
        if (line.startsWith("endTimeCode")) {
            stage.endTimeCode = afterEquals(line).stripRight(",").to!double;
            stage.hasEndTimeCode = true;
            continue;
        }
        if (line.startsWith("subLayers")) {
            foreach (assetPath; parseAssetList(afterEquals(line))) {
                addSublayer(stage, assetPath);
            }
            continue;
        }
        if (line.startsWith("subLayerOffsets")) {
            inSublayerOffsetsBlock = true;
            continue;
        }
        if (line.startsWith("relocates")) {
            inRelocatesBlock = true;
            continue;
        }
        if (line.startsWith("prefixSubstitutions")) {
            inPrefixSubstitutionsBlock = true;
            continue;
        }

        if (line.startsWith("def ") || line.startsWith("class ") || line.startsWith("over ")) {
            auto parsed = parsePrimHeader(line);
            auto parent = stack.length == 0 ? "" : stack[$ - 1];
            auto path = parent.length == 0 ? "/" ~ parsed.name : parent ~ "/" ~ parsed.name;
            Prim prim;
            prim.path = path;
            prim.typeName = parsed.typeName;
            prim.specifier = parsed.specifier;
            stage.prims[path] = prim;
            pendingPrimPath = path;
            if (line.canFind("(")) {
                inMetadataBlock = true;
            }
            continue;
        }

        if (stack.length == 0 || !line.canFind("=") || line.canFind(".timeSamples")) {
            continue;
        }

        if (line.startsWith("rel ")) {
            auto rel = parseRelationship(line);
            if (rel.name.length != 0) {
                stage.prims[stack[$ - 1]].relationships[rel.name] = rel.targets;
            }
            continue;
        }

        auto attr = parseAttribute(line);
        if (attr.name.length == 0) {
            continue;
        }
        stage.prims[stack[$ - 1]].attributes[attr.name] = attr.value;
    }

    applySelectedVariants(stage);
    return stage;
}

private string stripComment(string line) {
    auto index = line.indexOf("#");
    return index < 0 ? line : line[0 .. index];
}

private string afterEquals(string line) {
    auto index = line.indexOf("=");
    return index < 0 ? "" : line[index + 1 .. $].strip;
}

private string parseQuotedOrToken(string value) {
    value = value.strip;
    if (value.length >= 2 && value[0] == '"' && value[$ - 1] == '"') {
        return value[1 .. $ - 1];
    }
    return stripQuotes(value);
}

private string stripQuotes(string value) {
    value = value.strip;
    if (value.length >= 2 && value[0] == '"' && value[$ - 1] == '"') {
        return value[1 .. $ - 1];
    }
    return value;
}

private struct ParsedPrimHeader {
    string typeName;
    string name;
    Prim.Specifier specifier;
}

private ParsedPrimHeader parsePrimHeader(string line) {
    auto firstQuote = line.indexOf("\"");
    auto secondQuote = firstQuote < 0 ? -1 : line.indexOf("\"", firstQuote + 1);
    if (firstQuote < 0 || secondQuote < 0) {
        throw new Exception("invalid prim header: " ~ line);
    }
    auto before = line[0 .. firstQuote].strip.split;
    ParsedPrimHeader parsed;
    if (before.length > 0) {
        switch (before[0]) {
        case "class":
            parsed.specifier = Prim.Specifier.class_;
            break;
        case "over":
            parsed.specifier = Prim.Specifier.over;
            break;
        default:
            parsed.specifier = Prim.Specifier.def_;
            break;
        }
    }
    parsed.typeName = before.length >= 2 ? before[$ - 1] : "";
    parsed.name = line[firstQuote + 1 .. secondQuote];
    return parsed;
}

private bool parseBoolToken(string rawValue) {
    rawValue = rawValue.strip.stripRight(",");
    return rawValue == "true" || rawValue == "1";
}

private struct ParsedAttribute {
    string name;
    Value value;
}

private struct ParsedRelationship {
    string name;
    string[] targets;
}

private struct ParsedTimeSamplesHeader {
    string typeName;
    string attrName;
}

private struct ParsedVariantSelection {
    string setName;
    string variantName;
}

private ParsedTimeSamplesHeader parseTimeSamplesHeader(string line) {
    ParsedTimeSamplesHeader parsed;
    auto eq = line.indexOf("=");
    if (eq < 0) {
        return parsed;
    }
    auto lhsParts = line[0 .. eq].strip.split;
    if (lhsParts.length < 2) {
        return parsed;
    }
    parsed.typeName = lhsParts[0];
    auto name = lhsParts[$ - 1];
    enum suffix = ".timeSamples";
    if (!name.endsWith(suffix)) {
        return ParsedTimeSamplesHeader();
    }
    parsed.attrName = name[0 .. $ - suffix.length];
    return parsed;
}

private void addTimeSample(ref Stage stage, string primPath, string attrName, string typeName, string line) {
    auto colon = line.indexOf(":");
    if (colon < 0 || primPath.length == 0 || attrName.length == 0) {
        return;
    }
    auto time = line[0 .. colon].strip.to!double;
    auto rawValue = line[colon + 1 .. $].stripRight(",");
    auto sample = parseValue(typeName, rawValue);
    if (sample.kind == Value.Kind.none) {
        return;
    }
    auto prim = primPath in stage.prims;
    if (prim is null) {
        return;
    }
    Value[double] samples;
    if (auto existing = attrName in prim.timeSamples) {
        samples = *existing;
    }
    samples[time] = sample;
    prim.timeSamples[attrName] = samples;
}

private void addCustomData(ref Stage stage, string primPath, string line) {
    if (primPath.length == 0 || !line.canFind("=")) {
        return;
    }
    auto attr = parseAttribute(line);
    if (attr.name.length == 0 || attr.value.kind == Value.Kind.none) {
        return;
    }
    if (auto prim = primPath in stage.prims) {
        prim.customData[attr.name] = attr.value;
    }
}

private string variantKey(string setName, string variantName) {
    return setName ~ "\x1f" ~ variantName;
}

private void parseVariantSetMetadataLine(
    ref Stage stage,
    string variantPrimPath,
    ref string currentSet,
    ref string currentVariant,
    ref int variantDepth,
    string line) {
    if (variantPrimPath.length == 0) {
        return;
    }
    if (currentVariant.length != 0) {
        const bool closes = line == "}" || line == "},";
        if (closes) {
            if (variantDepth > 1) {
                appendVariantPayloadLine(stage, variantPrimPath, currentSet, currentVariant, line);
            }
            --variantDepth;
            if (variantDepth <= 0) {
                currentVariant = "";
                variantDepth = 0;
            }
            return;
        }
        appendVariantPayloadLine(stage, variantPrimPath, currentSet, currentVariant, line);
        if (line == "{" || line.endsWith("{")) {
            ++variantDepth;
        }
        return;
    }

    if (line == "}" || line == "},") {
        if (currentSet.length != 0) {
            currentSet = "";
        }
        return;
    }

    auto eq = line.indexOf("=");
    if (eq < 0 || !line.canFind("{")) {
        return;
    }
    auto lhs = line[0 .. eq].strip;
    if (lhs.startsWith("\"")) {
        currentVariant = parseQuotedOrToken(lhs);
        variantDepth = 1;
    } else {
        currentSet = lhs;
    }
}

private void appendVariantPayloadLine(ref Stage stage, string primPath, string setName, string variantName, string line) {
    auto prim = primPath in stage.prims;
    if (prim is null || setName.length == 0 || variantName.length == 0) {
        return;
    }
    const key = variantKey(setName, variantName);
    VariantPayload payload;
    if (auto existing = key in prim.variants) {
        payload = *existing;
    }
    payload.lines ~= line;
    prim.variants[key] = payload;
}

private ParsedVariantSelection parseVariantSelectionLine(string line) {
    ParsedVariantSelection outValue;
    auto eq = line.indexOf("=");
    if (eq < 0) {
        return outValue;
    }
    auto lhs = line[0 .. eq].strip.split;
    if (lhs.length == 0) {
        return outValue;
    }
    outValue.setName = lhs[$ - 1];
    outValue.variantName = parseQuotedOrToken(line[eq + 1 .. $].stripRight(","));
    return outValue;
}

private void applySelectedVariants(ref Stage stage) {
    auto paths = stage.prims.keys;
    foreach (path; paths) {
        auto prim = path in stage.prims;
        if (prim is null) {
            continue;
        }
        foreach (setName, variantName; prim.variantSelections) {
            auto payload = variantKey(setName, variantName) in prim.variants;
            if (payload is null) {
                continue;
            }
            applyVariantPayload(stage, path, payload.lines);
        }
    }
}

private void applyVariantPayload(ref Stage stage, string hostPath, string[] lines) {
    string[] stack = [hostPath];
    string pendingPrimPath;
    foreach (rawLine; lines) {
        auto line = rawLine.strip;
        if (line.length == 0 || line == "(" || line == ")") {
            continue;
        }
        if (line == "{") {
            if (pendingPrimPath.length != 0) {
                stack ~= pendingPrimPath;
                pendingPrimPath = "";
            }
            continue;
        }
        if (line == "}") {
            if (stack.length > 1) {
                stack = stack[0 .. $ - 1];
            }
            continue;
        }
        if (line.startsWith("def ") || line.startsWith("class ") || line.startsWith("over ")) {
            auto parsed = parsePrimHeader(line);
            auto path = stack[$ - 1] ~ "/" ~ parsed.name;
            Prim prim;
            prim.path = path;
            prim.typeName = parsed.typeName;
            stage.prims[path] = prim;
            pendingPrimPath = path;
            continue;
        }
        if (!line.canFind("=") || line.canFind(".timeSamples")) {
            continue;
        }
        if (line.startsWith("rel ")) {
            auto rel = parseRelationship(line);
            if (rel.name.length != 0) {
                stage.prims[stack[$ - 1]].relationships[rel.name] = rel.targets;
            }
            continue;
        }
        auto attr = parseAttribute(line);
        if (attr.name.length != 0) {
            stage.prims[stack[$ - 1]].attributes[attr.name] = attr.value;
        }
    }
}

private ParsedRelationship parseRelationship(string line) {
    ParsedRelationship parsed;
    auto eq = line.indexOf("=");
    if (eq < 0) {
        return parsed;
    }
    auto lhs = line[0 .. eq].strip.split;
    if (lhs.length < 2) {
        return parsed;
    }
    parsed.name = lhs[$ - 1];
    parsed.targets = parsePathList(line[eq + 1 .. $].stripRight(","));
    return parsed;
}

private ParsedAttribute parseAttribute(string line) {
    auto eq = line.indexOf("=");
    if (eq < 0) {
        return ParsedAttribute();
    }
    auto lhsParts = line[0 .. eq].strip.split;
    if (lhsParts.length < 2) {
        return ParsedAttribute();
    }
    auto typeName = lhsParts[0];
    auto attrName = lhsParts[$ - 1];
    auto rawValue = line[eq + 1 .. $].stripRight(",");

    ParsedAttribute parsed;
    parsed.name = attrName;
    parsed.value = parseValue(typeName, rawValue);
    return parsed;
}

private Value parseValue(string typeName, string rawValue) {
    rawValue = rawValue.strip;
    switch (typeName) {
    case "double":
        return Value.makeDouble(rawValue.to!double);
    case "float":
        return Value.makeFloat(rawValue.to!float);
    case "int":
        return Value.makeInt(rawValue.to!long);
    case "bool":
        return Value.makeBool(rawValue == "true" || rawValue == "1");
    case "string":
        return Value.makeString(parseQuotedOrToken(rawValue));
    case "asset":
        return Value.makeAsset(parseAssetValue(rawValue));
    case "token":
        return Value.makeToken(parseQuotedOrToken(rawValue));
    case "token[]":
        return Value.makeTokenArray(parseTokenArray(rawValue));
    case "double3":
        return Value.makeVec3(parseVec3(rawValue), false);
    case "color3f":
    case "normal3f":
    case "vector3f":
        return Value.makeVec3(parseVec3(rawValue), true);
    default:
        return Value();
    }
}

private string[] parseTokenArray(string rawValue) {
    rawValue = rawValue.strip;
    if (rawValue.length < 2 || rawValue[0] != '[' || rawValue[$ - 1] != ']') {
        return [];
    }
    auto body = rawValue[1 .. $ - 1].strip;
    if (body.length == 0) {
        return [];
    }
    string[] values;
    foreach (part; body.split(",")) {
        auto token = part.strip;
        if (token.length >= 2 && token[0] == '@' && token[$ - 1] == '@') {
            token = token[1 .. $ - 1];
        }
        values ~= token;
    }
    return values;
}

private string[] parsePathList(string rawValue) {
    rawValue = rawValue.strip;
    if (rawValue.length == 0) {
        return [];
    }
    if (rawValue.length >= 2 && rawValue[0] == '[' && rawValue[$ - 1] == ']') {
        string[] outPaths;
        foreach (part; rawValue[1 .. $ - 1].split(",")) {
            auto parsed = parsePathToken(part.strip);
            if (parsed.length != 0) {
                outPaths ~= parsed;
            }
        }
        return outPaths;
    }
    auto path = parsePathToken(rawValue);
    return path.length == 0 ? [] : [path];
}

private Reference[] parseReferenceList(string rawValue) {
    rawValue = rawValue.strip;
    Reference parsed = parseReferenceToken(rawValue);
    return parsed.assetPath.length == 0 ? [] : [parsed];
}

private string[] parseAssetList(string rawValue) {
    rawValue = rawValue.strip;
    if (rawValue.length >= 2 && rawValue[0] == '[' && rawValue[$ - 1] == ']') {
        string[] assets;
        foreach (part; rawValue[1 .. $ - 1].split(",")) {
            auto asset = parseAssetToken(part.strip);
            if (asset.length != 0) {
                assets ~= asset;
            }
        }
        return assets;
    }
    auto asset = parseAssetToken(rawValue);
    return asset.length == 0 ? [] : [asset];
}

private void addSublayer(ref Stage stage, string assetPath) {
    Sublayer sublayer;
    sublayer.assetPath = assetPath;
    sublayer.offset = 0.0;
    sublayer.scale = 1.0;
    sublayer.stage = Stage.open(buildNormalizedPath(stage.sourceDir, assetPath));
    stage.sublayers ~= sublayer;
}

private void applySublayerOffset(ref Stage stage, string line) {
    auto assetPath = parseAssetToken(line);
    if (assetPath.length == 0) {
        return;
    }
    auto colon = line.indexOf(":");
    if (colon < 0) {
        return;
    }
    auto tupleText = line[colon + 1 .. $].strip.stripRight(",");
    if (tupleText.length < 2 || tupleText[0] != '(' || tupleText[$ - 1] != ')') {
        return;
    }
    auto parts = tupleText[1 .. $ - 1].split(",");
    if (parts.length != 2) {
        return;
    }
    auto offset = parts[0].strip.to!double;
    auto scale = parts[1].strip.to!double;
    foreach (ref sublayer; stage.sublayers) {
        if (sublayer.assetPath == assetPath) {
            sublayer.offset = offset;
            sublayer.scale = scale;
            return;
        }
    }
}

private void addRelocate(ref Stage stage, string line) {
    auto colon = line.indexOf(":");
    if (colon < 0) {
        return;
    }
    auto fromPath = parsePathToken(line[0 .. colon]);
    auto toPath = parsePathToken(line[colon + 1 .. $].stripRight(","));
    if (fromPath.length == 0 || toPath.length == 0) {
        return;
    }
    stage.relocates[fromPath] = toPath;
}

private void addPrefixSubstitution(ref Stage stage, string line) {
    auto colon = line.indexOf(":");
    if (colon < 0) {
        return;
    }
    auto fromPrefix = parseQuotedOrToken(line[0 .. colon].strip);
    auto toPrefix = parseQuotedOrToken(line[colon + 1 .. $].stripRight(","));
    if (fromPrefix.length == 0 || toPrefix.length == 0) {
        return;
    }
    stage.prefixSubstitutions[fromPrefix] = toPrefix;
}

private string parseAssetToken(string rawValue) {
    rawValue = rawValue.strip;
    auto begin = rawValue.indexOf("@");
    if (begin < 0) {
        return "";
    }
    auto end = rawValue.indexOf("@", begin + 1);
    if (end <= begin) {
        return "";
    }
    return rawValue[begin + 1 .. end];
}

private string parseAssetValue(string rawValue) {
    rawValue = rawValue.strip;
    auto begin = rawValue.indexOf("@");
    auto end = rawValue.indexOf("@", begin + 1);
    if (begin < 0 || end <= begin) {
        return parseQuotedOrToken(rawValue);
    }
    return rawValue[begin + 1 .. end];
}

private Reference parseReferenceToken(string rawValue) {
    rawValue = rawValue.strip;
    Reference reference;
    auto assetBegin = rawValue.indexOf("@");
    if (assetBegin < 0) {
        return reference;
    }
    auto assetEnd = rawValue.indexOf("@", assetBegin + 1);
    if (assetEnd <= assetBegin) {
        return reference;
    }
    reference.assetPath = rawValue[assetBegin + 1 .. assetEnd];
    reference.primPath = parsePathToken(rawValue[assetEnd + 1 .. $]);
    return reference;
}

private string parsePathToken(string rawValue) {
    rawValue = rawValue.strip;
    auto begin = rawValue.indexOf("<");
    auto end = rawValue.indexOf(">");
    if (begin < 0 || end <= begin) {
        return "";
    }
    return rawValue[begin + 1 .. end];
}

private string parentPath(string path) {
    if (path == "/" || path.length == 0) {
        return "";
    }
    auto index = lastIndexOf(path, "/");
    if (index <= 0) {
        return "/";
    }
    return path[0 .. index];
}

private string[] ancestorPaths(string path) {
    string[] reversed;
    auto current = path;
    while (current.length != 0 && current != "/") {
        reversed ~= current;
        current = parentPath(current);
    }
    string[] ordered;
    foreach_reverse (item; reversed) {
        ordered ~= item;
    }
    return ordered;
}

private Vec3d parseVec3(string rawValue) {
    rawValue = rawValue.strip;
    if (rawValue.length < 2 || rawValue[0] != '(' || rawValue[$ - 1] != ')') {
        throw new Exception("invalid vec3 value: " ~ rawValue);
    }
    auto parts = rawValue[1 .. $ - 1].split(",");
    if (parts.length != 3) {
        throw new Exception("invalid vec3 component count: " ~ rawValue);
    }
    return Vec3d(parts[0].strip.to!double, parts[1].strip.to!double, parts[2].strip.to!double);
}

unittest {
    auto stage = Stage.open("../../tests/fixtures/usd_cross_language.usda");
    assert(stage.defaultPrim == "Scene");
    assert(stage.documentation == "cross_lang_fixture");
    assert(stage.hasMetersPerUnit && stage.metersPerUnit == 0.01);
    assert(stage.upAxis == "Y");
    assert(stage.hasStartTimeCode && stage.startTimeCode == 0.0);
    assert(stage.hasEndTimeCode && stage.endTimeCode == 100.0);
    assert(stage.primIsValid("/Scene/Child"));
    assert(!stage.primIsValid("/Scene/Missing"));
    assert(stage.primAt("/Scene/Child").typeName == "Cube");
    assert(stage.readDouble("/Scene/Child", "mass") == 2.5);
    assert(stage.readDouble("/Scene/Child", "mass", 2.0) == 4.0);
    assert(stage.readDouble("/Scene/Child", "density") == 1.25);
    assert(stage.readString("/Scene/Child", "label") == "hello");
    assert(stage.readString("/Scene/Child", "kind") == "component");
    assert(stage.readTokenArray("/Scene/Child", "tags") == ["a", "b"]);
    auto extent = stage.readVec3("/Scene/Child", "extent");
    assert(extent.x == 1.0 && extent.y == 2.0 && extent.z == 3.0);
    assert(stage.listPrimInherits("/Scene/ArcHost") == ["/Scene/Child"]);
    assert(stage.readDouble("/Scene/ArcHost", "arcOnly") == 7.0);
    assert(stage.readDouble("/Scene/ArcHost", "mass") == 2.5);
    assert(stage.readDouble("/Scene/ArcHost", "mass", 2.0) == 4.0);
    assert(stage.readString("/Scene/ArcHost", "label") == "hello");
    assert(stage.readTokenArray("/Scene/ArcHost", "tags") == ["a", "b"]);
    assert(stage.customDataInt("/Scene/Child", "tag") == 99);
}

unittest {
    auto stage = Stage.open("../../tests/fixtures/parity_physics_rigid_body_refs.usda");
    assert(stage.primIsValid("/World/RefHost"));
    assert(stage.readDouble("/World/RefHost", "physics:mass") == 7.0);
}

unittest {
    auto stage = Stage.open("../../tests/fixtures/parity_namespace.usda");
    assert(stage.readDouble("/Library/Source", "radius") == 2.0);
    assert(stage.readDouble("/Library/Source", "refOnly") == 11.0);
    assert(stage.readDouble("/Library/Source", "payloadOnly") == 33.0);
    assert(stage.relationshipTargets("/Library/Source", "refLink") == ["/RefRoot/RefLeaf"]);
    assert(stage.prefixSubstitutionKeyInAnyLayer("/Assets"));
    assert(stage.composedPrefixSubstitution("/Assets") == "/ResolvedAssets");
    assert(stage.primIsValid("/Library/Published"));
    assert(stage.readDouble("/Library/Published", "radius") == 2.0);
    assert(stage.readDouble("/Library/Published", "refOnly") == 11.0);
    assert(stage.readDouble("/Library/Published", "payloadOnly") == 33.0);
}

unittest {
    auto stage = Stage.open("../../tests/fixtures/parity_kind_active_refs.usda");
    assert(stage.resolvePrimSpecifier("/World/KindClass") == Prim.Specifier.class_);
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
    auto stage = Stage.open("../../tests/fixtures/parity_variants.usda");
    assert(stage.primIsValid("/World/VariantHost"));
    assert(stage.primIsValid("/World/VariantHost/VariantChild"));
    assert(stage.readDouble("/World/VariantHost", "variantValue") == 5.0);
    assert(stage.readDouble("/World/VariantHost/VariantChild", "branch") == 9.0);
}

unittest {
    auto stage = Stage.open("../../tests/fixtures/parity_variant_selection_refs.usda");
    assert(stage.primIsValid("/World/RefHost"));
    assert(stage.readDouble("/World/RefHost", "variantValue") == 9.0);
}

unittest {
    auto stage = Stage.open("../../tests/fixtures/parity_stack_root.usda");
    assert(stage.defaultPrim == "World");
    assert(stage.hasMetersPerUnit && stage.metersPerUnit == 0.01);
    assert(stage.upAxis == "Y");
    assert(stage.primIsValid("/World/Model"));
    assert(stage.readDouble("/World/Model", "rootOnly") == 1.0);
    assert(stage.readDouble("/World/Model", "animated", 0.0) == 1.0);
    assert(stage.readDouble("/World/Model", "animated", 10.0) == 2.0);
    assert(stage.readDouble("/World/Model", "strength") == 10.0);
    assert(stage.readDouble("/World/Model", "stackedOnly", 15.0) == 50.0);
}

unittest {
    auto stage = Stage.open("../../tests/fixtures/parity_imageable.usda");
    assert(stage.defaultPrim == "World");
    assert(stage.primAt("/World").typeName == "Xform");
    assert(stage.readString("/World", "purpose") == "render");
    assert(stage.readString("/World/Cube", "visibility") == "invisible");
    assert(stage.readDouble("/World/Cube", "size") == 2.0);
    auto translate = stage.readVec3("/World/Cube", "xformOp:translate");
    assert(translate.x == 1.0 && translate.y == 2.0 && translate.z == 3.0);
    auto local = stage.computeLocalTransform("/World/Cube");
    assert(local.rowMajor[12] == 1.0 && local.rowMajor[13] == 2.0 && local.rowMajor[14] == 3.0);
    auto l2w = stage.computeLocalToWorldTransform("/World/Cube");
    assert(l2w.rowMajor[12] == 1.0 && l2w.rowMajor[13] == 2.0 && l2w.rowMajor[14] == 3.0 && l2w.rowMajor[15] == 1.0);
    assert(!stage.computeImageableVisibility("/World/Cube"));
    assert(stage.computeImageablePurpose("/World/Cube") == "render");
    auto bounds = stage.computeBoundableWorldBounds("/World/Cube");
    assert(bounds.min.x == 0.0 && bounds.min.y == 1.0 && bounds.min.z == 2.0);
    assert(bounds.max.x == 2.0 && bounds.max.y == 3.0 && bounds.max.z == 4.0);
}

unittest {
    auto stage = Stage.open("../../tests/fixtures/parity_custom_data_inherit.usda");
    assert(stage.customDataString("/World/BaseClass", "role") == "base");
    assert(stage.customDataString("/World/Host", "role") == "base");
    assert(stage.customDataInt("/World/Host", "priority") == 9);
}

unittest {
    auto stage = Stage.open("../../tests/fixtures/parity_custom_data_refs.usda");
    assert(stage.customDataString("/World/RefHost", "role") == "from_ref");
    assert(stage.customDataInt("/World/RefHost", "priority") == 9);
    assert(stage.customDataString("/World/PayloadHost", "role") == "from_payload");
    assert(stage.customDataInt("/World/PayloadHost", "priority") == 3);
}

unittest {
    auto stage = Stage.open("../../tests/fixtures/parity_specializes.usda");
    assert(stage.listPrimSpecializes("/World/Host") == ["/World/BaseSpec"]);
    assert(stage.readDouble("/World/Host", "hostOnly") == 3.0);
    assert(stage.readDouble("/World/Host", "fromSpec") == 99.0);
    assert(stage.readDouble("/World/Host", "sharedStrength") == 10.0);
}

unittest {
    auto stage = Stage.open("../../tests/fixtures/parity_custom_data_specializes.usda");
    assert(stage.customDataString("/World/Host", "role") == "from_spec");
    assert(stage.customDataInt("/World/Host", "priority") == 9);
}

unittest {
    auto stage = Stage.open("../../tests/fixtures/parity_kind_active_specializes.usda");
    assert(stage.listPrimSpecializes("/World/SpecHost") == ["/World/KindSpec"]);
    assert(stage.resolvePrimKind("/World/SpecHost") == "assembly");
    assert(stage.resolveHasPrimActiveOpinion("/World/SpecHost"));
    assert(!stage.resolvePrimActive("/World/SpecHost"));
}

unittest {
    auto stage = Stage.open("../../tests/fixtures/parity_physics_fixed_joint.usda");
    assert(stage.relationshipTargets("/World/Anchor", "physics:body0") == ["/World/BodyA"]);
    assert(stage.relationshipTargets("/World/Anchor", "physics:body1") == ["/World/BodyB"]);
    assert(stage.readDouble("/World/Anchor", "physics:jointEnabled") == 1.0);
}

unittest {
    auto stage = Stage.open("../../tests/fixtures/parity_vol_openvdb.usda");
    assert(stage.primAt("/World/Smoke").typeName == "OpenVDBAsset");
    assert(stage.readString("/World/Smoke", "filePath") == "volumes/smoke.vdb");
    assert(stage.readString("/World/Smoke", "fieldName") == "density");
}

unittest {
    auto stage = Stage.open("../../tests/fixtures/parity_vol_volume.usda");
    assert(stage.primAt("/World/Cloud").typeName == "Volume");
    assert(stage.relationshipTargets("/World/Cloud", "field") == ["/World/Cloud/Smoke"]);
    assert(stage.readString("/World/Cloud/Smoke", "filePath") == "volumes/cloud/smoke.vdb");
    assert(stage.readString("/World/Cloud/Smoke", "fieldName") == "density");
}
