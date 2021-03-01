#include "languages.hpp"
#include "common.hpp"

NAMESPACE(linked_nodes)

// Escape the value and write it and the prefix to the stream
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
