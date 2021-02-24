#include "node.hpp"
#include "container.hpp"
#include "common.hpp"

#include <fstream>
#include <cstdlib>
#include <array>

NAMESPACE(lini::node)

string address_ref::get() const {
  auto result = ancestor.get_child(path);
  return result ? *result : use_fallback("Referenced path doesn't exist: " + path);
}

base_p address_ref::get_source() const {
  return ancestor.get_child_ptr(path);
}

bool address_ref::set(const string& val) {
  auto src = ancestor.get_child_ptr(path);
  auto target = dynamic_cast<settable*>(src ? src.get() : fallback ? fallback.get() : nullptr);
  if (target)
    return target->set(val);
  return false;
}

string hard_ref::get() const {
  if (src)
    return src->get();
  return use_fallback("Referenced key doesn't exist");
}

bool hard_ref::set(const string& val) {
  settable* target = dynamic_cast<settable*>(src ? src.get() : fallback ? fallback.get() : nullptr);
  if (target)
    return target->set(val);
  return false;
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

base_p color::clone(clone_handler handler) const {
  LG_DBUG("Clone color")
  auto result = std::make_shared<color>();
  result->processor = processor;
  return meta::copy(result, handler);
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

base_p env::clone(clone_handler handler) const {
  return meta::copy(std::make_shared<env>(), handler);
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

base_p file::clone(clone_handler handler) const {
  return meta::copy(std::make_shared<file>(), handler);
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

base_p cmd::clone(clone_handler handler) const {
  return meta::copy(std::make_shared<cmd>(), handler);
}

string map::get() const {
  auto str = value ? value->get() : use_fallback("Value key doesn't exist");
  size_t remaining;
  auto num =  std::stof(str, &remaining);
  if (remaining != str.size())
    throw std::invalid_argument("value is not a number");
  return std::to_string(to_min + to_range/from_range*(num - from_min));
}

base_p map::clone(clone_handler handler) const {
  auto result = std::make_shared<map>();
  result->from_min = from_min;
  result->from_range = from_range;
  result->to_min = to_min;
  result->to_range = to_range;
  return meta::copy(move(result), handler);
}

NAMESPACE_END
