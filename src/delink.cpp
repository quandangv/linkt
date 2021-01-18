#include "parse_delink.hpp"
#include "logger.hpp"
#include "tstring.hpp"
#include "error.hpp"

#include <stdexcept>
#include <functional>
#include <sstream>
#include <cspace/colorspace.hpp>

GLOBAL_NAMESPACE

using namespace std;
constexpr char scope[] = "delink";

void delink(document& doc, str_errlist& err) {
  std::function<void(const string&, const string&, string_ref_p&)>
  delink_key = [&](const string& sec, const string& key, string_ref_p& value) {
    auto report_err = [&](const string& msg, string&& fallback) {
      err.emplace_back(sec + "." + key, msg);
      value = make_unique<const_ref>(move(fallback));
    };
    if (!value)
      return report_err("Null value detected, possibly due to cyclical referencing", "");

    string src;
    std::function<void(tstring&, string_ref_p&)> delink_ref;

    // Delink an arbitrary string into `value`. The main thing this does is handling string interpolations. Otherwise, it would call delink_ref
    auto delink_string = [&](tstring& str, string_ref_p& value) {
      size_t start, end;
      if (!find_enclosed(str, src, "${", "}", start, end)) {
        // There is no token inside the string, it's a normal string
        value = make_unique<const_ref>(str);
      } else if (start == 0 && end == str.size()) {
        // There is a token inside, but string interpolation is unecessary
        str.erase_front(2);
        str.erase_back();
        delink_ref(str, value);
      } else {
        // String interpolation
        stringstream ss;
        auto newval = make_unique<string_interpolate_ref>();
        do {
          // Write the part we have moved past to get the token, to the base string
          ss << substr(str, 0, start);
          // Mark the position of the token in the base string
          newval->interpolator.positions.push_back(ss.tellp());

          // Make string_ref from the token
          auto token = str.interval(start + 2, end - 1);
          newval->replacements.list.emplace_back();
          delink_ref(token, newval->replacements.list.back());

          str.erase_front(end);
        } while (find_enclosed(str, src, "${", "}", start, end));
        ss << str;
        newval->interpolator.base = ss.str();
        value = move(newval);
      }
    };

    // Delink the reference represented by `str`, into `value`
    // The reference is the string between '${' and '}'
    delink_ref = [&](tstring& str, string_ref_p& value) {
      // Parse the fallback part of the reference string and remove it from consideration.
      auto take_fallback = [&](string_ref_p& fallback) {
        if (auto fb_str = cut_back(str, '?'); !fb_str.untouched())
          delink_string(trim_quotes(fb_str), fallback);
      };
      // Finish the intialization for the types derived from meta_ref
      auto make_meta_ref = [&]<typename T>(unique_ptr<T>&& ptr) {
        take_fallback(ptr->fallback);
        // Set the value field
        delink_string(trim_quotes(str), ptr->value);
        value = move(ptr);
      };
      if (auto ref_type= cut_front(str, ':'); !ref_type.untouched()) {
        // Determine the type of reference
        if (ref_type == "file") {
          make_meta_ref(std::make_unique<file_ref>());
        } else if (ref_type == "cmd") {
          make_meta_ref(std::make_unique<cmd_ref>());
        } else if (ref_type == "env") {
          make_meta_ref(std::make_unique<env_ref>());
        } else if (ref_type == "color") {
          // Color reference
          auto newval = std::make_unique<color_ref>();
          // Parse the modification part
          if (auto mod_str = cut_front(str, ';'); !mod_str.untouched()) {
            if (auto colorspace = cut_front(mod_str, ':'); !colorspace.untouched())
              newval->processor.inter = cspace::stospace(trim(colorspace));
            newval->processor.add_modification(mod_str);
          }
          make_meta_ref(move(newval));
        } else
          report_err("Unsupported reference type: " + ref_type, move(src));
      } else {
        // Local reference
        string_ref_p fallback;
        take_fallback(fallback);

        // Parse the referenced section and key
        string new_sec, new_key;
        if (auto sec_str = cut_front(str, '.'); !sec_str.untouched()) {
          new_sec = trim(sec_str);
        } else new_sec = sec;
        new_key = trim(str);

        // Look for the referenced key
        if (auto index = doc.find(new_sec, new_key); index) {
          // Found it
          auto& ref_val = doc.values[*index];
          value.reset();

          // Check for cyclical linking
          if (!ref_val) {
            report_err("Cyclical referencing detected", move(src));
          } else {
            delink_key(new_sec, new_key, ref_val);

            // Don't make a local_ref to another local_ref, reference directly to its source
            if (auto local = dynamic_cast<local_ref*>(ref_val.get()); local != nullptr)
              value = std::make_unique<local_ref>(local->ref);
            else
              value = std::make_unique<local_ref>(ref_val);
          }
        } else if (fallback) {
          value = move(fallback);
        } else
          report_err("Referenced key doesn't exist: " + new_sec + "." + new_key, move(src));
      }
    };
    // Skip if the original value is not a onetime_ref, which means it has already been delinked
    if (auto onetime = dynamic_cast<onetime_ref*>(value.get()); onetime != nullptr) {
      src = onetime->get_onetime();
      tstring str(src);
      delink_string(str, value);
    }
  };
  for(auto& seckey : doc.map) {
    auto& section = seckey.second;
    for(auto& keyval : section) {
      delink_key(seckey.first, keyval.first, doc.values[keyval.second]);
    }
  }
}

GLOBAL_NAMESPACE_END
