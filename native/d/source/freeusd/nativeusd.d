module freeusd.nativeusd;

import std.algorithm : canFind, startsWith;
import std.array : split;
import std.conv : to;
import std.file : readText;
import std.path : buildNormalizedPath, dirName;
import std.string : indexOf, lineSplitter, strip, stripLeft, stripRight;

struct Vec3d {
    double x;
    double y;
    double z;
}

alias Vec3f = Vec3d;

struct Value {
    enum Kind {
        none,
        double_,
        float_,
        bool_,
        int_,
        string_,
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
        case Kind.none, Kind.string_, Kind.token, Kind.tokenArray, Kind.vec3d, Kind.vec3f:
            throw new Exception("value is not numeric");
        }
    }
}

struct Prim {
    string path;
    string typeName;
    Value[string] attributes;
    string[] inherits;
    Reference[] references;
    Reference[] payloads;

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

struct Reference {
    string assetPath;
    string primPath;
}

struct Stage {
    string defaultPrim;
    string sourceDir;
    Prim[string] prims;
    Stage[] sublayers;

    static Stage open(string path) {
        return parseUsda(readText(path), dirName(path));
    }

    bool primIsValid(string path) const {
        if ((path in prims) !is null) {
            return true;
        }
        foreach (sublayer; sublayers) {
            if (sublayer.primIsValid(path)) {
                return true;
            }
        }
        return false;
    }

    const(Prim) primAt(string path) const {
        auto found = path in prims;
        if (found !is null) {
            return cast(Prim) *found;
        }
        foreach (sublayer; sublayers) {
            if (sublayer.primIsValid(path)) {
                return sublayer.primAt(path);
            }
        }
        throw new Exception("missing prim: " ~ path);
    }

    double readDouble(string primPath, string attrName) const {
        return composedAttribute(primPath, attrName).asDouble();
    }

    string readString(string primPath, string attrName) const {
        auto value = composedAttribute(primPath, attrName);
        if (value.kind != Value.Kind.string_ && value.kind != Value.Kind.token) {
            throw new Exception("attribute is not string-like: " ~ attrName);
        }
        return value.stringValue;
    }

    string[] readTokenArray(string primPath, string attrName) const {
        auto value = composedAttribute(primPath, attrName);
        if (value.kind != Value.Kind.tokenArray) {
            throw new Exception("attribute is not token[]: " ~ attrName);
        }
        return value.tokens;
    }

    Vec3d readVec3(string primPath, string attrName) const {
        auto value = composedAttribute(primPath, attrName);
        if (value.kind != Value.Kind.vec3d && value.kind != Value.Kind.vec3f) {
            throw new Exception("attribute is not vec3: " ~ attrName);
        }
        return value.vec3Value;
    }

    string[] listPrimInherits(string primPath) const {
        return (cast(string[]) primAt(primPath).inherits).dup;
    }

    private Value composedAttribute(string primPath, string attrName) const {
        return composedAttribute(primPath, attrName, []);
    }

    private Value composedAttribute(string primPath, string attrName, string[] visiting) const {
        foreach (visited; visiting) {
            if (visited == primPath) {
                throw new Exception("inherit cycle while reading: " ~ primPath);
            }
        }
        auto found = primPath in prims;
        if (found is null) {
            foreach (sublayer; sublayers) {
                try {
                    return sublayer.composedAttribute(primPath, attrName, visiting);
                } catch (Exception) {
                    // Try weaker sublayers before reporting the prim as missing.
                }
            }
            throw new Exception("missing prim: " ~ primPath);
        }
        auto attr = attrName in found.attributes;
        if (attr !is null) {
            return cast(Value) *attr;
        }
        auto nextVisiting = visiting ~ primPath;
        foreach (target; found.inherits) {
            if (!primIsValid(target)) {
                continue;
            }
            try {
                return composedAttribute(target, attrName, nextVisiting);
            } catch (Exception) {
                // Try weaker inherited targets before reporting the value as missing.
            }
        }
        foreach (reference; found.references) {
            try {
                auto refStage = Stage.open(buildNormalizedPath(sourceDir, reference.assetPath));
                auto targetPath = reference.primPath.length != 0 ? reference.primPath : "/" ~ refStage.defaultPrim;
                return refStage.composedAttribute(targetPath, attrName, []);
            } catch (Exception) {
                // Try later references before reporting the value as missing.
            }
        }
        foreach (payload; found.payloads) {
            try {
                auto payloadStage = Stage.open(buildNormalizedPath(sourceDir, payload.assetPath));
                auto targetPath = payload.primPath.length != 0 ? payload.primPath : "/" ~ payloadStage.defaultPrim;
                return payloadStage.composedAttribute(targetPath, attrName, []);
            } catch (Exception) {
                // Try later payloads before reporting the value as missing.
            }
        }
        foreach (sublayer; sublayers) {
            try {
                return sublayer.composedAttribute(primPath, attrName, visiting);
            } catch (Exception) {
                // Try weaker sublayers before reporting the value as missing.
            }
        }
        throw new Exception("missing attribute: " ~ attrName);
    }
}

private Stage parseUsda(string text, string sourceDir = ".") {
    Stage stage;
    stage.sourceDir = sourceDir.length == 0 ? "." : sourceDir;
    string[] stack;
    string pendingPrimPath;
    bool inMetadataBlock;
    bool inIgnoredBraceBlock;

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
        if (line == "(") {
            inMetadataBlock = true;
            continue;
        }
        if (line == ")") {
            inMetadataBlock = false;
            continue;
        }
        if (inMetadataBlock) {
            if (line.startsWith("defaultPrim")) {
                stage.defaultPrim = parseQuotedOrToken(afterEquals(line));
            } else if (line.startsWith("subLayers")) {
                foreach (assetPath; parseAssetList(afterEquals(line))) {
                    stage.sublayers ~= Stage.open(buildNormalizedPath(stage.sourceDir, assetPath));
                }
            } else if (pendingPrimPath.length != 0 && line.canFind("inherits")) {
                if (auto prim = pendingPrimPath in stage.prims) {
                    prim.inherits = parsePathList(afterEquals(line));
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
            inIgnoredBraceBlock = true;
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
        if (line.startsWith("subLayers")) {
            foreach (assetPath; parseAssetList(afterEquals(line))) {
                stage.sublayers ~= Stage.open(buildNormalizedPath(stage.sourceDir, assetPath));
            }
            continue;
        }

        if (line.startsWith("def ") || line.startsWith("class ") || line.startsWith("over ")) {
            auto parsed = parsePrimHeader(line);
            auto parent = stack.length == 0 ? "" : stack[$ - 1];
            auto path = parent.length == 0 ? "/" ~ parsed.name : parent ~ "/" ~ parsed.name;
            Prim prim;
            prim.path = path;
            prim.typeName = parsed.typeName;
            stage.prims[path] = prim;
            pendingPrimPath = path;
            continue;
        }

        if (stack.length == 0 || !line.canFind("=") || line.canFind(".timeSamples")) {
            continue;
        }

        auto attr = parseAttribute(line);
        if (attr.name.length == 0) {
            continue;
        }
        stage.prims[stack[$ - 1]].attributes[attr.name] = attr.value;
    }

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
}

private ParsedPrimHeader parsePrimHeader(string line) {
    auto firstQuote = line.indexOf("\"");
    auto secondQuote = firstQuote < 0 ? -1 : line.indexOf("\"", firstQuote + 1);
    if (firstQuote < 0 || secondQuote < 0) {
        throw new Exception("invalid prim header: " ~ line);
    }
    auto before = line[0 .. firstQuote].strip.split;
    ParsedPrimHeader parsed;
    parsed.typeName = before.length >= 2 ? before[$ - 1] : "";
    parsed.name = line[firstQuote + 1 .. secondQuote];
    return parsed;
}

private struct ParsedAttribute {
    string name;
    Value value;
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
    assert(stage.primIsValid("/Scene/Child"));
    assert(!stage.primIsValid("/Scene/Missing"));
    assert(stage.primAt("/Scene/Child").typeName == "Cube");
    assert(stage.readDouble("/Scene/Child", "mass") == 2.5);
    assert(stage.readDouble("/Scene/Child", "density") == 1.25);
    assert(stage.readString("/Scene/Child", "label") == "hello");
    assert(stage.readString("/Scene/Child", "kind") == "component");
    assert(stage.readTokenArray("/Scene/Child", "tags") == ["a", "b"]);
    auto extent = stage.readVec3("/Scene/Child", "extent");
    assert(extent.x == 1.0 && extent.y == 2.0 && extent.z == 3.0);
    assert(stage.listPrimInherits("/Scene/ArcHost") == ["/Scene/Child"]);
    assert(stage.readDouble("/Scene/ArcHost", "arcOnly") == 7.0);
    assert(stage.readDouble("/Scene/ArcHost", "mass") == 2.5);
    assert(stage.readString("/Scene/ArcHost", "label") == "hello");
    assert(stage.readTokenArray("/Scene/ArcHost", "tags") == ["a", "b"]);
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
}

unittest {
    auto stage = Stage.open("../../tests/fixtures/parity_stack_root.usda");
    assert(stage.defaultPrim == "World");
    assert(stage.primIsValid("/World/Model"));
    assert(stage.readDouble("/World/Model", "rootOnly") == 1.0);
    assert(stage.readDouble("/World/Model", "strength") == 10.0);
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
}
