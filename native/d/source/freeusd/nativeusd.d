module freeusd.nativeusd;

import std.algorithm : canFind, startsWith;
import std.array : split;
import std.conv : to;
import std.file : readText;
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

struct Stage {
    string defaultPrim;
    Prim[string] prims;

    static Stage open(string path) {
        return parseUsda(readText(path));
    }

    bool primIsValid(string path) const {
        return (path in prims) !is null;
    }

    const(Prim) primAt(string path) const {
        auto found = path in prims;
        if (found is null) {
            throw new Exception("missing prim: " ~ path);
        }
        return cast(Prim) *found;
    }

    double readDouble(string primPath, string attrName) const {
        return primAt(primPath).attribute(attrName).asDouble();
    }

    string readString(string primPath, string attrName) const {
        auto value = primAt(primPath).attribute(attrName);
        if (value.kind != Value.Kind.string_ && value.kind != Value.Kind.token) {
            throw new Exception("attribute is not string-like: " ~ attrName);
        }
        return value.stringValue;
    }

    string[] readTokenArray(string primPath, string attrName) const {
        auto value = primAt(primPath).attribute(attrName);
        if (value.kind != Value.Kind.tokenArray) {
            throw new Exception("attribute is not token[]: " ~ attrName);
        }
        return value.tokens;
    }

    Vec3d readVec3(string primPath, string attrName) const {
        auto value = primAt(primPath).attribute(attrName);
        if (value.kind != Value.Kind.vec3d && value.kind != Value.Kind.vec3f) {
            throw new Exception("attribute is not vec3: " ~ attrName);
        }
        return value.vec3Value;
    }
}

private Stage parseUsda(string text) {
    Stage stage;
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
