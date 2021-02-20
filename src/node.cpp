#include "node.hpp"
#include "common.hpp"

#include <fstream>
#include <cstdlib>
#include <array>

NAMESPACE(lini)

string local_ref::get() const {
  if (ref && *ref) {
    return (*ref)->get();
  }
  return use_fallback("Referenced key doesn't exist");
}

bool local_ref::readonly() const {
  if (*ref) {
    if (auto settable_ref = dynamic_cast<settable*>(ref->get()); settable_ref) {
      return settable_ref->readonly();
    }
  } else if (fallback) {
    if (auto settable_ref = dynamic_cast<settable*>(fallback.get()); settable_ref) {
      return settable_ref->readonly();
    }
  }
  return false;
}

void local_ref::set(const string& val) {
  if (*ref) {
    if (auto settable_ref = dynamic_cast<settable*>(ref->get()); settable_ref) {
      settable_ref->set(val);
    }
  } else if (fallback) {
    if (auto settable_ref = dynamic_cast<settable*>(fallback.get()); settable_ref) {
      settable_ref->set(val);
    }
  }
}

string fallback_ref::use_fallback(const string& msg) const {
  if (fallback)
    return fallback->get();
  throw error("Reference failed: " + msg + ". And no fallback was found");
}

string env_ref::get() const {
  auto result = getenv(value->get().data());
  if (result == nullptr)
    return use_fallback("Environment variable not found: " + value->get());
  return string(result);
}

void env_ref::set(const string& newval) {
  setenv(value->get().data(), newval.data(), true);
}

string file_ref::get() const {
  std::ifstream ifs(value->get().data());
  if (ifs.fail())
    return use_fallback("Can't read file: " + value->get());
  string result(std::istreambuf_iterator<char>{ifs}, {});
  ifs.close();
  auto last_line = result.find_last_not_of("\r\n");
  result.erase(last_line + 1);
  return move(result);
}

void file_ref::set(const string& content) {
  std::ofstream ofs(value->get().data(), std::ios_base::trunc);
  if (ofs.fail())
    throw error("Can't write to file: " + value->get());
  ofs << content;
  ofs.close();
}

string color_ref::get() const {
  try {
    auto result = processor.operate(value->get());
    if (result.empty() && fallback)
      return fallback->get();
    return result;
  } catch(const std::exception& e) {
    return use_fallback("Color processing failed, due to: " + string(e.what()));
  }
}

string cmd_ref::get() const {
  string result;
  try {
    auto file = popen(value->get().data(), "r");
    std::array<char, 128> buf;
    while (fgets(buf.data(), 128, file) != nullptr)
      result += buf.data();
  } catch (const std::exception& e) {
    use_fallback("Encountered error: " + string(e.what()));
  }
  auto last_line = result.find_last_not_of("\r\n");
  result.erase(last_line + 1);
  return result;
}

string map_ref::get() const {
  auto str = value ? value->get() : use_fallback("Value key doesn't exist");
  size_t remaining;
  auto num =  std::stof(str, &remaining);
  if (remaining != str.size())
    throw std::invalid_argument("value is not a number");
  return std::to_string(to_min + to_range/from_range*(num - from_min));
}

NAMESPACE_END
