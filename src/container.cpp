#include "container.hpp"
#include "common.hpp"
#include "document.hpp"
#include "token_iterator.hpp"

GLOBAL_NAMESPACE

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
      LG_INFO("document-get: failed due to value being null: " << path);
  } else
    LG_INFO("document-get: failed due to key not found: " << path);
  return {};
}

string container::get_child(const tstring& path, string&& fallback) const {
  if (auto result = get_child(path); result)
    return *result;
  return forward<string>(fallback);
}

string_ref& container::get_child_ref(const tstring& path) const {
  auto ptr = get_child_ptr(path);
  if (ptr && *ptr)
    return **ptr;
  throw string_ref::error("Key is empty");
}

bool container::set(const tstring& path, const string& value) {
  if (auto ptr = get_child_ptr(path); ptr) {
    if (auto settable_ref = dynamic_cast<settable*>(ptr->get()); settable_ref && !settable_ref->readonly()) {
      settable_ref->set(value);
      return true;
    }
  }
  return false;
}

string_ref_p2 addable::add(tstring path, string& raw, tstring value) {
  auto node = parse_string(raw, value);
  if (node)
    return add(path, move(node));
  return {};
}

string_ref_p2 addable::add(tstring path, string raw) {
  tstring value(raw);
  return add(path, raw, value);
}

string_ref_p addable::parse_ref(string& raw, tstring& str) {
  auto take_fallback = [&](string_ref_p& fallback) {
    if (auto fb_str = cut_back(str, '?'); !fb_str.untouched())
      fallback = parse_string(raw, trim_quotes(fb_str));
  };
  auto make_meta_ref = [&]<typename T>(std::unique_ptr<T>&& ptr) {
    take_fallback(ptr->fallback);
    ptr->value = parse_string(raw, trim_quotes(str));
    if (!ptr->value)
      throw error("Invalid content for this type of reference");
    return move(ptr);
  };
  auto ref_type = cut_front(str, ':');
  if (ref_type.untouched()) {
    string_ref_p fallback;
    take_fallback(fallback);
    auto ptr = get_child_ptr(str);
    if (!ptr)
      ptr = add(str, string_ref_p{});
    return std::make_unique<local_ref>(ptr, move(fallback));
  } else if (ref_type == "file"_ts) {
    return make_meta_ref(std::make_unique<file_ref>());
  } else if (ref_type == "cmd"_ts) {
    return make_meta_ref(std::make_unique<cmd_ref>());
  } else if (ref_type == "env"_ts) {
    return make_meta_ref(std::make_unique<env_ref>());
  } else if (ref_type == "map"_ts) {
    auto newval = std::make_unique<map_ref>();
    auto from = cut_front(str, ';');
    if (from.untouched())
      throw error("Expected 3 components separated by ';'");
    if (auto min = cut_front(from, ':'); !min.untouched())
      newval->from_min = convert<float, strtof>(trim(min));
    newval->from_range = convert<float, strtof>(trim(from));
    auto to = cut_front(str, ';');
    if (to.untouched())
      throw error("Expected 3 components separated by ';'");
    if (auto min = cut_front(to, ':'); !min.untouched())
      newval->to_min = convert<float, strtof>(trim(min));
    newval->to_range = convert<float, strtof>(trim(to));
    return make_meta_ref(move(newval));
  } else if (ref_type == "color"_ts) {
    auto newval = std::make_unique<color_ref>();
    if (auto mod_str = cut_front(str, ';'); !mod_str.untouched()) {
      if (auto space = cut_front(mod_str, ':'); !space.untouched())
        newval->processor.inter = cspace::stospace(trim(space));
      newval->processor.add_modification(mod_str);
    }
    return make_meta_ref(move(newval));
  } else if (ref_type == "key"_ts) {
    if (auto new_key = cut_front(str, '='); !new_key.untouched()) {
      add(trim(new_key), raw, trim_quotes(str));
      return {};
    }
    throw error("Missing assigned key name");
  } else if (ref_type == "doc"_ts) {
    auto subdoc = std::make_unique<document>();
    tstring line;
    while(!(line = get_token<';'>(str)).untouched()) {
      if (auto new_key = cut_front(line, '='); !new_key.untouched()) {
        subdoc->add(trim(new_key), raw, trim_quotes(line));
      } else
        throw error("Missing assigned key name");
    }
    return move(subdoc);
  } else
    throw error("Unsupported reference type: " + ref_type);
}

string_ref_p addable::parse_string(string& raw, tstring& str) {
  size_t start, end;
  if (!find_enclosed(str, raw, "${", "{", "}", start, end)) {
    // There is no token inside the string, it's a normal string
    return std::make_unique<const_ref>(str);
  } else if (start == 0 && end == str.size()) {
    // There is a token inside, but string interpolation is unecessary
    str.erase_front(2);
    str.erase_back();
    return parse_ref(raw, str);
  }
  // String interpolation
  std::stringstream ss;
  auto newval = std::make_unique<string_interpolate_ref>();
  do {
    // Write the part we have moved past to the base string
    ss << substr(str, 0, start);

    // Make string_ref from the token
    auto token = str.interval(start + 2, end - 1);
    auto value = parse_ref(raw, token);
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

GLOBAL_NAMESPACE_END
