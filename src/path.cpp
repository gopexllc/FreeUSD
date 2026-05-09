#include "freeusd/path.hpp"

#include <cctype>
#include <stdexcept>

namespace freeusd {
namespace {

bool is_identifier_start(char ch) {
  const unsigned char c = static_cast<unsigned char>(ch);
  return std::isalpha(c) || ch == '_';
}

bool is_identifier_continue(char ch) {
  const unsigned char c = static_cast<unsigned char>(ch);
  return std::isalnum(c) || ch == '_';
}

bool all_prim_segments_valid(std::string_view text) {
  if (text.empty() || text[0] != '/') {
    return false;
  }
  if (text == "/") {
    return true;
  }
  if (text.back() == '/') {
    return false;
  }

  std::size_t segment_start = 1;
  while (segment_start < text.size()) {
    const std::size_t segment_end = text.find('/', segment_start);
    const std::size_t length =
        (segment_end == std::string_view::npos ? text.size() : segment_end) - segment_start;
    if (length == 0 || !Path::is_valid_identifier(text.substr(segment_start, length))) {
      return false;
    }
    if (segment_end == std::string_view::npos) {
      break;
    }
    segment_start = segment_end + 1;
  }

  return true;
}

}  // namespace

Path::Path() : text_("/") {}

Path::Path(std::string text) : text_(std::move(text)) {
  if (!is_valid(text_)) {
    throw std::invalid_argument("invalid FreeUSD path: " + text_);
  }
}

Path Path::root() {
  return Path("/");
}

std::optional<Path> Path::parse(std::string_view text) {
  if (!is_valid(text)) {
    return std::nullopt;
  }
  return Path(std::string(text));
}

bool Path::is_valid(std::string_view text) {
  const std::size_t property_separator = text.find('.');
  if (property_separator == std::string_view::npos) {
    return all_prim_segments_valid(text);
  }
  if (text.find('.', property_separator + 1) != std::string_view::npos) {
    return false;
  }

  const std::string_view prim_path = text.substr(0, property_separator);
  const std::string_view property_name = text.substr(property_separator + 1);
  return all_prim_segments_valid(prim_path) && !prim_path.empty() && prim_path != "/" &&
         is_valid_property_name(property_name);
}

bool Path::is_valid_identifier(std::string_view text) {
  if (text.empty() || !is_identifier_start(text.front())) {
    return false;
  }
  for (const char ch : text.substr(1)) {
    if (!is_identifier_continue(ch)) {
      return false;
    }
  }
  return true;
}

bool Path::is_valid_property_name(std::string_view text) {
  if (text.empty()) {
    return false;
  }

  std::size_t component_start = 0;
  while (component_start < text.size()) {
    const std::size_t component_end = text.find(':', component_start);
    const std::size_t length =
        (component_end == std::string_view::npos ? text.size() : component_end) - component_start;
    if (length == 0 || !is_valid_identifier(text.substr(component_start, length))) {
      return false;
    }
    if (component_end == std::string_view::npos) {
      break;
    }
    component_start = component_end + 1;
  }

  return true;
}

bool Path::is_property_path() const noexcept {
  return text_.find('.') != std::string::npos;
}

std::string Path::name() const {
  const std::size_t property_separator = text_.find('.');
  if (property_separator != std::string::npos) {
    return text_.substr(property_separator + 1);
  }
  if (text_ == "/") {
    return {};
  }
  const std::size_t slash = text_.find_last_of('/');
  return text_.substr(slash == std::string::npos ? 0 : slash + 1);
}

std::string Path::parent_string() const {
  if (text_ == "/") {
    return {};
  }

  const std::size_t property_separator = text_.find('.');
  if (property_separator != std::string::npos) {
    return text_.substr(0, property_separator);
  }

  const std::size_t slash = text_.find_last_of('/');
  if (slash == 0) {
    return "/";
  }
  return text_.substr(0, slash);
}

Path Path::append_child(std::string_view child_name) const {
  if (is_property_path()) {
    throw std::invalid_argument("cannot append a child below a property path");
  }
  if (!is_valid_identifier(child_name)) {
    throw std::invalid_argument("invalid child name");
  }
  if (text_ == "/") {
    return Path("/" + std::string(child_name));
  }
  return Path(text_ + "/" + std::string(child_name));
}

Path Path::append_property(std::string_view property_name) const {
  if (is_root() || is_property_path()) {
    throw std::invalid_argument("properties must be appended to prim paths");
  }
  if (!is_valid_property_name(property_name)) {
    throw std::invalid_argument("invalid property name");
  }
  return Path(text_ + "." + std::string(property_name));
}

}  // namespace freeusd
