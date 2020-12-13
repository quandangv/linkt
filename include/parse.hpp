#pragma once

#include <string>
#include <map>
#include <istream>
#include <tuple>

using std::string;

using section = std::map<string, string>;
using document = std::map<string, section>;
using errorlist = std::map<int, string>;

void parse(std::istream& is, document&, errorlist& errors);

