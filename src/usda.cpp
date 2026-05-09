#include "freeusd/usda.hpp"

#include <charconv>
#include <cctype>
#include <stdexcept>
#include <system_error>

namespace freeusd {
namespace {

enum class TokenKind {
  End,
  Identifier,
  String,
  Number,
  LBrace,
  RBrace,
  Equal,
};

struct Token {
  TokenKind kind{TokenKind::End};
  std::string text;
  SourceLocation location;
};

class Lexer {
 public:
  explicit Lexer(std::string_view source) : source_(source) {}

  Token next() {
    skip_whitespace_and_comments();
    const SourceLocation start = location();
    if (at_end()) {
      return Token{TokenKind::End, {}, start};
    }

    const char ch = peek();
    if (ch == '{') {
      advance();
      return Token{TokenKind::LBrace, "{", start};
    }
    if (ch == '}') {
      advance();
      return Token{TokenKind::RBrace, "}", start};
    }
    if (ch == '=') {
      advance();
      return Token{TokenKind::Equal, "=", start};
    }
    if (ch == '"') {
      return read_string(start);
    }
    if (is_number_start(ch)) {
      return read_number(start);
    }
    if (is_identifier_start(ch)) {
      return read_identifier(start);
    }

    advance();
    return Token{TokenKind::Identifier, std::string(1, ch), start};
  }

 private:
  bool at_end() const noexcept { return index_ >= source_.size(); }

  char peek() const noexcept {
    return at_end() ? '\0' : source_[index_];
  }

  char peek_next() const noexcept {
    return index_ + 1 >= source_.size() ? '\0' : source_[index_ + 1];
  }

  char advance() {
    const char ch = source_[index_++];
    if (ch == '\n') {
      ++line_;
      column_ = 1;
    } else {
      ++column_;
    }
    return ch;
  }

  SourceLocation location() const noexcept {
    return SourceLocation{index_, line_, column_};
  }

  void skip_whitespace_and_comments() {
    while (!at_end()) {
      const char ch = peek();
      if (std::isspace(static_cast<unsigned char>(ch))) {
        advance();
        continue;
      }
      if (ch == '#') {
        while (!at_end() && peek() != '\n') {
          advance();
        }
        continue;
      }
      break;
    }
  }

  static bool is_identifier_start(char ch) {
    const unsigned char c = static_cast<unsigned char>(ch);
    return std::isalpha(c) || ch == '_';
  }

  static bool is_identifier_continue(char ch) {
    const unsigned char c = static_cast<unsigned char>(ch);
    return std::isalnum(c) || ch == '_' || ch == ':';
  }

  static bool is_number_start(char ch) {
    const unsigned char c = static_cast<unsigned char>(ch);
    return std::isdigit(c) || ((ch == '-' || ch == '+'));
  }

  Token read_identifier(SourceLocation start) {
    std::string text;
    while (!at_end() && is_identifier_continue(peek())) {
      text.push_back(advance());
    }
    return Token{TokenKind::Identifier, std::move(text), start};
  }

  Token read_number(SourceLocation start) {
    std::string text;
    if (peek() == '-' || peek() == '+') {
      text.push_back(advance());
    }
    while (!at_end() && std::isdigit(static_cast<unsigned char>(peek()))) {
      text.push_back(advance());
    }
    if (!at_end() && peek() == '.') {
      text.push_back(advance());
      while (!at_end() && std::isdigit(static_cast<unsigned char>(peek()))) {
        text.push_back(advance());
      }
    }
    if (!at_end() && (peek() == 'e' || peek() == 'E')) {
      text.push_back(advance());
      if (!at_end() && (peek() == '-' || peek() == '+')) {
        text.push_back(advance());
      }
      while (!at_end() && std::isdigit(static_cast<unsigned char>(peek()))) {
        text.push_back(advance());
      }
    }
    return Token{TokenKind::Number, std::move(text), start};
  }

  Token read_string(SourceLocation start) {
    advance();
    std::string text;
    while (!at_end()) {
      const char ch = advance();
      if (ch == '"') {
        return Token{TokenKind::String, std::move(text), start};
      }
      if (ch == '\\' && !at_end()) {
        const char escaped = advance();
        switch (escaped) {
          case 'n':
            text.push_back('\n');
            break;
          case 'r':
            text.push_back('\r');
            break;
          case 't':
            text.push_back('\t');
            break;
          case '\\':
          case '"':
            text.push_back(escaped);
            break;
          default:
            text.push_back(escaped);
            break;
        }
      } else {
        text.push_back(ch);
      }
    }
    return Token{TokenKind::String, std::move(text), start};
  }

  std::string_view source_;
  std::size_t index_{0};
  std::uint32_t line_{1};
  std::uint32_t column_{1};
};

class ParseFailure : public std::runtime_error {
 public:
  ParseFailure() : std::runtime_error("parse failed") {}
};

class Parser {
 public:
  Parser(std::string_view source, std::string identifier)
      : lexer_(source), layer_(std::move(identifier)), current_(lexer_.next()) {}

  ParseResult parse() {
    try {
      while (current_.kind != TokenKind::End) {
        layer_.add_root_prim(parse_prim(Path::root()));
      }
    } catch (const ParseFailure&) {
    }

    return ParseResult{std::move(layer_), std::move(diagnostics_)};
  }

 private:
  bool match(TokenKind kind) {
    if (current_.kind != kind) {
      return false;
    }
    advance();
    return true;
  }

  bool match_identifier(std::string_view text) {
    if (current_.kind != TokenKind::Identifier || current_.text != text) {
      return false;
    }
    advance();
    return true;
  }

  Token expect(TokenKind kind, const char* message) {
    if (current_.kind != kind) {
      fail(message, current_.location);
    }
    Token token = current_;
    advance();
    return token;
  }

  Token expect_identifier(const char* message) {
    return expect(TokenKind::Identifier, message);
  }

  void advance() {
    current_ = lexer_.next();
  }

  [[noreturn]] void fail(std::string message, SourceLocation location) {
    diagnostics_.push_back(Diagnostic{DiagnosticSeverity::Error, std::move(message), location});
    throw ParseFailure();
  }

  PrimSpecifier parse_specifier() {
    if (match_identifier("def")) {
      return PrimSpecifier::Def;
    }
    if (match_identifier("over")) {
      return PrimSpecifier::Over;
    }
    if (match_identifier("class")) {
      return PrimSpecifier::Class;
    }
    fail("expected prim specifier: def, over, or class", current_.location);
  }

  PrimSpec parse_prim(const Path& parent_path) {
    const PrimSpecifier specifier = parse_specifier();

    std::string type_name;
    std::string name;
    SourceLocation name_location = current_.location;

    if (current_.kind == TokenKind::Identifier) {
      Token first = current_;
      advance();

      if (current_.kind == TokenKind::String || current_.kind == TokenKind::Identifier) {
        type_name = std::move(first.text);
        name_location = current_.location;
        name = current_.text;
        advance();
      } else {
        name_location = first.location;
        name = std::move(first.text);
      }
    } else if (current_.kind == TokenKind::String) {
      name_location = current_.location;
      name = current_.text;
      advance();
    } else {
      fail("expected prim name", current_.location);
    }

    if (!Path::is_valid_identifier(name)) {
      fail("invalid prim name: " + name, name_location);
    }

    PrimSpec prim;
    prim.specifier = specifier;
    prim.type_name = std::move(type_name);
    prim.name = name;
    prim.path = parent_path.append_child(name);

    expect(TokenKind::LBrace, "expected '{' after prim declaration");

    while (current_.kind != TokenKind::End && current_.kind != TokenKind::RBrace) {
      if (current_.kind == TokenKind::Identifier &&
          (current_.text == "def" || current_.text == "over" || current_.text == "class")) {
        prim.children.push_back(parse_prim(prim.path));
      } else {
        prim.attributes.push_back(parse_attribute(prim.path));
      }
    }

    expect(TokenKind::RBrace, "expected '}' to close prim body");
    return prim;
  }

  AttributeSpec parse_attribute(const Path& prim_path) {
    AttributeSpec attribute;
    attribute.custom = match_identifier("custom");

    if (current_.kind == TokenKind::Identifier && (current_.text == "uniform" || current_.text == "varying")) {
      attribute.variability = current_.text;
      advance();
    }

    attribute.type_name = expect_identifier("expected attribute type name").text;
    attribute.name = expect_identifier("expected attribute name").text;
    if (!Path::is_valid_property_name(attribute.name)) {
      fail("invalid attribute name: " + attribute.name, current_.location);
    }
    attribute.path = prim_path.append_property(attribute.name);

    if (match(TokenKind::Equal)) {
      attribute.default_value = parse_value();
    }

    return attribute;
  }

  Value parse_value() {
    if (current_.kind == TokenKind::String) {
      std::string text = current_.text;
      advance();
      return Value::string(std::move(text));
    }
    if (current_.kind == TokenKind::Number) {
      std::string text = current_.text;
      advance();
      return parse_number_value(text);
    }
    if (current_.kind == TokenKind::Identifier) {
      std::string text = current_.text;
      advance();
      if (text == "true") {
        return Value::boolean(true);
      }
      if (text == "false") {
        return Value::boolean(false);
      }
      return Value::token(std::move(text));
    }
    fail("expected attribute value", current_.location);
  }

  Value parse_number_value(const std::string& text) {
    const bool is_double = text.find('.') != std::string::npos || text.find('e') != std::string::npos ||
                           text.find('E') != std::string::npos;
    if (is_double) {
      try {
        return Value::number(std::stod(text));
      } catch (const std::exception&) {
        fail("invalid floating-point literal: " + text, current_.location);
      }
    }

    std::int64_t value = 0;
    const char* begin = text.data();
    const char* end = text.data() + text.size();
    const std::from_chars_result result = std::from_chars(begin, end, value);
    if (result.ec != std::errc{} || result.ptr != end) {
      fail("invalid integer literal: " + text, current_.location);
    }
    return Value::integer(value);
  }

  Lexer lexer_;
  Layer layer_;
  Token current_;
  Diagnostics diagnostics_;
};

}  // namespace

bool ParseResult::ok() const {
  return !has_errors(diagnostics);
}

ParseResult parse_usda(std::string_view source, std::string identifier) {
  Parser parser(source, std::move(identifier));
  return parser.parse();
}

}  // namespace freeusd
