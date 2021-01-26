#include "document.hpp"
#include "common.hpp"
#include "tstring.hpp"
#include "add_key.hpp"
#include "token_iterator.hpp"

#include <sstream>
#include <iostream>

GLOBAL_NAMESPACE

using namespace std;

string_ref_p2 document::get_child_ptr(tstring path) const {
  if (auto immediate_path = cut_front(trim(path), '.'); !immediate_path.untouched()) {
    if (auto iterator = map.find(immediate_path); iterator != map.end() && iterator->second)
      if (auto child = dynamic_cast<container*>(iterator->second->get()); child)
        return child->get_child_ptr(path);
  } else if (auto iterator = map.find(path); iterator != map.end())
    return iterator->second;
  return {};
}

string_ref_p2 document::add(tstring path, string_ref_p&& value, bool dup) {
  if (auto immediate_path = cut_front(path, '.'); !immediate_path.untouched()) {
    // This isn't the final node of the path
    LG_DBUG("document-add: going through child: " + immediate_path);
    auto& ptr = map[immediate_path];
    if (!ptr) {
      ptr = make_shared<string_ref_p>(make_unique<document>());
      LG_DBUG("document-add: adding new child: " + immediate_path);
    }
    auto& child_ptr = *ptr;
    if (!child_ptr) {
      LG_DBUG("document-add: adding child to empty ptr: " + immediate_path);
      auto tmp = make_unique<document>();
      auto res = tmp->add(path, move(value), dup);
      child_ptr = move(tmp);
      return res;
    } if (auto child = dynamic_cast<addable*>(child_ptr.get()); child)
      return child->add(path, move(value), dup);
    throw error("Child " + immediate_path + " already exists but can't be added to");
  } else {
    // This is the final node of the path
    auto& place = map[path];
    LG_DBUG("document-add: key: " + path);
    if (!place) {
      LG_DBUG("document-add: add shared_ptr for key: " + path);
      place = make_shared<string_ref_p>();
    } else if (*place) {
      if (dup)
        return place;
      throw error("Duplicate key: " + static_cast<string>(path));
    }
    *place = move(value);
    LG_DBUG("document-add: afterward: " + tstring(map[path] ? "true" : "false"));
    return place;
  }
}

void document::add(tstring path, string& raw, tstring value) {
  auto node = parse_string(raw, value);
  if (node)
    add(path, move(node));
}

void document::add(tstring path, string raw) {
  LG_DBUG("document-add: start 1");
  tstring value(raw);
  add(path, raw, value);
}

string_ref_p document::parse_ref(string& raw, tstring& str) {
  auto take_fallback = [&](string_ref_p& fallback) {
    if (auto fb_str = cut_back(str, '?'); !fb_str.untouched())
      fallback = parse_string(raw, trim_quotes(fb_str));
  };
  auto make_meta_ref = [&]<typename T>(unique_ptr<T>&& ptr) {
    take_fallback(ptr->fallback);
    ptr->value = parse_string(raw, trim_quotes(str));
    if (!ptr->value)
      throw error("Invalid content for this type of reference");
    return move(ptr);
  };
  if (auto ref_type= cut_front(str, ':'); !ref_type.untouched()) {
    if (ref_type == "file") {
      return make_meta_ref(std::make_unique<file_ref>());
    } else if (ref_type == "cmd") {
      return make_meta_ref(std::make_unique<cmd_ref>());
    } else if (ref_type == "env") {
      return make_meta_ref(std::make_unique<env_ref>());
    } else if (ref_type == "color") {
      auto newval = std::make_unique<color_ref>();
      if (auto mod_str = cut_front(str, ';'); !mod_str.untouched()) {
        if (auto colorspace = cut_front(mod_str, ':'); !colorspace.untouched())
          newval->processor.inter = cspace::stospace(trim(colorspace));
        newval->processor.add_modification(mod_str);
      }
      return make_meta_ref(move(newval));
    } else if (ref_type == "key") {
      if (auto new_key = cut_front(str, '='); !new_key.untouched()) {
        trim(new_key);
        LG_DBUG("parse-key: add key: " + new_key);
        add(new_key, raw, trim_quotes(str));
        return {};
      }
      throw error("Missing assigned key name");
    } else if (ref_type == "doc") {
      auto subdoc = make_unique<document>();
      tstring line;
      while(!(line = get_token<';'>(str)).untouched()) {
        LG_DBUG("Before cut: " << line)
        if (auto new_key = cut_front(line, '='); trim(new_key).empty()) {
          throw new error("Missing assigned key name");
        } else
          subdoc->add(new_key, raw, trim_quotes(line));
      }
      return move(subdoc);
    } else
      throw error("Unsupported reference type: " + ref_type);
  } else {
    string_ref_p fallback;
    take_fallback(fallback);
    auto ptr = get_child_ptr(str);
    if (!ptr)
      ptr = add(str, string_ref_p{});
    return make_unique<local_ref>(ptr, move(fallback));
  }
}

string_ref_p document::parse_string(string& raw, tstring& str) {
  size_t start, end;
  if (!find_enclosed(str, raw, "${", "{", "}", start, end)) {
    // There is no token inside the string, it's a normal string
    return make_unique<const_ref>(str);
  } else if (start == 0 && end == str.size()) {
    // There is a token inside, but string interpolation is unecessary
    str.erase_front(2);
    str.erase_back();
    return parse_ref(raw, str);
  }
  // String interpolation
  stringstream ss;
  auto newval = make_unique<string_interpolate_ref>();
  do {
    // Write the part we have moved past to the base string
    ss << substr(str, 0, start);

    // Make string_ref from the token
    auto token = str.interval(start + 2, end - 1);
    auto value = parse_ref(raw, token);
    if (value) {
      // Mark the position of the token in the base string
      newval->positions.push_back(ss.tellp());
      newval->replacements.list.emplace_back(move(value));
    }
    str.erase_front(end);
  } while (find_enclosed(str, raw, "${", "{", "}", start, end));
  ss << str;
  newval->base = ss.str();
  return move(newval);
}

GLOBAL_NAMESPACE_END
