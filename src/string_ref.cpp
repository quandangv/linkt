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
  auto result = getenv(value->get().data());
  if (result == nullptr)
    return use_fallback("Environment variable not found: " + value->get());
  return string(result);
}

bool env_ref::readonly() const {
  return false;
}

void env_ref::set(const string& newval) {
  setenv(value->get().data(), newval.data(), true);
}

string file_ref::get() const {
  ifstream ifs(value->get().data());
  if (ifs.fail())
    return use_fallback("Can't read file: " + value->get());
  string result(istreambuf_iterator<char>{ifs}, {});
  ifs.close();
  auto last_line = result.find_last_not_of("\r\n");
  result.erase(last_line + 1);
  return move(result);
}

bool file_ref::readonly() const {
  return false;
}

void file_ref::set(const string& content) {
  ofstream ofs(value->get().data(), ios_base::trunc);
  if (ofs.fail())
    throw stringref_error("Can't write to file: " + value->get());
  ofs << content;
  ofs.close();
}

string color_ref::get() const {
  try {
    auto result = processor.operate(value->get());
    if (result.empty() && fallback)
      return fallback->get();
    return result;
  } catch(const exception& e) {
    return use_fallback("Color processing failed, due to: " + string(e.what()));
  }
}

string cmd_ref::get() const {
  string result;
  try {
    execstream exec(value->get().data(), execstream::type_out);
    result = exec.readall();
    if (auto exitstat = WEXITSTATUS(exec.close()); exitstat)
      use_fallback("Process exited with status " + to_string(exitstat) + ": " + value->get());
  } catch (const exception& e) {
    use_fallback("Can't start process due to: " + string(e.what()));
  }
  auto last_line = result.find_last_not_of("\r\n");
  result.erase(last_line + 1);
  return result;
}

GLOBAL_NAMESPACE_END
