#include "freeusd/io/usda.hpp"

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <optional>
#include <sstream>
#include <unordered_map>
#include <variant>
#include <vector>

#include "freeusd/gf/vec3d.hpp"
#include "freeusd/sdf/layer.hpp"
#include "freeusd/sdf/path.hpp"
#include "freeusd/tf/token.hpp"
#include "freeusd/vt/value.hpp"

namespace freeusd::io::usda {
namespace {

std::string_view trim(std::string_view s) {
  while (!s.empty() && std::isspace(static_cast<unsigned char>(s.front()))) {
    s.remove_prefix(1);
  }
  while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back()))) {
    s.remove_suffix(1);
  }
  return s;
}

bool sv_starts_with(std::string_view s, std::string_view p) noexcept {
  return s.size() >= p.size() && s.compare(0, p.size(), p) == 0;
}

bool kw_line(std::string_view s, std::string_view kw) noexcept {
  s = trim(s);
  return s.size() >= kw.size() && s.compare(0, kw.size(), kw) == 0 &&
         (s.size() == kw.size() || std::isspace(static_cast<unsigned char>(s[kw.size()])) ||
          s[kw.size()] == '"' || s[kw.size()] == '\'');
}

bool is_def_line(std::string_view s) noexcept { return kw_line(s, "def"); }

bool is_prim_spec_line(std::string_view s) noexcept {
  return kw_line(s, "def") || kw_line(s, "class") || kw_line(s, "over");
}

std::string strip_comment_keep_strings(std::string_view line) {
  std::string out;
  out.reserve(line.size());
  bool in_double = false;
  bool in_single = false;
  for (std::size_t i = 0; i < line.size(); ++i) {
    const char c = line[i];
    if (!in_single && c == '"' && (i == 0 || line[i - 1] != '\\')) {
      in_double = !in_double;
      out.push_back(c);
      continue;
    }
    if (!in_double && c == '\'' && (i == 0 || line[i - 1] != '\\')) {
      in_single = !in_single;
      out.push_back(c);
      continue;
    }
    if (!in_double && !in_single && c == '#') {
      break;
    }
    out.push_back(c);
  }
  return out;
}

bool read_quoted_name(std::string_view s, std::size_t* idx, std::string* out) {
  while (*idx < s.size() && std::isspace(static_cast<unsigned char>(s[*idx]))) {
    ++*idx;
  }
  if (*idx >= s.size()) {
    return false;
  }
  const char q = s[*idx];
  if (q != '"' && q != '\'') {
    return false;
  }
  ++*idx;
  std::string name;
  while (*idx < s.size()) {
    const char c = s[*idx];
    if (c == '\\' && *idx + 1 < s.size()) {
      name.push_back(s[*idx + 1]);
      *idx += 2;
      continue;
    }
    if (c == q) {
      ++*idx;
      *out = std::move(name);
      return !out->empty();
    }
    name.push_back(c);
    ++*idx;
  }
  return false;
}

bool parse_def_line(std::string_view line, freeusd::sdf::Layer::PrimSpecifierKind* specifier, std::string* prim_type,
                    std::string* prim_name, std::size_t* idx_after_name) {
  line = trim(line);
  std::size_t i = 0;
  if (kw_line(line, "class")) {
    *specifier = freeusd::sdf::Layer::PrimSpecifierKind::Class;
    i = 5;
  } else if (kw_line(line, "over")) {
    *specifier = freeusd::sdf::Layer::PrimSpecifierKind::Over;
    i = 4;
  } else if (kw_line(line, "def")) {
    *specifier = freeusd::sdf::Layer::PrimSpecifierKind::Def;
    i = 3;
  } else {
    return false;
  }
  while (i < line.size() && std::isspace(static_cast<unsigned char>(line[i]))) {
    ++i;
  }
  if (i >= line.size()) {
    return false;
  }
  prim_type->clear();
  if (line[i] != '"' && line[i] != '\'') {
    const std::size_t t0 = i;
    while (i < line.size() && !std::isspace(static_cast<unsigned char>(line[i]))) {
      ++i;
    }
    *prim_type = std::string{line.substr(t0, i - t0)};
    while (i < line.size() && std::isspace(static_cast<unsigned char>(line[i]))) {
      ++i;
    }
  }
  if (!read_quoted_name(line, &i, prim_name)) {
    return false;
  }
  if (idx_after_name) {
    *idx_after_name = i;
  }
  return true;
}

bool parse_type_name_value(std::string_view line, std::string* type, std::string* name, std::string* value) {
  line = trim(line);
  const std::size_t sp = [&] {
    for (std::size_t j = 0; j < line.size(); ++j) {
      if (std::isspace(static_cast<unsigned char>(line[j]))) {
        return j;
      }
    }
    return std::string::npos;
  }();
  if (sp == std::string::npos) {
    return false;
  }
  *type = std::string{line.substr(0, sp)};
  std::string_view rest = trim(line.substr(sp));
  const std::size_t eq = rest.find('=');
  if (eq == std::string_view::npos) {
    return false;
  }
  std::string_view n = trim(rest.substr(0, eq));
  if (n.empty()) {
    return false;
  }
  *name = std::string{n};
  *value = std::string{trim(rest.substr(eq + 1))};
  // strip trailing semicolon
  while (!value->empty() && (value->back() == ';' || std::isspace(static_cast<unsigned char>(value->back())))) {
    if (value->back() == ';') {
      value->pop_back();
      *value = std::string{trim(*value)};
    } else {
      value->pop_back();
    }
  }
  return true;
}

void set_err(ParseResult* err, std::size_t line, const char* msg) {
  if (!err) {
    return;
  }
  err->ok = false;
  err->line = line;
  err->message = msg;
}

freeusd::vt::Value parse_value(std::string_view t, ParseResult* err, std::size_t line) {
  t = trim(t);
  if (t == "true") {
    return freeusd::vt::Value::MakeBool(true);
  }
  if (t == "false") {
    return freeusd::vt::Value::MakeBool(false);
  }
  if (!t.empty() && t.front() == '@') {
    if (t.size() < 2) {
      set_err(err, line, "empty asset path");
      return {};
    }
    const std::size_t end = t.find('@', 1);
    if (end == std::string_view::npos) {
      set_err(err, line, "unclosed asset path @");
      return {};
    }
    return freeusd::vt::Value::MakeString(std::string{trim(t.substr(1, end - 1))});
  }
  if (!t.empty() && (t.front() == '"' || t.front() == '\'')) {
    std::string name;
    std::size_t idx = 0;
    std::string tmp{t};
    if (!read_quoted_name(tmp, &idx, &name)) {
      set_err(err, line, "bad string literal");
      return {};
    }
    return freeusd::vt::Value::MakeString(std::move(name));
  }
  if (!t.empty() && t.front() == '(' && t.back() == ')') {
    t = trim(t.substr(1, t.size() - 2));
    double a{}, b{}, c{};
    char comma1 = 0;
    char comma2 = 0;
    std::istringstream iss{std::string{t}};
    iss >> a >> comma1 >> b >> comma2 >> c;
    if (!iss || comma1 != ',' || comma2 != ',') {
      set_err(err, line, "bad tuple3 literal");
      return {};
    }
    freeusd::gf::Vec3d v;
    v.set(a, b, c);
    return freeusd::vt::Value::MakeVec3d(v);
  }
  // Prefer full-token integer parsing, then floating-point parse; `'e'` in identifiers (e.g. Hello)
  // must not force the float branch.
  const std::string tstr{t};
  char* endi = nullptr;
  errno = 0;
  const long long vi = std::strtoll(tstr.c_str(), &endi, 10);
  if (endi == tstr.c_str() + tstr.size() && errno == 0) {
    if (vi > 2147483647LL || vi < -2147483648LL) {
      return freeusd::vt::Value::MakeInt64(static_cast<std::int64_t>(vi));
    }
    return freeusd::vt::Value::MakeInt32(static_cast<std::int32_t>(vi));
  }
  errno = 0;
  char* endf = nullptr;
  const double vd = std::strtod(tstr.c_str(), &endf);
  if (endf == tstr.c_str() + tstr.size() && errno == 0) {
    return freeusd::vt::Value::MakeDouble(vd);
  }
  // unquoted non-numeric literal -> TfToken
  return freeusd::vt::Value::MakeToken(freeusd::tf::Token{t});
}

constexpr std::string_view kTimeSamplesSuffix = ".timeSamples";
constexpr std::string_view kAttrConnectionSuffix = ".connect";

bool ends_with(std::string_view s, std::string_view suf) noexcept {
  return s.size() >= suf.size() && s.compare(s.size() - suf.size(), suf.size(), suf) == 0;
}

bool parse_time_colon_value(std::string_view line, double* t_out, freeusd::vt::Value* v_out, ParseResult* err, std::size_t line_no) {
  const std::size_t colon = line.find(':');
  if (colon == std::string_view::npos) {
    set_err(err, line_no, "time sample line missing ':'");
    return false;
  }
  std::string_view lt = trim(line.substr(0, colon));
  std::string_view rv = trim(line.substr(colon + 1));
  while (!rv.empty() && (rv.back() == ',' || rv.back() == ';' || std::isspace(static_cast<unsigned char>(rv.back())))) {
    rv.remove_suffix(1);
  }
  rv = trim(rv);
  if (lt.empty() || rv.empty()) {
    set_err(err, line_no, "bad time sample line");
    return false;
  }
  char* e1 = nullptr;
  const std::string lt_str{lt};
  const double t = std::strtod(lt_str.c_str(), &e1);
  if (e1 == lt_str.c_str()) {
    set_err(err, line_no, "bad time sample time");
    return false;
  }
  *v_out = parse_value(rv, err, line_no);
  if (err && !err->ok) {
    return false;
  }
  *t_out = t;
  return true;
}

bool parse_inline_time_samples(std::string_view inner,
                               const std::shared_ptr<freeusd::sdf::Layer>& layer,
                               const freeusd::sdf::Path& prim,
                               std::string_view base_name,
                               ParseResult* r,
                               std::size_t line_no) {
  while (!inner.empty()) {
    const std::size_t comma = inner.find(',');
    const std::string_view piece = trim(inner.substr(0, comma));
    if (!piece.empty()) {
      double t{};
      freeusd::vt::Value v;
      if (!parse_time_colon_value(piece, &t, &v, r, line_no)) {
        return false;
      }
      layer->SetTimeSample(prim, freeusd::tf::Token{std::string{base_name}}, t, v);
    }
    if (comma == std::string_view::npos) {
      break;
    }
    inner.remove_prefix(comma + 1);
  }
  return true;
}

std::string escape_string(const std::string& s) {
  std::string o;
  o.reserve(s.size() + 2);
  o.push_back('"');
  for (char c : s) {
    if (c == '"' || c == '\\') {
      o.push_back('\\');
    }
    o.push_back(c);
  }
  o.push_back('"');
  return o;
}

std::string value_to_usda(const freeusd::vt::Value& v) {
  if (v.IsEmpty()) {
    return "None";
  }
  if (v.HoldsBool()) {
    bool b{};
    v.GetBool(&b);
    return b ? "true" : "false";
  }
  if (v.HoldsDouble()) {
    double d{};
    v.GetDouble(&d);
    std::ostringstream oss;
    oss << std::setprecision(17) << d;
    return oss.str();
  }
  if (v.HoldsString()) {
    std::string s;
    v.GetString(&s);
    return escape_string(s);
  }
  if (v.HoldsToken()) {
    freeusd::tf::Token t;
    v.GetToken(&t);
    return t.GetText();
  }
  if (v.HoldsVec3d()) {
    freeusd::gf::Vec3d vec;
    v.GetVec3d(&vec);
    std::ostringstream oss;
    oss << "(" << vec.x() << ", " << vec.y() << ", " << vec.z() << ")";
    return oss.str();
  }
  if (const auto* i32 = std::get_if<std::int32_t>(&v.GetPayload())) {
    return std::to_string(*i32);
  }
  if (const auto* i64 = std::get_if<std::int64_t>(&v.GetPayload())) {
    return std::to_string(*i64);
  }
  if (const auto* f = std::get_if<float>(&v.GetPayload())) {
    std::ostringstream oss;
    oss << std::setprecision(9) << *f;
    return oss.str();
  }
  std::ostringstream oss;
  oss << v;
  return oss.str();
}

std::string field_type_hint(const freeusd::vt::Value& v) {
  if (v.HoldsBool()) {
    return "bool";
  }
  if (const auto* i32 = std::get_if<std::int32_t>(&v.GetPayload())) {
    (void)i32;
    return "int";
  }
  if (const auto* i64 = std::get_if<std::int64_t>(&v.GetPayload())) {
    (void)i64;
    return "int64";
  }
  if (const auto* f = std::get_if<float>(&v.GetPayload())) {
    (void)f;
    return "float";
  }
  if (v.HoldsDouble()) {
    return "double";
  }
  if (v.HoldsString()) {
    return "string";
  }
  if (v.HoldsToken()) {
    return "token";
  }
  if (v.HoldsVec3d()) {
    return "double3";
  }
  return "double";
}

std::string asset_path_to_usda(const std::string& s) { return std::string{"@"} + s + "@"; }

struct ParenConsumeResult {
  bool ok{false};
  std::size_t end_line{0};
  std::size_t resume_col{0};
  std::string inner;
};

ParenConsumeResult consume_paren_block(const std::vector<std::string>& lines,
                                       std::size_t line_idx,
                                       std::size_t open_col,
                                       ParseResult* err,
                                       std::size_t anchor_line_no) {
  ParenConsumeResult out;
  int depth = 0;
  std::ostringstream inner;
  bool in_double = false;
  bool in_single = false;

  for (std::size_t li = line_idx; li < lines.size(); ++li) {
    const std::string line = strip_comment_keep_strings(lines[li]);
    const std::size_t start_i = (li == line_idx ? open_col : 0);
    for (std::size_t i = start_i; i < line.size(); ++i) {
      const char c = line[i];

      if (!in_single && c == '"' && (i == 0 || line[i - 1] != '\\')) {
        in_double = !in_double;
        if (depth >= 1) {
          inner << c;
        }
        continue;
      }
      if (!in_double && c == '\'' && (i == 0 || line[i - 1] != '\\')) {
        in_single = !in_single;
        if (depth >= 1) {
          inner << c;
        }
        continue;
      }
      if (!in_double && !in_single) {
        if (c == '(') {
          ++depth;
          if (depth > 1) {
            inner << '(';
          }
          continue;
        }
        if (c == ')') {
          if (depth > 1) {
            inner << ')';
          }
          --depth;
          if (depth == 0) {
            out.ok = true;
            out.end_line = li;
            out.resume_col = i + 1;
            out.inner = inner.str();
            return out;
          }
          continue;
        }
      }
      if (depth >= 1) {
        inner << c;
      }
    }
    // Preserve newlines inside (...) so downstream metadata parsers split on assignments per line.
    if (depth > 0 && li + 1 < lines.size()) {
      inner << '\n';
    }
  }

  set_err(err, anchor_line_no, "unclosed '(' in prim or layer metadata");
  return out;
}

void strip_uniform_custom_qualifiers(std::string* line) {
  for (;;) {
    std::string_view s = trim(*line);
    if (sv_starts_with(s, "uniform ") && (s.size() == 8 || std::isspace(static_cast<unsigned char>(s[8])))) {
      *line = std::string(trim(s.substr(8)));
      continue;
    }
    if (sv_starts_with(s, "custom ") && (s.size() == 7 || std::isspace(static_cast<unsigned char>(s[7])))) {
      *line = std::string(trim(s.substr(7)));
      continue;
    }
    break;
  }
}

enum class RelListOpKind { Replace, Prepend, Append, Delete };

void strip_leading_rel_list_op(std::string* line, RelListOpKind* out_kind) {
  *out_kind = RelListOpKind::Replace;
  std::string_view s = trim(std::string_view(*line));
  if (kw_line(s, "delete")) {
    *out_kind = RelListOpKind::Delete;
    *line = std::string(trim(s.substr(6)));
    return;
  }
  if (kw_line(s, "prepend")) {
    *out_kind = RelListOpKind::Prepend;
    *line = std::string(trim(s.substr(7)));
    return;
  }
  if (kw_line(s, "append")) {
    *out_kind = RelListOpKind::Append;
    *line = std::string(trim(s.substr(6)));
    return;
  }
}

std::string flatten_newlines_inside_square_brackets(std::string_view in);

std::string ascii_lower(std::string_view in) {
  std::string o;
  o.reserve(in.size());
  for (char c : in) {
    o.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
  }
  return o;
}

bool tag_is_relationship_none(std::string_view t) {
  t = trim(t);
  return ascii_lower(t) == "none";
}

freeusd::sdf::Path parse_relationship_path_token(std::string_view t, ParseResult* err, std::size_t line) {
  t = trim(t);
  if (t.size() < 3 || t.front() != '<' || t.back() != '>') {
    set_err(err, line, "expected path literal like </Prim> in relationship");
    return {};
  }
  const std::string_view inner = trim(t.substr(1, t.size() - 2));
  const freeusd::sdf::Path p = freeusd::sdf::Path::FromString(inner);
  if (p.IsEmpty()) {
    set_err(err, line, "invalid relationship path");
    return {};
  }
  return p;
}

bool parse_relationship_targets_value(std::string_view val, std::vector<freeusd::sdf::Path>* out, ParseResult* err,
                                      std::size_t line) {
  out->clear();
  const std::string flat = flatten_newlines_inside_square_brackets(val);
  std::string_view v = trim(std::string_view{flat});
  if (tag_is_relationship_none(v)) {
    return true;
  }
  if (sv_starts_with(v, "[")) {
    int depth = 0;
    bool dq = false;
    bool sq = false;
    std::size_t close = std::string::npos;
    for (std::size_t i = 0; i < v.size(); ++i) {
      const char c = v[i];
      if (!sq && c == '"' && (i == 0 || v[i - 1] != '\\')) {
        dq = !dq;
        continue;
      }
      if (!dq && c == '\'' && (i == 0 || v[i - 1] != '\\')) {
        sq = !sq;
        continue;
      }
      if (!dq && !sq) {
        if (c == '[') {
          ++depth;
        } else if (c == ']') {
          --depth;
          if (depth == 0) {
            close = i;
            break;
          }
        }
      }
    }
    if (close == std::string_view::npos || close <= 1) {
      set_err(err, line, "bad relationship path list");
      return false;
    }
    std::string_view inner = trim(v.substr(1, close - 1));
    while (!inner.empty()) {
      const std::size_t comma = inner.find(',');
      const std::string_view piece = trim(inner.substr(0, comma));
      if (!piece.empty()) {
        const freeusd::sdf::Path p = parse_relationship_path_token(piece, err, line);
        if (!err->ok) {
          return false;
        }
        out->push_back(p);
      }
      if (comma == std::string_view::npos) {
        break;
      }
      inner.remove_prefix(comma + 1);
    }
    return true;
  }
  const freeusd::sdf::Path one = parse_relationship_path_token(v, err, line);
  if (!err->ok) {
    return false;
  }
  out->push_back(one);
  return true;
}

std::string path_to_usda_angle(const freeusd::sdf::Path& p) { return std::string{"<"} + p.GetText() + ">"; }

std::string relationship_targets_to_usda(const std::vector<freeusd::sdf::Path>& targets) {
  if (targets.empty()) {
    return "none";
  }
  if (targets.size() == 1) {
    return path_to_usda_angle(targets[0]);
  }
  std::ostringstream os;
  os << '[';
  for (std::size_t i = 0; i < targets.size(); ++i) {
    if (i != 0) {
      os << ", ";
    }
    os << path_to_usda_angle(targets[i]);
  }
  os << ']';
  return os.str();
}

/// Turn newlines inside `[`…`]` into spaces so line-based metadata parsing sees one assignment value.
std::string flatten_newlines_inside_square_brackets(std::string_view in) {
  std::string out;
  out.reserve(in.size());
  int bracket_depth = 0;
  bool in_double = false;
  bool in_single = false;
  for (std::size_t i = 0; i < in.size(); ++i) {
    const char c = in[i];
    if (!in_single && c == '"' && (i == 0 || in[i - 1] != '\\')) {
      in_double = !in_double;
      out.push_back(c);
      continue;
    }
    if (!in_double && c == '\'' && (i == 0 || in[i - 1] != '\\')) {
      in_single = !in_single;
      out.push_back(c);
      continue;
    }
    if (!in_double && !in_single) {
      if (c == '[') {
        ++bracket_depth;
        out.push_back(c);
        continue;
      }
      if (c == ']' && bracket_depth > 0) {
        --bracket_depth;
        out.push_back(c);
        continue;
      }
      if (c == '\n' && bracket_depth > 0) {
        out.push_back(' ');
        continue;
      }
    }
    out.push_back(c);
  }
  return out;
}

/// Turn newlines inside `{`…`}` into spaces so multi-line \c customData blocks can use comma separators.
std::string flatten_newlines_inside_curly_braces(std::string_view in) {
  std::string out;
  out.reserve(in.size());
  int brace_depth = 0;
  bool in_double = false;
  bool in_single = false;
  for (std::size_t i = 0; i < in.size(); ++i) {
    const char c = in[i];
    if (!in_single && c == '"' && (i == 0 || in[i - 1] != '\\')) {
      in_double = !in_double;
      out.push_back(c);
      continue;
    }
    if (!in_double && c == '\'' && (i == 0 || in[i - 1] != '\\')) {
      in_single = !in_single;
      out.push_back(c);
      continue;
    }
    if (!in_double && !in_single) {
      if (c == '{') {
        ++brace_depth;
        out.push_back(c);
        continue;
      }
      if (c == '}' && brace_depth > 0) {
        --brace_depth;
        out.push_back(c);
        continue;
      }
      if (c == '\n' && brace_depth > 0) {
        out.push_back(' ');
        continue;
      }
    }
    out.push_back(c);
  }
  return out;
}

bool ci_type_prefix_followed_by_space(std::string_view s, std::string_view prefix_lower) {
  if (s.size() <= prefix_lower.size()) {
    return false;
  }
  for (std::size_t j = 0; j < prefix_lower.size(); ++j) {
    if (std::tolower(static_cast<unsigned char>(s[j])) != prefix_lower[j]) {
      return false;
    }
  }
  return std::isspace(static_cast<unsigned char>(s[prefix_lower.size()]));
}

std::string normalize_custom_data_key(std::string_view lhs_full) {
  std::string_view lhs = trim(lhs_full);
  static constexpr std::string_view prefixes[] = {
      "double", "string", "int64", "float", "bool", "int", "token",
  };
  for (std::string_view pref : prefixes) {
    if (ci_type_prefix_followed_by_space(lhs, pref)) {
      lhs = trim(lhs.substr(pref.size()));
      break;
    }
  }
  lhs = trim(lhs);
  if (lhs.size() >= 2 && lhs.front() == '"' && lhs.back() == '"') {
    return std::string(lhs.substr(1, lhs.size() - 2));
  }
  if (lhs.size() >= 2 && lhs.front() == '\'' && lhs.back() == '\'') {
    return std::string(lhs.substr(1, lhs.size() - 2));
  }
  return std::string(lhs);
}

void split_custom_data_dictionary_entries(std::string_view inner, std::vector<std::string_view>* parts) {
  parts->clear();
  int depth_paren = 0;
  int depth_bracket = 0;
  int depth_brace = 0;
  bool dq = false;
  bool sq = false;
  std::size_t start = 0;
  for (std::size_t i = 0; i <= inner.size(); ++i) {
    char c{};
    bool at_end = (i >= inner.size());
    if (!at_end) {
      c = inner[i];
    }
    if (!at_end) {
      if (!sq && c == '"' && (i == 0 || inner[i - 1] != '\\')) {
        dq = !dq;
      } else if (!dq && c == '\'' && (i == 0 || inner[i - 1] != '\\')) {
        sq = !sq;
      } else if (!dq && !sq) {
        if (c == '(') {
          ++depth_paren;
        } else if (c == ')' && depth_paren > 0) {
          --depth_paren;
        } else if (c == '[') {
          ++depth_bracket;
        } else if (c == ']' && depth_bracket > 0) {
          --depth_bracket;
        } else if (c == '{') {
          ++depth_brace;
        } else if (c == '}' && depth_brace > 0) {
          --depth_brace;
        }
      }
    }
    const bool sep =
        !at_end && !dq && !sq && depth_paren == 0 && depth_bracket == 0 && depth_brace == 0 && (c == ',' || c == ';');
    if (sep || at_end) {
      const std::string_view piece = trim(inner.substr(start, i - start));
      if (!piece.empty()) {
        parts->push_back(piece);
      }
      start = i + 1;
    }
  }
}

std::optional<std::size_t> outer_closing_brace_index(std::string_view s, std::size_t open_brace) {
  if (open_brace >= s.size() || s[open_brace] != '{') {
    return std::nullopt;
  }
  int brace_depth = 0;
  bool dq = false;
  bool sq = false;
  for (std::size_t i = open_brace; i < s.size(); ++i) {
    const char c = s[i];
    if (!sq && c == '"' && (i == 0 || s[i - 1] != '\\')) {
      dq = !dq;
      continue;
    }
    if (!dq && c == '\'' && (i == 0 || s[i - 1] != '\\')) {
      sq = !sq;
      continue;
    }
    if (!dq && !sq) {
      if (c == '{') {
        ++brace_depth;
      } else if (c == '}') {
        --brace_depth;
        if (brace_depth == 0) {
          return i;
        }
      }
    }
  }
  return std::nullopt;
}

bool bare_custom_dict_key_emit(std::string_view k) noexcept {
  if (k.empty()) {
    return false;
  }
  unsigned char lead = static_cast<unsigned char>(k[0]);
  if (!std::isalpha(lead) && lead != '_' && lead != '$') {
    return false;
  }
  for (std::size_t i = 1; i < k.size(); ++i) {
    const unsigned char c = static_cast<unsigned char>(k[i]);
    if (!std::isalnum(c) && c != '_' && c != ':') {
      return false;
    }
  }
  return true;
}

bool split_assignment(std::string_view line, std::string_view* lhs, std::string_view* rhs) {
  bool in_double = false;
  bool in_single = false;
  for (std::size_t i = 0; i < line.size(); ++i) {
    const char c = line[i];
    if (!in_single && c == '"' && (i == 0 || line[i - 1] != '\\')) {
      in_double = !in_double;
      continue;
    }
    if (!in_double && c == '\'' && (i == 0 || line[i - 1] != '\\')) {
      in_single = !in_single;
      continue;
    }
    if (!in_double && !in_single && c == '=') {
      *lhs = trim(line.substr(0, i));
      *rhs = trim(line.substr(i + 1));
      std::size_t j = rhs->size();
      while (j > 0 && ((*rhs)[j - 1] == ';' || std::isspace(static_cast<unsigned char>((*rhs)[j - 1])))) {
        if ((*rhs)[j - 1] == ';') {
          *rhs = trim(rhs->substr(0, j - 1));
          break;
        }
        --j;
      }
      *rhs = trim(*rhs);
      return !lhs->empty();
    }
  }
  return false;
}

bool apply_prim_custom_data_from_dictionary(const std::shared_ptr<freeusd::sdf::Layer>& layer,
                                            const freeusd::sdf::Path& prim, std::string_view dict_inner,
                                            ParseResult* err, std::size_t line_no) {
  layer->ClearPrimCustomData(prim);
  dict_inner = trim(dict_inner);
  if (dict_inner.empty()) {
    return true;
  }
  std::vector<std::string_view> chunks;
  split_custom_data_dictionary_entries(dict_inner, &chunks);
  for (std::string_view piece : chunks) {
    piece = trim(piece);
    if (piece.empty()) {
      continue;
    }
    std::string_view lk;
    std::string_view rv;
    if (!split_assignment(piece, &lk, &rv)) {
      set_err(err, line_no, "bad customData entry (expected key = value)");
      return false;
    }
    const std::string key = normalize_custom_data_key(lk);
    if (key.empty()) {
      set_err(err, line_no, "empty customData key");
      return false;
    }
    freeusd::vt::Value val = parse_value(trim(rv), err, line_no);
    if (err && !err->ok) {
      return false;
    }
    if (val.IsEmpty()) {
      set_err(err, line_no, "empty customData value");
      return false;
    }
    layer->SetPrimCustomDataEntry(prim, key, val);
  }
  return true;
}

bool gather_asset_refs(std::string_view val_c, std::vector<std::string>* out_paths, ParseResult* err, std::size_t line_no) {
  std::string_view val = trim(val_c);
  if (sv_starts_with(val, "[")) {
    std::size_t close = std::string::npos;
    int bdepth = 0;
    for (std::size_t i = 0; i < val.size(); ++i) {
      if (val[i] == '[') {
        bdepth++;
      } else if (val[i] == ']') {
        bdepth--;
        if (bdepth == 0) {
          close = i;
          break;
        }
      }
    }
    if (close == std::string::npos || close <= 1) {
      set_err(err, line_no, "bad subLayers bracket list");
      return false;
    }
    std::string_view inner = trim(val.substr(1, close - 1));
    while (!inner.empty()) {
      const std::size_t comma = inner.find(',');
      const std::string_view piece = trim(inner.substr(0, comma));
      if (!piece.empty()) {
        const freeusd::vt::Value pv = parse_value(piece, err, line_no);
        if (!err->ok) {
          return false;
        }
        std::string ps;
        if (!pv.GetString(&ps)) {
          set_err(err, line_no, "expected quoted or @asset@ path");
          return false;
        }
        out_paths->push_back(std::move(ps));
      }
      if (comma == std::string_view::npos) {
        break;
      }
      inner.remove_prefix(comma + 1);
    }
    return true;
  }
  const freeusd::vt::Value pv = parse_value(val, err, line_no);
  if (!err->ok) {
    return false;
  }
  std::string ps;
  if (!pv.GetString(&ps)) {
    freeusd::tf::Token tk;
    if (pv.GetToken(&tk)) {
      out_paths->push_back(tk.GetText());
      return true;
    }
    set_err(err, line_no, "expected asset path");
    return false;
  }
  out_paths->push_back(std::move(ps));
  return true;
}

bool gather_prim_reference_list(std::string_view val_c, std::vector<freeusd::sdf::PrimReference>* out_refs, ParseResult* err,
                                std::size_t line_no) {
  out_refs->clear();
  std::string_view val = trim(val_c);
  if (sv_starts_with(val, "[")) {
    std::size_t close = std::string::npos;
    int bdepth = 0;
    for (std::size_t i = 0; i < val.size(); ++i) {
      if (val[i] == '[') {
        bdepth++;
      } else if (val[i] == ']') {
        bdepth--;
        if (bdepth == 0) {
          close = i;
          break;
        }
      }
    }
    if (close == std::string_view::npos || close <= 1) {
      set_err(err, line_no, "bad references bracket list");
      return false;
    }
    std::string_view inner = trim(val.substr(1, close - 1));
    while (!inner.empty()) {
      const std::size_t comma = inner.find(',');
      const std::string_view piece = trim(inner.substr(0, comma));
      if (!piece.empty()) {
        freeusd::sdf::PrimReference r{};
        if (!freeusd::sdf::PrimReference::ParseAuthored(piece, &r) || r.IsEmpty()) {
          set_err(err, line_no, "bad prim reference entry");
          return false;
        }
        out_refs->push_back(std::move(r));
      }
      if (comma == std::string_view::npos) {
        break;
      }
      inner.remove_prefix(comma + 1);
    }
    return true;
  }
  freeusd::sdf::PrimReference one{};
  if (!freeusd::sdf::PrimReference::ParseAuthored(val, &one) || one.IsEmpty()) {
    set_err(err, line_no, "expected prim reference");
    return false;
  }
  out_refs->push_back(std::move(one));
  return true;
}

void apply_layer_metadata_blob(const std::string& blob, freeusd::sdf::Layer* layer, ParseResult* err, std::size_t anchor_line) {
  const std::string flat = flatten_newlines_inside_square_brackets(blob);
  std::istringstream iss(flat);
  std::string line;
  while (std::getline(iss, line)) {
    std::string_view s = trim(line);
    if (s.empty() || s == ")" || s == "(") {
      continue;
    }
    std::string_view lhs{};
    std::string_view rhs{};
    if (!split_assignment(s, &lhs, &rhs)) {
      continue;
    }
    const std::string key = [](std::string_view k) {
      std::string o;
      o.reserve(k.size());
      for (char c : k) {
        o.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
      }
      return o;
    }(lhs);
    if (key == "doc" || key == "documentation") {
      freeusd::vt::Value v = parse_value(rhs, err, anchor_line);
      if (err && !err->ok) {
        return;
      }
      std::string t;
      if (v.GetString(&t)) {
        layer->SetDocumentation(std::move(t));
      }
    } else if (key == "defaultprim") {
      freeusd::vt::Value v = parse_value(rhs, err, anchor_line);
      if (err && !err->ok) {
        return;
      }
      std::string t;
      if (v.GetString(&t)) {
        layer->SetDefaultPrim(t);
      } else if (v.HoldsToken()) {
        freeusd::tf::Token tok;
        v.GetToken(&tok);
        layer->SetDefaultPrim(tok.GetText());
      }
    } else if (key == "sublayers") {
      std::vector<std::string> subs;
      if (!gather_asset_refs(rhs, &subs, err, anchor_line)) {
        return;
      }
      layer->SetSubLayers(std::move(subs));
    }
    // Ignore unknown metadata keys so slightly richer USDA still loads.
  }
}

void apply_prim_metadata_blob(const std::string& blob,
                              const std::shared_ptr<freeusd::sdf::Layer>& layer,
                              const freeusd::sdf::Path& prim,
                              ParseResult* err,
                              std::size_t anchor_line) {
  const std::string flat_sq = flatten_newlines_inside_square_brackets(blob);
  const std::string flat = flatten_newlines_inside_curly_braces(flat_sq);
  std::istringstream iss(flat);
  std::string line;
  while (std::getline(iss, line)) {
    std::string_view s = trim(line);
    if (s.empty()) {
      continue;
    }
    std::string_view lhs{};
    std::string_view rhs{};
    if (!split_assignment(s, &lhs, &rhs)) {
      continue;
    }
    const std::string key_full = [](std::string_view k) {
      std::string o;
      for (char c : k) {
        o.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
      }
      return o;
    }(lhs);

    std::vector<freeusd::sdf::PrimReference> refs;
    if (key_full == "prepend references" || key_full == "references") {
      if (!gather_prim_reference_list(rhs, &refs, err, anchor_line)) {
        break;
      }
      if (key_full == "references") {
        layer->ClearReferences(prim);
      }
      for (auto& rf : refs) {
        layer->AddPrimReference(prim, std::move(rf));
      }
    } else if (key_full == "active") {
      freeusd::vt::Value bv = parse_value(rhs, err, anchor_line);
      bool b{};
      if (!bv.GetBool(&b)) {
        continue;
      }
      layer->SetPrimActive(prim, b);
    } else if (key_full == "kind") {
      freeusd::vt::Value kv = parse_value(rhs, err, anchor_line);
      if (kv.HoldsToken()) {
        layer->SetField(prim, freeusd::tf::Token{"kind"}, kv);
      } else {
        std::string ks;
        if (kv.GetString(&ks)) {
          layer->SetField(prim, freeusd::tf::Token{"kind"}, freeusd::vt::Value::MakeToken(freeusd::tf::Token{ks}));
        }
      }
    } else if (key_full == "doc" || key_full == "documentation") {
      freeusd::vt::Value v = parse_value(rhs, err, anchor_line);
      layer->SetField(prim, freeusd::tf::Token{"documentation"}, v);
    } else if (key_full == "instanceable") {
      freeusd::vt::Value bv = parse_value(rhs, err, anchor_line);
      layer->SetField(prim, freeusd::tf::Token{"instanceable"}, bv);
    } else if (key_full == "customdata") {
      std::string_view rv = trim(rhs);
      const std::size_t open_brace = rv.find('{');
      if (open_brace == std::string_view::npos) {
        set_err(err, anchor_line, "customData requires a {...} dictionary");
        return;
      }
      const std::optional<std::size_t> close = outer_closing_brace_index(rv, open_brace);
      if (!close.has_value()) {
        set_err(err, anchor_line, "unclosed '{' in customData dictionary");
        return;
      }
      const std::string_view inner_dict = trim(rv.substr(open_brace + 1, *close - open_brace - 1));
      if (!apply_prim_custom_data_from_dictionary(layer, prim, inner_dict, err, anchor_line)) {
        return;
      }
    }
  }
}

std::size_t find_body_and_layer_header(std::vector<std::string>* lines_ref,
                                       const std::shared_ptr<freeusd::sdf::Layer>& layer,
                                       bool* seen_usda,
                                       ParseResult* r) {
  auto& lines = *lines_ref;
  *seen_usda = false;

  std::size_t idx = 0;
  while (idx < lines.size()) {
    const std::string_view raw_line = trim(std::string_view{lines[idx]});
    if (sv_starts_with(raw_line, "#usda")) {
      *seen_usda = true;
      ++idx;
      continue;
    }
    if (!*seen_usda && !raw_line.empty() && raw_line.front() == '#') {
      ++idx;
      continue;
    }
    const std::string raw = strip_comment_keep_strings(lines[idx]);
    std::string_view sv = trim(raw);
    if (sv.empty()) {
      ++idx;
      continue;
    }
    if (*seen_usda && sv.front() == '(') {
      const std::size_t col = raw.find('(');
      auto pc = consume_paren_block(lines, idx, col, r, idx + 1);
      if (!pc.ok) {
        return idx;
      }
      apply_layer_metadata_blob(pc.inner, layer.get(), r, idx + 1);
      if (!r->ok) {
        return idx;
      }
      idx = pc.end_line + 1;
      continue;
    }
    if (is_prim_spec_line(sv)) {
      break;
    }
    ++idx;
    continue;
  }
  return idx;
}

}  // namespace

ParseResult LoadFromString(std::string_view text, const std::shared_ptr<freeusd::sdf::Layer>& layer) {
  ParseResult r;
  r.ok = true;
  if (!layer) {
    r.ok = false;
    r.line = 0;
    r.message = "null layer";
    return r;
  }
  layer->Clear();

  std::vector<std::string> lines;
  {
    std::string buf{text};
    std::istringstream iss{buf};
    std::string line;
    while (std::getline(iss, line)) {
      if (!line.empty() && line.back() == '\r') {
        line.pop_back();
      }
      lines.push_back(std::move(line));
    }
  }

  std::vector<freeusd::sdf::Path> stack;
  std::optional<freeusd::sdf::Path> pending_def;
  bool seen_usda = false;

  std::size_t body_start = find_body_and_layer_header(&lines, layer, &seen_usda, &r);
  if (!r.ok) {
    return r;
  }

  for (std::size_t li = body_start; li < lines.size(); ++li) {
    const std::size_t line_no = li + 1;
    const std::string_view raw_line = trim(std::string_view{lines[li]});
    if (sv_starts_with(raw_line, "#usda")) {
      seen_usda = true;
      continue;
    }
    if (!seen_usda && !raw_line.empty() && raw_line.front() == '#') {
      continue;
    }
    std::string raw = strip_comment_keep_strings(lines[li]);
    std::string_view s = trim(raw);
    if (s.empty()) {
      continue;
    }

    if (s == "}") {
      if (stack.empty()) {
        r.ok = false;
        r.line = line_no;
        r.message = "unexpected '}'";
        return r;
      }
      stack.pop_back();
      continue;
    }

    if (s == "{") {
      if (!pending_def) {
        r.ok = false;
        r.line = line_no;
        r.message = "unexpected '{'";
        return r;
      }
      stack.push_back(*pending_def);
      pending_def.reset();
      continue;
    }

    if (is_prim_spec_line(s)) {
      if (pending_def) {
        r.ok = false;
        r.line = line_no;
        r.message = "expected '{' before next def";
        return r;
      }
      freeusd::sdf::Layer::PrimSpecifierKind spec{};
      std::string ptype;
      std::string pname;
      std::size_t iafter = 0;
      const std::string work = std::string(s);
      if (!parse_def_line(work, &spec, &ptype, &pname, &iafter)) {
        r.ok = false;
        r.line = line_no;
        r.message = "bad def/class/over line";
        return r;
      }
      const freeusd::sdf::Path parent = stack.empty() ? freeusd::sdf::Path::AbsoluteRootPath() : stack.back();
      const freeusd::sdf::Path child = parent.AppendChild(pname);
      if (child.IsEmpty()) {
        r.ok = false;
        r.line = line_no;
        r.message = "bad prim path from def name";
        return r;
      }
      if (!ptype.empty()) {
        layer->SetPrimKind(child, freeusd::tf::Token{ptype});
      }
      layer->SetPrimSpecifier(child, spec);

      std::string_view remainder = trim(std::string_view(work).substr(iafter));

      if (!remainder.empty() && remainder.front() == '(') {
        std::size_t tls = 0;
        while (tls < raw.size() && std::isspace(static_cast<unsigned char>(raw[tls]))) {
          ++tls;
        }
        std::size_t j = iafter;
        while (j < work.size() && std::isspace(static_cast<unsigned char>(work[j]))) {
          ++j;
        }
        if (j >= work.size() || work[j] != '(') {
          r.ok = false;
          r.line = line_no;
          r.message = "expected '(' for prim metadata";
          return r;
        }
        const std::size_t open_col_in_raw = tls + j;
        ParenConsumeResult pc = consume_paren_block(lines, li, open_col_in_raw, &r, line_no);
        if (!pc.ok) {
          return r;
        }
        apply_prim_metadata_blob(pc.inner, layer, child, &r, line_no);
        if (!r.ok) {
          return r;
        }
        const std::string tail_line = strip_comment_keep_strings(lines[pc.end_line]);
        remainder = trim(std::string_view(tail_line).substr(pc.resume_col));
        li = pc.end_line;
      }

      const std::size_t brace = [&](std::string_view v) -> std::size_t {
        bool in_double = false;
        bool in_single = false;
        for (std::size_t bi = 0; bi < v.size(); ++bi) {
          const char c = v[bi];
          if (!in_single && c == '"' && (bi == 0 || v[bi - 1] != '\\')) {
            in_double = !in_double;
            continue;
          }
          if (!in_double && c == '\'' && (bi == 0 || v[bi - 1] != '\\')) {
            in_single = !in_single;
            continue;
          }
          if (!in_double && !in_single && c == '{') {
            return bi;
          }
        }
        return std::string_view::npos;
      }(remainder);

      if (brace != std::string_view::npos) {
        std::string_view tail = trim(remainder.substr(brace + 1));
        stack.push_back(child);
        if (tail == "}") {
          stack.pop_back();
        } else if (!tail.empty()) {
          r.ok = false;
          r.line = line_no;
          r.message = "unexpected tokens after '{' on prim line";
          return r;
        }
      } else {
        pending_def = child;
      }
      continue;
    }

    if (pending_def) {
      const freeusd::sdf::Path pending_prim = *pending_def;
      if (s == "{") {
        stack.push_back(pending_prim);
        pending_def.reset();
        continue;
      }
      std::string_view ps = trim(raw);
      if (!ps.empty() && ps.front() == '(') {
        const std::size_t col_paren = raw.find('(');
        if (col_paren == std::string::npos) {
          r.ok = false;
          r.line = line_no;
          r.message = "bad prim metadata '('";
          return r;
        }
        ParenConsumeResult pc = consume_paren_block(lines, li, col_paren, &r, line_no);
        if (!pc.ok) {
          return r;
        }
        apply_prim_metadata_blob(pc.inner, layer, pending_prim, &r, line_no);
        if (!r.ok) {
          return r;
        }
        const std::string tail_ln = strip_comment_keep_strings(lines[pc.end_line]);
        std::string_view rem = trim(std::string_view(tail_ln).substr(pc.resume_col));
        li = pc.end_line;
        const std::size_t br = [&](std::string_view v) -> std::size_t {
          bool in_double = false;
          bool in_single = false;
          for (std::size_t bi = 0; bi < v.size(); ++bi) {
            const char c = v[bi];
            if (!in_single && c == '"' && (bi == 0 || v[bi - 1] != '\\')) {
              in_double = !in_double;
              continue;
            }
            if (!in_double && c == '\'' && (bi == 0 || v[bi - 1] != '\\')) {
              in_single = !in_single;
              continue;
            }
            if (!in_double && !in_single && c == '{') {
              return bi;
            }
          }
          return std::string_view::npos;
        }(rem);
        if (br != std::string_view::npos) {
          std::string_view tail = trim(rem.substr(br + 1));
          stack.push_back(pending_prim);
          pending_def.reset();
          if (tail == "}") {
            stack.pop_back();
          } else if (!tail.empty()) {
            r.ok = false;
            r.line = line_no;
            r.message = "unexpected tokens after '{' on prim line";
            return r;
          }
        }
        continue;
      }
      r.ok = false;
      r.line = line_no;
      r.message = "expected '{' after def";
      return r;
    }

    if (stack.empty()) {
      r.ok = false;
      r.line = line_no;
      r.message = "attribute outside prim scope";
      return r;
    }

    std::string typ;
    std::string fname;
    std::string fval;
    RelListOpKind rel_list_op = RelListOpKind::Replace;
    strip_leading_rel_list_op(&raw, &rel_list_op);
    strip_uniform_custom_qualifiers(&raw);
    s = trim(std::string_view(raw));
    if (!parse_type_name_value(s, &typ, &fname, &fval)) {
      r.ok = false;
      r.line = line_no;
      r.message = "bad attribute line";
      return r;
    }
    if (rel_list_op != RelListOpKind::Replace && ascii_lower(typ) != "rel") {
      set_err(&r, line_no, R"(unsupported relationship list op keyword for non-rel USDA statements)");
      return r;
    }
    const std::string_view fname_sv = fname;
    if (ascii_lower(typ) == "rel") {
      std::vector<freeusd::sdf::Path> tgts;
      const std::string_view rhs = trim(std::string_view{fval});
      if (!parse_relationship_targets_value(rhs, &tgts, &r, line_no)) {
        return r;
      }
      if (rel_list_op == RelListOpKind::Prepend) {
        layer->PrependRelationshipTargets(stack.back(), freeusd::tf::Token{fname}, std::move(tgts));
      } else if (rel_list_op == RelListOpKind::Append) {
        layer->AppendRelationshipTargets(stack.back(), freeusd::tf::Token{fname}, std::move(tgts));
      } else if (rel_list_op == RelListOpKind::Delete) {
        layer->DeleteRelationshipTargets(stack.back(), freeusd::tf::Token{fname}, std::move(tgts));
      } else {
        layer->SetRelationshipTargets(stack.back(), freeusd::tf::Token{fname}, std::move(tgts));
      }
      continue;
    }
    if (ends_with(fname_sv, kTimeSamplesSuffix)) {
      const std::string base = std::string{fname_sv.substr(0, fname_sv.size() - kTimeSamplesSuffix.size())};
      if (base.empty()) {
        r.ok = false;
        r.line = line_no;
        r.message = "empty base name for .timeSamples";
        return r;
      }
      const std::string_view tv = trim(fval);
      if (tv == "{") {
        std::size_t j = li + 1;
        for (; j < lines.size(); ++j) {
          const std::size_t inner_line_no = j + 1;
          const std::string rawj = strip_comment_keep_strings(lines[j]);
          const std::string_view sj = trim(rawj);
          if (sj.empty()) {
            continue;
          }
          if (sj == "}") {
            break;
          }
          double t{};
          freeusd::vt::Value v;
          if (!parse_time_colon_value(sj, &t, &v, &r, inner_line_no)) {
            return r;
          }
          layer->SetTimeSample(stack.back(), freeusd::tf::Token{base}, t, v);
        }
        if (j >= lines.size()) {
          r.ok = false;
          r.line = lines.size();
          r.message = "unclosed timeSamples block";
          return r;
        }
        li = j;
        continue;
      }
      if (tv.size() >= 2 && tv.front() == '{' && tv.back() == '}') {
        const std::string_view inner = trim(tv.substr(1, tv.size() - 2));
        if (!parse_inline_time_samples(inner, layer, stack.back(), base, &r, line_no)) {
          return r;
        }
        continue;
      }
      r.ok = false;
      r.line = line_no;
      r.message = "bad timeSamples initializer";
      return r;
    }

    if (ends_with(fname_sv, kAttrConnectionSuffix)) {
      std::string base_name =
          std::string{fname_sv.substr(0, fname_sv.size() - kAttrConnectionSuffix.size())};
      if (base_name.empty()) {
        r.ok = false;
        r.line = line_no;
        r.message = "empty attribute name before .connect";
        return r;
      }
      const freeusd::sdf::Path prop =
          parse_relationship_path_token(trim(std::string_view{fval}), &r, line_no);
      if (!r.ok) {
        return r;
      }
      if (!prop.IsPropertyPath()) {
        r.ok = false;
        r.line = line_no;
        r.message = "attribute .connect requires a property path target like </Prim.attr>";
        return r;
      }
      layer->SetAttributeConnection(stack.back(), freeusd::tf::Token{std::move(base_name)}, prop);
      continue;
    }

    freeusd::vt::Value v = parse_value(fval, &r, line_no);
    if (!r.ok) {
      return r;
    }
    layer->SetField(stack.back(), freeusd::tf::Token{fname}, v);
  }

  if (pending_def) {
    r.ok = false;
    r.line = lines.size();
    r.message = "unclosed def (missing '{')";
    return r;
  }

  if (!stack.empty()) {
    r.ok = false;
    r.line = lines.size();
    r.message = "unclosed '{' blocks";
    return r;
  }
  r.ok = true;
  return r;
}

std::string SaveToString(const freeusd::sdf::Layer& layer) {
  std::ostringstream os;
  os << "#usda 1.0\n";
  os << "(\n";
  if (!layer.GetDocumentation().empty()) {
    os << "    documentation = " << escape_string(layer.GetDocumentation()) << "\n";
  }
  if (layer.HasDefaultPrim()) {
    const auto dp = layer.GetDefaultPrim();
    os << "    defaultPrim = \"" << (dp.has_value() ? std::string{*dp} : std::string{}) << "\"\n";
  }
  const auto& subs = layer.GetSubLayers();
  if (!subs.empty()) {
    os << "    subLayers = [";
    for (std::size_t i = 0; i < subs.size(); ++i) {
      if (i) {
        os << ", ";
      }
      os << asset_path_to_usda(subs[i]);
    }
    os << "]\n";
  }
  if (!layer.HasDefaultPrim() && layer.GetDocumentation().empty() && subs.empty()) {
    os << "    doc = \"FreeUSD minimal USDA export\"\n";
  }
  os << ")\n\n";

  const freeusd::sdf::Path root = freeusd::sdf::Path::AbsoluteRootPath();
  const auto prims = layer.ListPrimPaths();

  std::unordered_map<std::string, std::vector<freeusd::sdf::Path>, freeusd::tf::HashString, std::equal_to<>> children;
  for (const auto& p : prims) {
    if (p.IsAbsoluteRootPath()) {
      continue;
    }
    const freeusd::sdf::Path par = p.GetParentPath();
    children[par.GetString()].push_back(p);
  }
  for (auto& e : children) {
    std::sort(e.second.begin(), e.second.end(), [](const freeusd::sdf::Path& a, const freeusd::sdf::Path& b) {
      return a.GetString() < b.GetString();
    });
  }

  const auto emit_prim = [&](auto&& self, const freeusd::sdf::Path& p, int indent) -> void {
    if (p.IsAbsoluteRootPath()) {
      const auto it = children.find(p.GetString());
      if (it == children.end()) {
        return;
      }
      for (const auto& c : it->second) {
        self(self, c, indent);
      }
      return;
    }
    const std::string pad(static_cast<std::size_t>(indent) * 4, ' ');
    const std::string name = p.GetName();
    std::ostringstream head;
    const auto spec = layer.GetPrimSpecifier(p);
    const char* spec_kw = "def";
    if (spec == freeusd::sdf::Layer::PrimSpecifierKind::Class) {
      spec_kw = "class";
    } else if (spec == freeusd::sdf::Layer::PrimSpecifierKind::Over) {
      spec_kw = "over";
    }
    head << spec_kw << " ";
    if (layer.HasPrimKind(p)) {
      head << layer.GetPrimKind(p).GetText() << " ";
    }
    head << "\"" << name << "\"";

    const bool meta_active_off = layer.HasPrimActiveOpinion(p) && !layer.IsPrimActive(p);
    const auto refs = layer.ListReferences(p);
    freeusd::vt::Value pv_doc;
    const bool meta_doc =
        layer.GetField(p, freeusd::tf::Token{"documentation"}, &pv_doc) ||
        layer.GetField(p, freeusd::tf::Token{"doc"}, &pv_doc);
    freeusd::vt::Value pv_kind;
    const bool meta_kind = layer.GetField(p, freeusd::tf::Token{"kind"}, &pv_kind);
    freeusd::vt::Value pv_inst;
    const bool meta_inst = layer.GetField(p, freeusd::tf::Token{"instanceable"}, &pv_inst);
    const std::vector<std::string> custom_keys = layer.ListPrimCustomDataKeys(p);
    const bool meta_custom = !custom_keys.empty();

    const bool emit_meta_paren =
        meta_active_off || !refs.empty() || meta_doc || meta_kind || meta_inst || meta_custom;
    os << pad << head.str();
    if (emit_meta_paren) {
      os << "\n" << pad << "(\n";
      if (meta_active_off) {
        os << pad << "    active = false\n";
      }
      for (const std::string& authored : layer.ListReferences(p)) {
        os << pad << "    prepend references = " << authored << "\n";
      }
      if (meta_doc) {
        os << pad << "    doc = " << value_to_usda(pv_doc) << "\n";
      }
      if (meta_kind) {
        os << pad << "    kind = " << value_to_usda(pv_kind) << "\n";
      }
      if (meta_inst) {
        os << pad << "    instanceable = " << value_to_usda(pv_inst) << "\n";
      }
      if (meta_custom) {
        os << pad << "    customData = {\n";
        for (const std::string& ck : custom_keys) {
          freeusd::vt::Value cv;
          if (!layer.GetPrimCustomDataEntry(p, ck, &cv)) {
            continue;
          }
          os << pad << "        ";
          if (bare_custom_dict_key_emit(ck)) {
            os << ck;
          } else {
            os << escape_string(ck);
          }
          os << " = " << value_to_usda(cv) << ",\n";
        }
        os << pad << "    }\n";
      }
      os << pad << ")\n";
    } else {
      os << "\n";
    }
    os << pad << "{\n";

    for (const auto& fn : layer.ListFieldNames(p)) {
      if (emit_meta_paren && meta_doc && (fn == "documentation" || fn == "doc")) {
        continue;
      }
      if (emit_meta_paren && meta_kind && fn == "kind") {
        continue;
      }
      if (emit_meta_paren && meta_inst && fn == "instanceable") {
        continue;
      }
      const freeusd::tf::Token fnt{fn};
      freeusd::vt::Value v;
      if (layer.GetField(p, fnt, &v)) {
        os << pad << "    " << field_type_hint(v) << " " << fn << " = " << value_to_usda(v) << "\n";
      }
      const auto times = layer.ListSampleTimes(p, fnt);
      if (!times.empty()) {
        freeusd::vt::Value hint;
        if (!layer.GetField(p, fnt, &hint) && !layer.GetTimeSample(p, fnt, times.front(), &hint)) {
          hint = freeusd::vt::Value::MakeDouble(0);
        }
        os << pad << "    " << field_type_hint(hint) << " " << fn << ".timeSamples = {\n";
        for (double t : times) {
          freeusd::vt::Value sv;
          if (layer.GetTimeSample(p, fnt, t, &sv)) {
            os << pad << "        " << std::setprecision(17) << t << ": " << value_to_usda(sv) << ",\n";
          }
        }
        os << pad << "    }\n";
      }
    }

    for (const std::string& rn : layer.ListRelationshipNames(p)) {
      const freeusd::tf::Token rt{rn};
      const std::vector<freeusd::sdf::Path> rtg = layer.GetRelationshipTargets(p, rt);
      os << pad << "    rel " << rn << " = " << relationship_targets_to_usda(rtg) << "\n";
    }

    for (const auto& ce : layer.ListAttributeConnections(p)) {
      os << pad << "    token " << ce.first << ".connect = " << path_to_usda_angle(ce.second) << "\n";
    }

    const auto it = children.find(p.GetString());
    if (it != children.end()) {
      for (const auto& c : it->second) {
        self(self, c, indent + 1);
      }
    }
    os << pad << "}\n";
  };

  emit_prim(emit_prim, root, 0);
  return os.str();
}

ParseResult LoadFromFile(const std::string& path, const std::shared_ptr<freeusd::sdf::Layer>& layer) {
  std::ifstream in(path, std::ios::binary);
  if (!in) {
    ParseResult r;
    r.ok = false;
    r.line = 0;
    r.message = "failed to open file";
    return r;
  }
  std::ostringstream oss;
  oss << in.rdbuf();
  return LoadFromString(oss.str(), layer);
}

ParseResult SaveToFile(const std::string& path, const freeusd::sdf::Layer& layer) {
  ParseResult r;
  std::ofstream out(path, std::ios::binary);
  if (!out) {
    r.ok = false;
    r.line = 0;
    r.message = "failed to open file";
    return r;
  }
  out << SaveToString(layer);
  if (!out.good()) {
    r.ok = false;
    r.line = 0;
    r.message = "failed to write file";
    return r;
  }
  r.ok = true;
  return r;
}

}  // namespace freeusd::io::usda
