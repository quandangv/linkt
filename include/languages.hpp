#pragma once

#include <istream>

#include "document.hpp"

namespace lini {
  struct errorlist : std::vector<std::pair<std::string, std::string>> {
    void report_error(int line, const std::string& msg);
    void report_error(const std::string& path, const std::string& msg);
    bool extract_key(tstring& line, int linecount, char separator, tstring& key);
    bool check_name(const tstring& name, int linecount);
  };
  void parse_ini(std::istream&, document&, errorlist&);
  void write_ini(std::ostream&, const container&, const string& prefix = "");
  void parse_yml(std::istream&, document&, errorlist&);
  void write_yml(std::ostream&, const container&, int indent = 0);

  void write_key(std::ostream&, const std::string& prefix, std::string&& value);
}
