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
    
    string src;
    if (auto onetime = dynamic_cast<onetime_ref*>(value.get()); onetime != nullptr) {
      src = onetime->get_onetime();
    } else return;
    tstring mod(src);

    auto take_fallback = [&](string_ref_p& fallback) {
      if (auto sep = mod.rfind('?'); sep != tstring::npos) {
        fallback = make_unique<onetime_ref>(mod.substr(sep + 1).trim_quotes().to_string());
        delink_key(sec, key, fallback);
        mod.set_length(sep);
      }
    };
    std::function<void()> subroutine;

    auto make_meta_ref = [&]<typename T>(unique_ptr<T>&& ptr) {
      take_fallback(ptr->fallback);
      mod.trim_quotes();
      if (mod.front() == '$') {
        mod.erase_front();
        subroutine();
        ptr->value = move(value);
      } else ptr->value = make_unique<const_ref>(mod.to_string());
      value = move(ptr);
    };
    subroutine = [&] {
      if (mod.cut_front_back("file:", "")) {
        make_meta_ref(std::make_unique<file_ref>());
      } else if (mod.cut_front_back("cmd:", "")) {
        make_meta_ref(std::make_unique<cmd_ref>());
      } else if (mod.cut_front_back("env:", "")) {
        make_meta_ref(std::make_unique<env_ref>());
      } else if (mod.cut_front_back("color:", "")) {
        // Delink color
        auto newval = std::make_unique<color_ref>();
        if (auto sep = mod.find(';'); sep != tstring::npos) {
          auto sep2 = mod.find(':');
          if (sep2 != tstring::npos)
            newval->processor.inter = cspace::stospace(mod.substr(0,sep2++).trim());
          else sep2 = 0;
          newval->processor.add_modification(mod.substr(sep2, sep - sep2).to_string());
          mod.erase_front(sep + 1);
        }
        make_meta_ref(move(newval));
      } else {
        // Delink local value
        string_ref_p fallback;
        take_fallback(fallback);

        string new_sec, new_key;
        if (auto sep = mod.find('.'); sep != tstring::npos) {
          new_sec = mod.substr(0, sep).trim().to_string();
          mod.erase_front(sep + 1);
        } else new_sec = sec;
        new_key = mod.trim().to_string();
        logger::debug<scope>(new_key);

        if (auto index = doc.find(new_sec, new_key); index) {
          // Clear to avoid cyclical linking
          value.reset();
          auto& ref_val = doc.values[*index];
          if (!ref_val) {
            report_err("Cyclical referencing detected");
            value = make_unique<const_ref>(move(src));
          } else {
            delink_key(new_sec, new_key, ref_val);
            value = std::make_unique<local_ref>(ref_val);
          }
        } else if (fallback)
          value = move(fallback);
        else {
          value = make_unique<const_ref>(move(src));
          return report_err("Referenced key doesn't exist: " + new_sec + "." + new_key);
        }
      }
    };

    if (!mod.cut_front_back("${", "}")) {
      stringstream ss;
      auto newval = make_unique<string_interpolate_ref>();
      size_t start = 0;
      tstring original(mod);
      while(true) {
        if (start = original.find('$'); start != tstring::npos && original.size() > start++ + 2 && original[start++] == '{') {
          if (auto end = original.find('}'); end != tstring::npos) {
            ss << original.substr(0, start - 2).to_string();
            newval->interpolator.positions.push_back(ss.tellp());

            mod = original.substr(start, end - start);
            subroutine();
            newval->replacements.list.emplace_back(move(value));

            original.erase_front(++end);
            start = end;
            continue;
          }
        }
        break;
      }
      if (newval->interpolator.positions.empty()) {
        value = make_unique<const_ref>(move(src));
        return;
      }
      ss << original.to_string();
      newval->interpolator.base = ss.str();
      value = move(newval);
      return;
    }

    subroutine();
  };
  for(auto& seckey : doc.map) {
    auto& section = seckey.second;
    for(auto& keyval : section) {
      delink_key(seckey.first, keyval.first, doc.values[keyval.second]);
    }
  }
}

GLOBAL_NAMESPACE_END
