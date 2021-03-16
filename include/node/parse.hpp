#pragma once

#include "base.hpp"

namespace node {
  template<class T> std::shared_ptr<base<T>>
  parse_raw  (string& raw, tstring& str, parse_context& context);

  template<class T> std::shared_ptr<base<T>>
  parse_escaped  (string& raw, tstring& str, parse_context& context);
}

#include "parse.hxx"
