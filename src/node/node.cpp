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
  if (context.optimize && is_fixed())
    return std::make_shared<plain<string>>(operator string());
  auto result = meta::copy<color>(context);
  result->processor = processor;
  return result;
}

bool color::is_fixed() const {
  return value->is_fixed();
}

std::shared_ptr<color> color::parse(parse_context& context, parse_preprocessed& prep) {
  auto result = std::make_shared<color>(parse_raw<string>(context, prep.tokens[prep.token_count - 1]), move(prep.fallback));
  if (prep.token_count > 2) {
    if (prep.token_count > 3)
      result->processor.inter = cspace::stospace(prep.tokens[1]);
    result->processor.add_modification(trim_quotes(prep.tokens[prep.token_count - 2]));
  }
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
  result->value = checked_clone<string>(value, context, "save::clone");
  result->target = checked_clone<string>(target, context, "save::clone");
  return result;
}

std::shared_ptr<save> save::parse(parse_context& context, parse_preprocessed& prep) {
  if (prep.token_count != 3)
    THROW_ERROR(parse, "save: Expected 2 components");
  auto result = std::make_shared<save>();
  result->target = std::make_shared<address_ref<string>>(context.get_current(), prep.tokens[1]);
  result->value = checked_parse_raw<string>(context, prep.tokens[2]);
  return result;
}

cache::operator string() const {
  if (auto now = std::chrono::steady_clock::now(); now > cache_expire) {
    cache_str = source->get();
    cache_expire = now + std::chrono::milliseconds(duration_ms->operator int());
  }
  return cache_str;
}

base_s cache::clone(clone_context& context) const {
  LG_DBUG("start clone");
  auto result = std::make_shared<cache>();
  result->source = checked_clone<string>(source, context, "cache::clone");
  LG_DBUG("start duration");
  result->duration_ms = checked_clone<int>(duration_ms, context, "cache::clone");
  LG_DBUG("end duration");
  result->cache_str = cache_str;
  result->cache_expire = cache_expire;
  LG_DBUG("end clone");
  return result;
}

std::shared_ptr<cache> cache::parse(parse_context& context, parse_preprocessed& prep) {
  if (prep.token_count != 3)
    THROW_ERROR(parse, "cache: Expected 2 components");
  auto result = std::make_shared<cache>();
  result->duration_ms = checked_parse_raw<int>(context, prep.tokens[1]);
  result->source = checked_parse_raw<string>(context, prep.tokens[2]);
  return result;
}

array_cache::operator string() const {
  return get(source->operator int());
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
  LG_DBUG((long)source.get());
  LG_DBUG((long)std::dynamic_pointer_cast<address_ref<int>>(source).get());
  LG_DBUG((long)std::dynamic_pointer_cast<ref<int>>(source->clone(context)).get());
  result->source = checked_clone<int>(source, context, "array_cache::clone");
  result->calculator = checked_clone<string>(calculator, context, "array_cache::clone");
  result->cache_arr = cache_arr;
  return result;
}

std::shared_ptr<array_cache> array_cache::parse(parse_context& context, parse_preprocessed& prep) {
  if (prep.token_count == 4) {
    auto result = std::make_shared<array_cache>();
    auto size = parse_ulong(prep.tokens[1].begin(), prep.tokens[1].size());
    if (size) {
      result->cache_arr = std::make_shared<std::vector<string>>(*size + 1);
      for (size_t i = 0; i < size; i++)
        result->cache_arr->emplace_back();
    } else {
      auto cache_base = address_ref<string>(context.get_parent(), prep.tokens[1]).get_source();
      if (auto cache = std::dynamic_pointer_cast<array_cache>(cache_base))
        result->cache_arr = cache->cache_arr;
      else THROW_ERROR(parse, "1st argument must be the size of the cache or a parent path to another array_cache: " + context.raw);
    }
    result->source = checked_parse_raw<int>(context, prep.tokens[2]);
    result->calculator = checked_parse_raw<string>(context, prep.tokens[3]);
    return result;
  } else
    THROW_ERROR(parse, "array_cache: Expected 3 components");
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
  if (context.optimize && is_fixed())
      return std::make_shared<plain<string>>(operator string());
  auto result = std::make_shared<map>(value->clone(context));
  result->from_min = from_min;
  result->from_range = from_range;
  result->to_min = to_min;
  result->to_range = to_range;
  return result;
}

std::shared_ptr<map> map::parse(parse_context& context, parse_preprocessed& prep) {
  if (prep.token_count != 4)
    THROW_ERROR(parse, "map: Expected 3 components");
  auto result = std::make_shared<map>(parse_raw<string>(context, prep.tokens[prep.token_count - 1]));
  if (auto min = cut_front(prep.tokens[1], ':'); !min.untouched())
    result->from_min = convert<float, strtof>(min);
  result->from_range = convert<float, strtof>(prep.tokens[1]) - result->from_min;

  if (auto min = cut_front(prep.tokens[2], ':'); !min.untouched())
    result->to_min = convert<float, strtof>(min);
  result->to_range = convert<float, strtof>(prep.tokens[2]) - result->to_min;
  return result;
}

bool map::is_fixed() const {
  return value->is_fixed();
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

std::shared_ptr<clock> clock::parse(parse_context&, parse_preprocessed& prep) {
  if (prep.token_count != 4)
    THROW_ERROR(parse, "clock: Expected 3 components");
  auto result = std::make_shared<clock>();
  result->tick_duration = std::chrono::milliseconds(
      node::parse<unsigned long>(prep.tokens[1].begin(), prep.tokens[1].size()));
  result->loop = node::parse<unsigned long>(prep.tokens[2].begin(), prep.tokens[2].size());
  result->zero_point = steady_time(
      result->tick_duration * node::parse<unsigned long>(prep.tokens[3].begin(), prep.tokens[3].size()));
  return result;
}

NAMESPACE_END
