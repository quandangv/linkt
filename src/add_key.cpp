#include "add_key.hpp"
#include "common.hpp"

#include <functional>

GLOBAL_NAMESPACE

void add_key(document& doc, const string& section, const string& key, string& raw, tstring pos) {
  std::function<string_ref_p(tstring&)> parse_ref;

  // Delink an arbitrary string into `value`. The main thing this does is handling string interpolations. Otherwise, it would call delink_ref
  auto parse_string = [&](tstring& str)->string_ref_p {
    size_t start, end;
    if (!find_enclosed(str, raw, "${", "{", "}", start, end)) {
      // There is no token inside the string, it's a normal string
      return make_unique<const_ref>(str);
    } else if (start == 0 && end == str.size()) {
      // There is a token inside, but string interpolation is unecessary
      str.erase_front(2);
      str.erase_back();
      return parse_ref(str);
    } else {
      // String interpolation
      stringstream ss;
      auto newval = make_unique<string_interpolate_ref>();
      do {
        // Write the part we have moved past to get the token, to the base string
        ss << substr(str, 0, start);

        // Make string_ref from the token
        auto token = str.interval(start + 2, end - 1);
        auto value = parse_ref(token);
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
  };

  parse_ref = [&](tstring& str)->string_ref_p {
    auto take_fallback = [&](string_ref_p& fallback) {
      if (auto fb_str = cut_back(str, '?'); !fb_str.untouched())
        fallback = parse_string(trim_quotes(fb_str));
    };
    auto make_meta_ref = [&]<typename T>(unique_ptr<T>&& ptr) {
      take_fallback(ptr->fallback);
      ptr->value = parse_string(trim_quotes(str));
      if (!ptr->value)
        throw document::error("Invalid content for this type of reference");
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
        if (auto new_key = cut_front(str, '='); !new_key.empty()) {
          LG_DBUG("new-key: " <<new_key)
          trim(new_key);
          if (auto new_section = cut_front(new_key, '.'); !new_section.untouched()) {
            add_key(doc, new_section, new_key, raw, trim_quotes(str));
            return {};
          }
        }
        throw document::error("Invalid assigned key name");
      } else
      throw document::error("Unsupported reference type: " + ref_type);
    } else {
      string_ref_p fallback;
      take_fallback(fallback);
      if (auto new_section = cut_front(str, '.'); !new_section.untouched()) {
        return make_unique<local_ref>(doc.add(trim(new_section), trim(str)), move(fallback));
      } else
        throw document::error("Missing section");
    }
  };

  auto value = parse_string(pos);
  if (!value) return;
  if (auto place = doc.add(section, key); !*place) {
    *place = move(value);
  } else
    throw document::error("Duplicate key: " + key);
}

GLOBAL_NAMESPACE_END
