#pragma once

#include <cerrno>
#include <cstring>
#include <stdexcept>

class application_error : public std::runtime_error {
 public:
  explicit application_error(const std::string& message, int code = 0) : std::runtime_error(message), code(code) {}
  virtual ~application_error() {}
  int code{0};
};

class system_error : public application_error {
 public:
  explicit system_error() : application_error(std::strerror(errno), errno) {}
  explicit system_error(const std::string& message) : application_error(message + " (reason: " + std::strerror(errno) + ")", errno) {}
  virtual ~system_error() {}
};

#define DEFINE_CHILD_ERROR(error, parent) \
  class error : public parent { \
    using parent::parent; \
  };
#define DEFINE_ERROR(error) DEFINE_CHILD_ERROR(error, application_error)

