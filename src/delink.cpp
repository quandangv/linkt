#include "delink.hpp"

#include "tstring.hpp"

void delink(document& doc, str_errlist& err, const string& sec, const string& key, string& src) {
  auto report_err = [&](const string& msg) {
    err[sec + "." + key] = msg;
  };

  tstring mod(src);
  if (mod.cut_front_back("${", "}")) {
    auto sep = mod.find('.');
    string new_sec, new_key;
    if (sep == tstring::npos) {
      new_sec = sec;
    } else {
      new_sec = mod.substr(0, sep).to_string();
      mod.erase_front(sep + 1);
    }

    sep = mod.find(':');
    if (sep == tstring::npos) {
      new_key = mod.to_string();
      mod = tstring(src);
    } else {
      new_key = mod.substr(0, sep).to_string();
      mod.erase_front(sep + 1);
    }

    if (auto new_value = find(doc, new_sec, new_key); new_value != nullptr) {
      // Clear to avoid cyclical linking
      src.clear();
      delink(doc, err, new_sec, new_key, *new_value);
      src = *new_value;
    } else src = mod.to_string();
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
