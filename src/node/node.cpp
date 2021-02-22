#include "node.hpp"
#include "container.hpp"
#include "common.hpp"

#include <fstream>
#include <cstdlib>
#include <array>

NAMESPACE(lini::node)

string address_ref::get() const {
  auto result = ancestor.get_child(path);
  return result ? *result : use_fallback("Referenced key doesn't exist");
}

bool address_ref::set(const string& val) {
  auto src = ancestor.get_child_ptr(path);
  settable* target = dynamic_cast<settable*>(*src ? src->get() : fallback ? fallback.get() : nullptr);
  if (target)
    return target->set(val);
  return false;
}

string ref::get() const {
  if (src && *src) {
    return (*src)->get();
  }
  return use_fallback("Referenced key doesn't exist");
}

bool ref::set(const string& val) {
  settable* target = dynamic_cast<settable*>(*src ? src->get() : fallback ? fallback.get() : nullptr);
  if (target)
    return target->set(val);
  return false;
}

string defaultable::use_fallback(const string& msg) const {
  if (fallback)
    return fallback->get();
  throw error("Reference failed: " + msg + ". And no fallback was found");
}

string env::get() const {
  auto result = getenv(value->get().data());
  if (result == nullptr)
    return use_fallback("Environment variable not found: " + value->get());
  return string(result);
}

bool env::set(const string& newval) {
  setenv(value->get().data(), newval.data(), true);
  return true;
}

string file::get() const {
  std::ifstream ifs(value->get().data());
  if (ifs.fail())
    return use_fallback("Can't read file: " + value->get());
  string result(std::istreambuf_iterator<char>{ifs}, {});
  ifs.close();
  auto last_line = result.find_last_not_of("\r\n");
  result.erase(last_line + 1);
  return move(result);
}

bool file::set(const string& content) {
  std::ofstream ofs(value->get().data(), std::ios_base::trunc);
  if (ofs.fail())
    return false;
  ofs << content;
  ofs.close();
  return true;
}

string color::get() const {
  try {
    auto result = processor.operate(value->get());
    if (result.empty() && fallback)
      return fallback->get();
    return result;
  } catch(const std::exception& e) {
    return use_fallback("Color processing failed, due to: " + string(e.what()));
  }
}

string cmd::get() const {
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

string map::get() const {
  auto str = value ? value->get() : use_fallback("Value key doesn't exist");
  size_t remaining;
  auto num =  std::stof(str, &remaining);
  if (remaining != str.size())
    throw std::invalid_argument("value is not a number");
  return std::to_string(to_min + to_range/from_range*(num - from_min));
}

NAMESPACE_END
