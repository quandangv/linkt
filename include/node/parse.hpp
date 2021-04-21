#pragma once

#include "base.hpp"

namespace node {

    template<class T> std::shared_ptr<base<T>>
  parse_raw(parse_context& context, tstring& value);

    template<class T> std::shared_ptr<base<T>>
  parse_escaped(parse_context& context, tstring& value);

    template<class T> std::shared_ptr<base<T>>
  checked_parse_raw(parse_context& context, tstring& value);
}
