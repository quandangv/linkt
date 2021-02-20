#include "languages.hpp"
#include "common.hpp"

NAMESPACE(lini)

void errorlist::report_error(int linecount, const string& msg) {
  emplace_back("line " +std::to_string(linecount), msg);
}

bool errorlist::extract_key(tstring& line, int linecount, char separator, tstring& key) {
  key = cut_front(line, separator);
  if (key.untouched())
    return report_error(linecount, "Line ignored: " + line), false;
  return true;
}

void write_key(std::ostream& os, const string& prefix, string&& value) {
  size_t opening = 0;
  while((opening = value.find("${", opening)) != string::npos) {
    value.insert(value.begin() + opening, '\\');
    opening += 2;
  }
  if (isspace(value.front()) || isspace(value.back()))
    os << prefix << '"' << value << '"' << endl;
  else
    os << prefix << value << endl;
}

NAMESPACE_END
