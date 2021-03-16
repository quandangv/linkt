#include "node.hpp"
#include "wrapper.hpp"
#include "parse.hpp"
#include "common.hpp"
#include "token_iterator.hpp"

#include <fstream>
#include <cstdlib>
#include <array>

NAMESPACE(node)

meta::meta(const base_s& value, const base_s& fallback) : with_fallback(fallback), value(value) {
  if (!value) THROW_ERROR(required_field_null, "meta::meta");
}

color::operator string() const {
  try {
    auto result = processor.operate(value->get());
    return result.empty() && fallback ? fallback->get() : result;
  } catch(const std::exception& e) {
    return use_fallback("Color processing failed, due to: "s + e.what());
  }
}

base_s color::clone(clone_context& context) const {
  auto result = meta::copy<color>(context);
  if (context.optimize && is_fixed(result->value))
    return std::make_shared<plain<string>>(operator string());
  result->processor = processor;
  return result;
}

env::operator string() const {
  auto result = getenv(value->get().data());
  return string(result ?: use_fallback("Environment variable not found: " + value->get()));
}

bool env::set(const string& newval) {
  setenv(value->get().data(), newval.data(), true);
  return true;
}

base_s env::clone(clone_context& context) const {
  return meta::copy<env>(context);
}

file::operator string() const {
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

base_s file::clone(clone_context& context) const {
  return meta::copy<file>(context);
}

cmd::operator string() const {
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

base_s cmd::clone(clone_context& context) const {
  return meta::copy<cmd>(context);
}

save::operator string() const {
  auto str = value->get();
  auto sep = str.rfind('\n');
  string result;
  if (sep == string::npos) {
    result = str;
  } else {
    result = str.substr(sep + 1);
    str.erase(sep);
  }
  if (auto conv_target = std::dynamic_pointer_cast<settable<string>>(target);
      !conv_target || !conv_target->set(str))
    THROW_ERROR(node, "save: Can't set value to target");
  return result;
}

base_s save::clone(clone_context& context) const {
  auto result = std::make_shared<save>();
  result->value = value->checked_clone(context, "save::clone");
  result->target = target->checked_clone(context, "save::clone");
  return result;
}

cache::operator string() const {
  if (auto now = std::chrono::steady_clock::now(); now > cache_expire) {
    cache_str = source->get();
    cache_expire = now + std::chrono::milliseconds(force_parse_ulong(duration_ms->get()));
  }
  return cache_str;
}

base_s cache::clone(clone_context& context) const {
  auto result = std::make_shared<cache>();
  result->source = source->checked_clone(context, "cache::clone");
  result->duration_ms = duration_ms->checked_clone(context, "cache::clone");
  result->cache_str = cache_str;
  result->cache_expire = cache_expire;
  return result;
}

array_cache::operator string() const {
  auto str = source->get();
  auto index = *(parse_ulong(str.data(), str.size()) ?: THROW_ERROR(parse, "Expected numeric value: " + str));
  return get(index);
}

string array_cache::get(size_t index) const {
  if (index >=cache_arr->size())
    THROW_ERROR(node, "Index larger than cache maximum: " + std::to_string(index) + " > " + std::to_string(cache_arr->size() - 1));
  if (auto& result = cache_arr->operator[](index); result.empty()) {
    return result = calculator->get();
  } else return result;
}

base_s array_cache::clone(clone_context& context) const {
  auto result = std::make_shared<array_cache>();
  result->source = source->checked_clone(context, "array_cache::clone");
  result->calculator = calculator->checked_clone(context, "array_cache::clone");
  result->cache_arr = cache_arr;
  return result;
}

map::map(base_s value) : value(value) {
  if (!value) THROW_ERROR(required_field_null, "meta::meta");
}

map::operator float() const {
  auto str = value->get();
  size_t remaining;
  auto num =  std::stof(str, &remaining);
  return remaining != str.size() ? THROW_ERROR(node, "value is not a number") :
      to_min + to_range/from_range*(num - from_min);
}

base_s map::clone(clone_context& context) const {
  auto result = std::make_shared<map>(value->clone(context));
  if (context.optimize && is_fixed(result->value))
      return std::make_shared<plain<string>>(operator string());

  result->from_min = from_min;
  result->from_range = from_range;
  result->to_min = to_min;
  result->to_range = to_range;
  return result;
}

clock::operator int() const {
  auto unlooped = (std::chrono::steady_clock::now() - zero_point) / tick_duration;
  return unlooped % loop;
}

base_s clock::clone(clone_context&) const {
  auto result = std::make_shared<clock>();
  result->tick_duration = tick_duration;
  result->loop = loop;
  result->zero_point = zero_point;
  return result;
}

NAMESPACE_END
