#include "node.hpp"
#include "wrapper.hpp"
#include "common.hpp"
#include "execstream.hpp"
#include "token_iterator.hpp"

#include <fstream>
#include <cstdlib>
#include <array>

NAMESPACE(node)

string color::get() const {
  try {
    auto result = processor.operate(value->get());
    return result.empty() && fallback ? fallback->get() : result;
  } catch(const std::exception& e) {
    return use_fallback("Color processing failed, due to: "s + e.what());
  }
}

base_p color::clone(clone_context& context) const {
  auto result = std::make_shared<color>();
  if (!value)
    context.report_error("Color: value is empty");

  result->value = value->clone(context);
  if (context.optimize && is_fixed(result->value))
    return std::make_shared<plain>(get());

  result->processor = processor;
  if (fallback)
    result->fallback = fallback->clone(context);
  return result;
}

string env::get() const {
  auto result = getenv(value->get().data());
  return string(result ?: use_fallback("Environment variable not found: " + value->get()));
}

bool env::set(const string& newval) {
  setenv(value->get().data(), newval.data(), true);
  return true;
}

base_p env::clone(clone_context& context) const {
  return meta::copy(std::make_shared<env>(), context);
}

string file::get() const {
  std::ifstream ifs(value->get().data());
  if (ifs.fail())
    return use_fallback("Can't read file: " + value->get());

  string result(std::istreambuf_iterator<char>{ifs}, {});
  ifs.close();
  return result.erase(result.find_last_not_of("\r\n") + 1);
}

bool file::set(const string& content) {
  std::ofstream ofs(value->get().data(), std::ios_base::trunc);
  if (ofs.fail())
    return false;
  ofs << content;
  ofs.close();
  return true;
}

base_p file::clone(clone_context& context) const {
  return meta::copy(std::make_shared<file>(), context);
}

string cmd::get() const {
  string result;
  try {
    auto file = popen((value->get() + string(" 2>/dev/null")).data(), "r");
    std::array<char, 128> buf;
    while (fgets(buf.data(), 128, file) != nullptr)
      result += buf.data();
    pclose(file);
  } catch (const std::exception& e) {
    use_fallback("Encountered error: "s + e.what());
  }
  auto last_line = result.find_last_not_of("\r\n");
  result.erase(last_line + 1);
  return result;
}

base_p cmd::clone(clone_context& context) const {
  return meta::copy(std::make_shared<cmd>(), context);
}

string map::get() const {
  auto str = value ? value->get() : use_fallback("Value key doesn't exist");
  size_t remaining;
  auto num =  std::stof(str, &remaining);
  return remaining != str.size() ? use_fallback("value is not a number") :
      std::to_string(to_min + to_range/from_range*(num - from_min));
}

base_p map::clone(clone_context& context) const {
  auto result = std::make_shared<map>();
  if (value)
    result->value = value->clone(context);
  if (context.optimize && is_fixed(result->value))
      return std::make_shared<plain>(get());

  result->from_min = from_min;
  result->from_range = from_range;
  result->to_min = to_min;
  result->to_range = to_range;
  if (fallback)
    result->fallback = fallback->clone(context);
  return result;
}

NAMESPACE_END
