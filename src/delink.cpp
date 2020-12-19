#include "delink.hpp"

#include <cstdlib>
#include <fstream>

#include "tstring.hpp"
#include "logger.hpp"

void delink(document& doc, str_errlist& err, const string& sec, const string& key, string& src) {
  auto report_err = [&](const string& msg) {
    err[sec + "." + key] = msg;
  };

  tstring mod(src);
  if (mod.cut_front_back("${", "}")) {
    tstring fallback;
    auto take_fallback = [&] {
      auto sep = mod.rfind(':');
      if (sep != tstring::npos) {
        fallback = mod.substr(sep + 1);
        mod.set_length(sep);
      }
    };

    if (mod.cut_front_back("file:", "")) {
      take_fallback();
      // Copy the string because the original string have to be returned if the operation fail and no fallback provided
      std::ifstream ifs(mod.to_string().data());
      if (!ifs.fail()) {
        src.assign(std::istreambuf_iterator<char>{ifs}, {});
        ifs.close();
        auto last_line = src.find_last_not_of("\r\n");
        src.erase(last_line + 1);
        return;
      }
    } else if (mod.cut_front_back("env:", "")) {
      take_fallback();
      if (auto new_value = std::getenv(mod.to_string().data()); new_value != nullptr) {
        src = string(new_value);
        return;
      }
    } else {
      take_fallback();
      string new_sec, new_key;
      auto sep = mod.find('.');
      if (sep == tstring::npos) {
        new_key = mod.to_string();
        new_sec = sec;
      } else {
        new_sec = mod.substr(0, sep).to_string();
        new_key = mod.substr(sep + 1).to_string();
      }

      if (auto new_value = find(doc, new_sec, new_key); new_value != nullptr) {
        // Clear to avoid cyclical linking
        src.clear();
        delink(doc, err, new_sec, new_key, *new_value);
        src = *new_value;
        return;
      }
    }
    if (fallback.untouched())
      report_err("De-linking failed and no fallback was found");
    else src = fallback.to_string();
  }
}

void delink(document& doc, str_errlist& err) {
  for(auto& seckey : doc) {
    auto& section = seckey.second;
    for(auto& keyval : section) {
      delink(doc, err, seckey.first, keyval.first, keyval.second);
    }
  }
}
