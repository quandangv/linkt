#pragma once

#include <istream>

#include "document.hpp"

void parse(std::istream& is, document&, errorlist&);
