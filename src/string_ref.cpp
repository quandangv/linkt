#include "string_ref.hpp"
#include "logger.hpp"

#include <fstream>

GLOBAL_NAMESPACE

DEFINE_ERROR(stringref_error)

string fallback_string::use_fallback(const string& msg) const {
  if (fallback)
    return *fallback;
  throw stringref_error("Reference failed: " + msg + ". And no fallback was found");
}

string env_string::get() const {
  auto result = std::getenv(name.data());
  if (result == nullptr)
    return use_fallback("Environment variable not found: " + name);
  return string(result);
}

bool env_string::readonly() const {
  return true;
}

void env_string::set(string value) {
  setenv(name.data(), value.data(), true);
}

string file_string::get() const {
  std::ifstream ifs(path.data());
  if (!ifs.fail()) {
    string result(std::istreambuf_iterator<char>{ifs}, {});
    ifs.close();
    auto last_line = result.find_last_not_of("\r\n");
    result.erase(last_line + 1);
    return move(result);
  } else
    return use_fallback("Can't read file: " + path);
}

bool file_string::readonly() const {
  return true;
}

void file_string::set(string value) {
}

string color_string::get() const {
  return processor.operate(ref->get());
}


GLOBAL_NAMESPACE_END
