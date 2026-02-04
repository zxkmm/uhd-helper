#pragma once

#include <cctype>
#include <map>
#include <cstring>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace json_min {

struct Value;
using Object = std::map<std::string, Value>;
using Array = std::vector<Value>;

struct Value {
  using Storage = std::variant<std::nullptr_t, bool, double, std::string, Array,
                               Object>;

  Storage storage;

  Value() : storage(nullptr) {}
  explicit Value(std::nullptr_t) : storage(nullptr) {}
  explicit Value(bool v) : storage(v) {}
  explicit Value(double v) : storage(v) {}
  explicit Value(std::string v) : storage(std::move(v)) {}
  explicit Value(Array v) : storage(std::move(v)) {}
  explicit Value(Object v) : storage(std::move(v)) {}

  bool IsNull() const { return std::holds_alternative<std::nullptr_t>(storage); }
  bool IsBool() const { return std::holds_alternative<bool>(storage); }
  bool IsNumber() const { return std::holds_alternative<double>(storage); }
  bool IsString() const { return std::holds_alternative<std::string>(storage); }
  bool IsArray() const { return std::holds_alternative<Array>(storage); }
  bool IsObject() const { return std::holds_alternative<Object>(storage); }

  const bool* AsBool() const { return std::get_if<bool>(&storage); }
  const double* AsNumber() const { return std::get_if<double>(&storage); }
  const std::string* AsString() const {
    return std::get_if<std::string>(&storage);
  }
  const Array* AsArray() const { return std::get_if<Array>(&storage); }
  const Object* AsObject() const { return std::get_if<Object>(&storage); }
};

class Parser {
 public:
  explicit Parser(std::string input) : input_(std::move(input)) {}

  Value Parse() {
    SkipWhitespace();
    Value value = ParseValue();
    SkipWhitespace();
    if (pos_ != input_.size()) {
      throw std::runtime_error("Unexpected trailing content");
    }
    return value;
  }

 private:
  Value ParseValue() {
    SkipWhitespace();
    if (Match("null")) {
      return Value(nullptr);
    }
    if (Match("true")) {
      return Value(true);
    }
    if (Match("false")) {
      return Value(false);
    }
    if (Peek() == '"') {
      return Value(ParseString());
    }
    if (Peek() == '{') {
      return Value(ParseObject());
    }
    if (Peek() == '[') {
      return Value(ParseArray());
    }
    if (Peek() == '-' || std::isdigit(static_cast<unsigned char>(Peek()))) {
      return Value(ParseNumber());
    }
    throw std::runtime_error("Invalid JSON value");
  }

  Object ParseObject() {
    Expect('{');
    SkipWhitespace();
    Object obj;
    if (Peek() == '}') {
      pos_++;
      return obj;
    }
    while (true) {
      SkipWhitespace();
      if (Peek() != '"') {
        throw std::runtime_error("Expected string key");
      }
      std::string key = ParseString();
      SkipWhitespace();
      Expect(':');
      SkipWhitespace();
      Value value = ParseValue();
      obj.emplace(std::move(key), std::move(value));
      SkipWhitespace();
      if (Peek() == '}') {
        pos_++;
        break;
      }
      Expect(',');
    }
    return obj;
  }

  Array ParseArray() {
    Expect('[');
    SkipWhitespace();
    Array arr;
    if (Peek() == ']') {
      pos_++;
      return arr;
    }
    while (true) {
      SkipWhitespace();
      arr.push_back(ParseValue());
      SkipWhitespace();
      if (Peek() == ']') {
        pos_++;
        break;
      }
      Expect(',');
    }
    return arr;
  }

  std::string ParseString() {
    Expect('"');
    std::string out;
    while (pos_ < input_.size()) {
      char c = input_[pos_++];
      if (c == '"') {
        return out;
      }
      if (c == '\\') {
        if (pos_ >= input_.size()) {
          throw std::runtime_error("Invalid escape");
        }
        char esc = input_[pos_++];
        switch (esc) {
          case '"':
          case '\\':
          case '/':
            out.push_back(esc);
            break;
          case 'b':
            out.push_back('\b');
            break;
          case 'f':
            out.push_back('\f');
            break;
          case 'n':
            out.push_back('\n');
            break;
          case 'r':
            out.push_back('\r');
            break;
          case 't':
            out.push_back('\t');
            break;
          case 'u':
            // Minimal unicode handling: skip 4 hex digits and emit '?'
            if (pos_ + 4 > input_.size()) {
              throw std::runtime_error("Invalid unicode escape");
            }
            pos_ += 4;
            out.push_back('?');
            break;
          default:
            throw std::runtime_error("Invalid escape sequence");
        }
        continue;
      }
      out.push_back(c);
    }
    throw std::runtime_error("Unterminated string");
  }

  double ParseNumber() {
    size_t start = pos_;
    if (Peek() == '-') {
      pos_++;
    }
    while (pos_ < input_.size() &&
           std::isdigit(static_cast<unsigned char>(input_[pos_]))) {
      pos_++;
    }
    if (pos_ < input_.size() && input_[pos_] == '.') {
      pos_++;
      while (pos_ < input_.size() &&
             std::isdigit(static_cast<unsigned char>(input_[pos_]))) {
        pos_++;
      }
    }
    if (pos_ < input_.size() &&
        (input_[pos_] == 'e' || input_[pos_] == 'E')) {
      pos_++;
      if (pos_ < input_.size() &&
          (input_[pos_] == '+' || input_[pos_] == '-')) {
        pos_++;
      }
      while (pos_ < input_.size() &&
             std::isdigit(static_cast<unsigned char>(input_[pos_]))) {
        pos_++;
      }
    }
    double value = 0.0;
    std::istringstream stream(input_.substr(start, pos_ - start));
    stream >> value;
    if (stream.fail()) {
      throw std::runtime_error("Invalid number");
    }
    return value;
  }

  char Peek() const {
    if (pos_ >= input_.size()) {
      return '\0';
    }
    return input_[pos_];
  }

  void Expect(char expected) {
    if (Peek() != expected) {
      throw std::runtime_error("Unexpected character");
    }
    pos_++;
  }

  bool Match(const char* token) {
    size_t len = std::strlen(token);
    if (input_.compare(pos_, len, token) == 0) {
      pos_ += len;
      return true;
    }
    return false;
  }

  void SkipWhitespace() {
    while (pos_ < input_.size() &&
           std::isspace(static_cast<unsigned char>(input_[pos_]))) {
      pos_++;
    }
  }

  std::string input_;
  size_t pos_ = 0;
};

inline std::string EscapeString(const std::string& value) {
  std::string out;
  out.reserve(value.size());
  for (char c : value) {
    switch (c) {
      case '"':
        out += "\\\"";
        break;
      case '\\':
        out += "\\\\";
        break;
      case '\n':
        out += "\\n";
        break;
      case '\r':
        out += "\\r";
        break;
      case '\t':
        out += "\\t";
        break;
      default:
        out.push_back(c);
        break;
    }
  }
  return out;
}

inline void Serialize(const Value& value, std::ostringstream& out, int indent,
                      int depth) {
  const auto pad = [&](int level) {
    for (int i = 0; i < level; ++i) {
      out << ' ';
    }
  };

  if (value.IsNull()) {
    out << "null";
    return;
  }
  if (value.IsBool()) {
    out << (*value.AsBool() ? "true" : "false");
    return;
  }
  if (value.IsNumber()) {
    out << *value.AsNumber();
    return;
  }
  if (value.IsString()) {
    out << '"' << EscapeString(*value.AsString()) << '"';
    return;
  }
  if (value.IsArray()) {
    const auto& arr = *value.AsArray();
    out << "[";
    if (!arr.empty()) {
      out << "\n";
      for (size_t i = 0; i < arr.size(); ++i) {
        pad((depth + 1) * indent);
        Serialize(arr[i], out, indent, depth + 1);
        if (i + 1 < arr.size()) {
          out << ",";
        }
        out << "\n";
      }
      pad(depth * indent);
    }
    out << "]";
    return;
  }
  if (value.IsObject()) {
    const auto& obj = *value.AsObject();
    out << "{";
    if (!obj.empty()) {
      out << "\n";
      size_t index = 0;
      for (const auto& [key, child] : obj) {
        pad((depth + 1) * indent);
        out << '"' << EscapeString(key) << "\": ";
        Serialize(child, out, indent, depth + 1);
        if (index + 1 < obj.size()) {
          out << ",";
        }
        out << "\n";
        index++;
      }
      pad(depth * indent);
    }
    out << "}";
    return;
  }
}

inline std::string Serialize(const Value& value, int indent = 2) {
  std::ostringstream out;
  Serialize(value, out, indent, 0);
  return out.str();
}

}  // namespace json_min
