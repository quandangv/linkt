#include "string_ref.hpp"
#include "logger.hpp"
#include "execstream.hpp"

#include <fstream>
#include <cstdlib>
#include <array>

GLOBAL_NAMESPACE

using namespace std;
DEFINE_ERROR(stringref_error)

string fallback_ref::use_fallback(const string& msg) const {
  if (fallback)
    return fallback->get();
  throw stringref_error("Reference failed: " + msg + ". And no fallback was found");
}

string env_ref::get() const {
  auto result = getenv(name.data());
  if (result == nullptr)
    return use_fallback("Environment variable not found: " + name);
  return string(result);
}

bool env_ref::readonly() const {
  return true;
}

void env_ref::set(string value) {
  setenv(name.data(), value.data(), true);
}

string file_ref::get() const {
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

bool file_ref::readonly() const {
  return true;
}

void file_ref::set(string value) {
}

string color_ref::get() const {
  return processor.operate(ref->get());
}

string cmd_ref::get() const {
  string result;
  try {
    execstream exec(name.data(), execstream::type_out);
    result = exec.readall();
    if (auto exitstat = WEXITSTATUS(exec.close()); exitstat)
      use_fallback("Process exited with status " + to_string(exitstat) + ": " + name);
  } catch (const exception& e) {
    use_fallback("Can't start process due to: " + string(e.what()));
  }
  auto last_line = result.find_last_not_of("\r\n");
  result.erase(last_line + 1);
  return result;
}

GLOBAL_NAMESPACE_END
