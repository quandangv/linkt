#include "node.hpp"
#include "wrapper.hpp"
#include "common.hpp"
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
  auto result = meta::copy<color>(context);
  if (context.optimize && is_fixed(result->value))
    return std::make_shared<plain>(get());
  result->processor = processor;
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
  return meta::copy<env>(context);
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
  return meta::copy<file>(context);
}

string cmd::get() const {
  string result;
  try {
    auto file = popen((value->get() + string(" 2>/dev/null")).data(), "r");
    std::array<char, 128> buf;
    while (fgets(buf.data(), 128, file) != nullptr)
      result += buf.data();
    if (auto exit_code = WEXITSTATUS(pclose(file)))
      return use_fallback("Process produced exit code: " + std::to_string(exit_code));
  } catch (const std::exception& e) {
    return use_fallback("Encountered error: "s + e.what());
  }
  auto last_line = result.find_last_not_of("\r\n");
  result.erase(last_line + 1);
  return result;
}

base_p cmd::clone(clone_context& context) const {
  return meta::copy<cmd>(context);
}

string save::get() const {
  auto str = value->get();
  auto sep = str.rfind('\n');
  string result;
  if (sep == string::npos) {
    result = str;
  } else {
    result = str.substr(sep + 1);
    str.erase(sep);
  }
  if (auto conv_target = dynamic_cast<settable*>(target.get());
      !conv_target || !conv_target->set(str))
    THROW_ERROR(node, "save: Can't set value to target");
  return result;
}

base_p save::clone(clone_context& context) const {
  auto result = std::make_shared<save>();
  result->value = value->clone(context);
  result->target = target->clone(context);
  return result;
}

string cache::get() const {
  if (auto now = std::chrono::steady_clock::now(); now > cache_expire) {
    cache_str = source->get();
    cache_expire = now + cache_duration;
  }
  return cache_str;
}

base_p cache::clone(clone_context& context) const {
  auto result = std::make_shared<cache>();
  result->source = source->clone(context);
  result->cache_duration = cache_duration;
  return result;
}

string array_cache::get() const {
  auto str = source->get();
  auto index = *(parse_ulong(str.data(), str.size()) ?: THROW_ERROR(parse, "Expected numeric value: " + str));
  return get(index);
}

string array_cache::get(size_t index) const {
  if (index >=cache_arr->size())
    THROW_ERROR(node, "Index larger than cache maximum: " + std::to_string(index) + " > " + std::to_string(cache_arr->size() - 1));
  if (auto& result = cache_arr->operator[](index); result.empty()) {
    LG_DBUG("Get new");
    return result = calculator->get();
  } else return result;
}

base_p array_cache::clone(clone_context& context) const {
  auto result = std::make_shared<array_cache>();
  result->source = source->clone(context);
  result->calculator = calculator->clone(context);
  result->cache_arr = cache_arr;
  return result;
}

string map::get() const {
  try {
    auto str = value ? value->get() : use_fallback("Value key doesn't exist");
    size_t remaining;
    auto num =  std::stof(str, &remaining);
    return remaining != str.size() ? use_fallback("value is not a number") :
        std::to_string(to_min + to_range/from_range*(num - from_min));
  } catch (const std::exception& e) {
    return use_fallback("Linear mapping failed, due to: "s + e.what());
  }
}

base_p map::clone(clone_context& context) const {
  auto result = meta::copy<map>(context);
  if (context.optimize && is_fixed(result->value))
      return std::make_shared<plain>(get());

  result->from_min = from_min;
  result->from_range = from_range;
  result->to_min = to_min;
  result->to_range = to_range;
  return result;
}

string clock::get() const {
  auto unlooped = (std::chrono::steady_clock::now() - zero_point) / tick_duration;
  return std::to_string(unlooped % loop);
}

base_p clock::clone(clone_context&) const {
  auto result = std::make_shared<clock>();
  result->tick_duration = tick_duration;
  result->loop = loop;
  result->zero_point = zero_point;
  return result;
}

NAMESPACE_END
