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
    auto report_err = [&](const string& msg) { err.emplace_back(sec + "." + key, msg); };
    if (!value)
      return report_err("Null value detected, possibly due to cyclical referencing");

    // Skip if the original value is not a onetime_ref, which means it has already been delinked
    string src;
    if (auto onetime = dynamic_cast<onetime_ref*>(value.get()); onetime != nullptr) {
      src = onetime->get_onetime();
    } else return;
    tstring str(src);

    std::function<void(tstring&, string_ref_p&)> delink_string;

    // Delink the reference represented by `str`, into `value`
    // The reference is the string between '${' and '}'
    auto delink_ref = [&](tstring& str, string_ref_p& value) {
      // Parse the fallback part of the reference string and remove it from consideration.
      auto take_fallback = [&](string_ref_p& fallback) {
        if (auto sep = str.rfind('?'); sep != tstring::npos) {
          delink_string(str.substr(sep + 1).trim_quotes(), fallback);
          str.set_length(sep);
        }
      };
      // Finish the intialization for the types derived from meta_ref
      auto make_meta_ref = [&]<typename T>(unique_ptr<T>&& ptr) {
        take_fallback(ptr->fallback);
        // Set the value field
        delink_string(str.trim_quotes(), ptr->value);
        value = move(ptr);
      };
      if (str.cut_front_back("file:", "")) {
        make_meta_ref(std::make_unique<file_ref>());
      } else if (str.cut_front_back("cmd:", "")) {
        make_meta_ref(std::make_unique<cmd_ref>());
      } else if (str.cut_front_back("env:", "")) {
        make_meta_ref(std::make_unique<env_ref>());
      } else if (str.cut_front_back("color:", "")) {
        // Delink color
        auto newval = std::make_unique<color_ref>();
        // Parse the modification part
        if (auto sep = str.find(';'); sep != tstring::npos) {
          auto sep2 = str.find(':');
          if (sep2 != tstring::npos)
            newval->processor.inter = cspace::stospace(str.substr(0,sep2++).trim());
          else sep2 = 0;
          newval->processor.add_modification(str.substr(sep2, sep - sep2).to_string());
          str.erase_front(sep + 1);
        }
        make_meta_ref(move(newval));
      } else {
        // Delink local value
        string_ref_p fallback;
        take_fallback(fallback);

        // Parse the referenced section and key
        string new_sec, new_key;
        if (auto sep = str.find('.'); sep != tstring::npos) {
          new_sec = str.substr(0, sep).trim().to_string();
          str.erase_front(sep + 1);
        } else new_sec = sec;
        new_key = str.trim().to_string();

        // Look for the referenced key
        if (auto index = doc.find(new_sec, new_key); index) {
          // Found it
          auto& ref_val = doc.values[*index];
          value.reset();

          // Check for cyclical linking
          if (!ref_val) {
            report_err("Cyclical referencing detected");
            value = make_unique<const_ref>(move(src));
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
        } else {
          value = make_unique<const_ref>(move(src));
          report_err("Referenced key doesn't exist: " + new_sec + "." + new_key);
        }
      }
    };

    // Delink an arbitrary string into `value`. The main thing this does is handling string interpolations. Otherwise, it would call delink_ref
    delink_string = [&](tstring& str, string_ref_p& value) {
      // Find the '${' and '}' pairs and mark them with `start` and `end`
      auto find_token = [&](tstring& str, size_t& start, size_t& end) {
        end = 0;
        for(size_t opening_count = 0; end < str.size(); end++)
          if (str[end] == '$') {
            if (str[end + 1] == '$') {
              str.erase(src, end, 1);
            } else if (str[end + 1] == '{') {
              if (opening_count == 0)
                start = end;
              opening_count++;
            }
          } else if (str[end] == '}' && opening_count > 0 && --opening_count == 0)
            return true;
        return false;
      };
      size_t start, end;
      if (!find_token(str, start, end)) {
        // There is no token inside the string, it's a normal string
        value = make_unique<const_ref>(str);
      } else if (start == 0 && end == str.size() - 1) {
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
          ss << str.substr(0, start).to_string();
          // Mark the position of the token in the base string
          newval->interpolator.positions.push_back(ss.tellp());

          // Make string_ref from the token
          auto ref = str.interval(start + 2, end);
          newval->replacements.list.emplace_back();
          delink_ref(ref, newval->replacements.list.back());

          str.erase_front(end + 1);
        } while (find_token(str, start, end));
        ss << str.to_string();
        newval->interpolator.base = ss.str();
        value = move(newval);
      }
    };
    delink_string(str, value);
  };
  for(auto& seckey : doc.map) {
    auto& section = seckey.second;
    for(auto& keyval : section) {
      delink_key(seckey.first, keyval.first, doc.values[keyval.second]);
    }
  }
}

GLOBAL_NAMESPACE_END
