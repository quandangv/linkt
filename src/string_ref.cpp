#include "string_ref.hpp"
#include "logger.hpp"

#include <fstream>
#include <cstdlib>
#include <array>

GLOBAL_NAMESPACE

using namespace std;
DEFINE_ERROR(stringref_error)

string fallback_string::use_fallback(const string& msg) const {
  if (fallback)
    return fallback->get();
  throw stringref_error("Reference failed: " + msg + ". And no fallback was found");
}

string env_string::get() const {
  auto result = getenv(name.data());
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
  ifstream ifs(name.data());
  if (!ifs.fail()) {
    string result(istreambuf_iterator<char>{ifs}, {});
    ifs.close();
    auto last_line = result.find_last_not_of("\r\n");
    result.erase(last_line + 1);
    return move(result);
  } else
    return use_fallback("Can't read file: " + name);
}

bool file_string::readonly() const {
  return true;
}

void file_string::set(string value) {
}

string color_string::get() const {
  return processor.operate(ref->get());
}

string cmd_string::get() const {
  array<char, 128> buffer;
  string result;
  FILE* pipe = popen(name.data(), "r");
  if (!pipe)
    throw stringref_error("popen() failed!");
  while (fgets(buffer.data(), buffer.size(), pipe) != nullptr)
    result += buffer.data();
  if (auto exitstatus = WEXITSTATUS(pclose(pipe)); exitstatus)
    return use_fallback("Process failed with exit code: " + to_string(exitstatus));
  auto last_line = result.find_last_not_of("\r\n");
  result.erase(last_line + 1);
  return result;
}

GLOBAL_NAMESPACE_END
