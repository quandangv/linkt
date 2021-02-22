#include "container.hpp"
#include "common.hpp"
#include "token_iterator.hpp"

#include <array>

NAMESPACE(lini::node)

bool container::has_child(const tstring& path) const {
  auto ptr = get_child_ptr(path);
  return ptr && *ptr;
}

std::optional<string> container::get_child(const tstring& path) const {
  if (auto ptr = get_child_ptr(path); ptr) {
    if (auto& value = *ptr; value) {
      try {
        return value->get();
      } catch(const std::exception& e) {
        throw error("Exception while retrieving value of '" + path + "': " + e.what());
      }
    } else
      LG_INFO("contriner-get_child: failed due to value being null: " << path);
  } else
    LG_INFO("container-get_child: failed due to key not found: " << path);
  return {};
}

string container::get_child(const tstring& path, string&& fallback) const {
  if (auto result = get_child(path); result)
    return *result;
  return forward<string>(fallback);
}

base& container::get_child_ref(const tstring& path) const {
  auto ptr = get_child_ptr(path);
  if (ptr && *ptr)
    return **ptr;
  throw base::error("Key is empty");
}

bool container::set(const tstring& path, const string& value) {
  if (auto ptr = get_child_ptr(path); ptr)
    if (auto target = dynamic_cast<settable*>(ptr->get()); target)
      return target->set(value);
  return false;
}

base_pp addable::add(tstring path, string& raw, tstring value) {
  auto node = parse_string(raw, value, [&](tstring& ts, base_p&& fallback) { return make_ref(ts, move(fallback)); });
  if (node)
    return add(path, move(node));
  return {};
}

base_pp addable::add(tstring path, string raw) {
  tstring value(raw);
  return add(path, raw, value);
}

base_p addable::make_ref(const tstring& ts, base_p&& fallback) {
  auto ptr = get_child_ptr(ts);
  if (!ptr)
    ptr = add(ts, base_p{});
  return std::make_unique<ref>(ptr, move(fallback));
}

base_p parse(string& raw, tstring& str, ref_maker rmaker) {
  base_p fallback;
  if (auto fb_str = cut_back(str, '?'); !fb_str.untouched())
    fallback = parse_string(raw, trim_quotes(fb_str), rmaker);

  std::array<tstring, 5> tokens;
  auto token_count = fill_tokens(str, tokens);
  auto make_meta = [&]<typename T>(std::unique_ptr<T>&& ptr) {
    ptr->fallback = move(fallback);
    ptr->value = parse_string(raw, trim_quotes(tokens[token_count - 1]), rmaker);
    return move(ptr);
  };
  if (token_count == 0)
    throw addable::error("parse: Empty reference conent");
  if (token_count == 1) {
    return rmaker(tokens[0], move(fallback));
  } else if (tokens[0] == "file"_ts) {
    tokens[token_count - 1].merge(tokens[1]);
    return make_meta(std::make_unique<file>());
    
  } else if (tokens[0] == "cmd"_ts) {
    tokens[token_count - 1].merge(tokens[1]);
    return make_meta(std::make_unique<cmd>());

  } else if (tokens[0] == "env"_ts) {
    if (token_count != 2)
      throw addable::error("parse: Expected 2 components");
    return make_meta(std::make_unique<env>());

  } else if (tokens[0] == "map"_ts) {
    if (token_count != 4)
      throw addable::error("parse.map: Expected 3 components");
    auto newval = std::make_unique<map>();
    if (auto min = cut_front(tokens[1], ':'); !min.untouched())
      newval->from_min = convert<float, strtof>(min);
    newval->from_range = convert<float, strtof>(tokens[1]) - newval->from_min;

    if (auto min = cut_front(tokens[2], ':'); !min.untouched())
      newval->to_min = convert<float, strtof>(min);
    newval->to_range = convert<float, strtof>(tokens[2]) - newval->to_min;
    return make_meta(move(newval));

  } else if (tokens[0] == "color"_ts) {
    auto newval = std::make_unique<color>();
    if (token_count > 2) {
      if (token_count > 3)
        newval->processor.inter = cspace::stospace(tokens[1]);
      newval->processor.add_modification(tokens[token_count - 2]);
    }
    return make_meta(move(newval));

  } else
    throw addable::error("Unsupported reference type: " + tokens[0]);
}

base_p parse_string(string& raw, tstring& str, ref_maker rmaker) {
  size_t start, end;
  if (!find_enclosed(str, raw, "${", "{", "}", start, end)) {
    // There is no token inside the string, it's a normal string
    return std::make_unique<plain>(str);
  } else if (start == 0 && end == str.size()) {
    // There is a single token inside, interpolation is unecessary
    str.erase_front(2);
    str.erase_back();
    return parse(raw, str, rmaker);
  }
  // String interpolation
  std::stringstream ss;
  auto newval = std::make_unique<string_interpolate>();
  do {
    // Write the part we have moved past to the base string
    ss << substr(str, 0, start);

    // Make node from the token
    auto token = str.interval(start + 2, end - 1);
    auto value = parse(raw, token, rmaker);
    if (value) {
      // Mark the position of the token in the base string
      newval->spots.push_back({int(ss.tellp()), "", move(value)});
    }
    str.erase_front(end);
  } while (find_enclosed(str, raw, "${", "{", "}", start, end));
  ss << str;
  newval->base = ss.str();
  return move(newval);
}

NAMESPACE_END
