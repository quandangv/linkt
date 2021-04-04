#include "node.hpp"
#include "wrapper.hpp"
#include "parse.hpp"
#include "common.hpp"
#include "token_iterator.hpp"

#include <fstream>
#include <cstdlib>
#include <array>
#include <unistd.h>
#include <sys/socket.h>

NAMESPACE(node)

  template<class T>
nested<T>::nested(parse_context& context, parse_preprocessed& prep)
    : value(checked_parse_raw<T>(context, prep.tokens[prep.token_count - 1])) {}
  template<class T>
nested<T>::nested(const nested<T>& other, clone_context& context)
    : value(checked_clone<T>(other.value, context, "nested::nested")) {}

color::operator string() const {
  try {
    auto result = processor.operate(value->get());
    return result.empty() && fallback ? fallback->get() : result;
  } catch(const std::exception& e) {
    return use_fallback("Color processing failed, due to: "s + e.what());
  }
}

base_s color::clone(clone_context& context) const {
  if (context.optimize && is_fixed())
    return std::make_shared<plain<string>>(operator string());
  auto result = std::make_shared<color>(*this, context);
  result->processor = processor;
  return result;
}

color::color(parse_context& context, parse_preprocessed& prep) : meta(context, prep) {
  if (prep.token_count > 2) {
    if (prep.token_count > 3)
      processor.inter = cspace::stospace(prep.tokens[1]);
    processor.add_modification(trim_quotes(prep.tokens[prep.token_count - 2]));
  }
}

template<class From, class To, class Processor>
lazy_node<From, To, Processor>::lazy_node(parse_context& context, parse_preprocessed& prep)
    : loaded_node<From, To, Processor>(context, prep) {
  if (prep.token_count != 3)
    THROW_ERROR(parse, "gradient: Expected 2 components");
  base_raw = checked_parse_raw<string>(context, prep.tokens[1]);
}

template<class From, class To, class Processor>
base_s lazy_node<From, To, Processor>::clone(clone_context& context) const {
  if (context.optimize) {
    auto result = std::make_unique<loaded_node<From, To, Processor>>(*this, context);
    result->base = loaded_node<From, To, Processor>::get_base();
    return result;
  }
  auto result = std::make_shared<gradient>(*this, context);
  result->base_raw = checked_clone<string>(base_raw, context, "gradient::clone");
  return result;
}

template<>
loaded_node<float, string, cspace::gradient<3>>::operator string() const {
  return get_base().get_hex(value->operator float());
}

cspace::gradient<3>& gradient::get_base() const {
  if (!base.points.size()) {
    auto str = base_raw->get();
    tstring ts(str);
    tstring point;
    trim_quotes(ts);
    while (!(point = get_word(ts)).untouched()) {
      if (auto at = cut_front(point, ':'); !at.untouched()) {
        base.add_hex(node::parse<float>(at.begin(), at.size()), point, false);
      } else
        THROW_ERROR(parse, "gradient: invalid point: " + point);
    }
    base.convert(cspace::colorspaces::rgb, cspace::colorspaces::cielch);
    base.auto_add(10);
    base.convert(cspace::colorspaces::cielch, cspace::colorspaces::rgb);
  }
  return base;
}

env::operator string() const {
  auto result = getenv(value->get().data());
  return string(result ?: use_fallback("Environment variable not found: " + value->get()));
}

bool env::set(const string& newval) {
  setenv(value->get().data(), newval.data(), true);
  return true;
}

base_s env::clone(clone_context& context) const {
  return std::make_shared<env>(*this, context);
}

file::operator string() const {
  std::ifstream ifs(value->get().data());
  if (ifs.fail())
    return use_fallback("Can't read file: " + value->get());

  string result(std::istreambuf_iterator<char>{ifs}, {});
  ifs.close();
  return result.erase(result.find_last_not_of("\r\n") + 1);
}

bool file::set(const string& content) {
  std::ofstream ofs(value->get().data(), std::ios_base::trunc);
  if (ofs.fail()) {
    return false;
  }
  ofs << content;
  ofs.close();
  return true;
}

base_s file::clone(clone_context& context) const {
  return std::make_shared<file>(*this, context);
}

cmd::operator string() const {
  string result;
  try {
    auto file = popen((value->get() + string(" 2>/dev/null")).data(), "r");
    std::array<char, 128> buf;
    while (fgets(buf.data(), 128, file) != nullptr)
      result += buf.data();
    if (auto exit_code = WEXITSTATUS(pclose(file)))
      return use_fallback("Process produced exit code: " + std::to_string(exit_code));
  } catch (const std::exception& e) {
    return use_fallback("Encountered error: "s + e.what());
  }
  auto last_line = result.find_last_not_of("\r\n");
  result.erase(last_line + 1);
  return result;
}

base_s cmd::clone(clone_context& context) const {
  return std::make_shared<cmd>(*this, context);
}

void poll::start_cmd() const {
  int pipes[2];
  if (socketpair(AF_UNIX, SOCK_STREAM, 0, pipes) < 0)
    throw std::runtime_error("socketpair failed");
  switch(fork()) {
    case -1:
      close(pipes[0]);
      close(pipes[1]);
      throw std::runtime_error("fork failed");
    case 0: // Child case
      dup2(pipes[1], STDOUT_FILENO);
      dup2(pipes[1], STDERR_FILENO);
      dup2(pipes[1], STDIN_FILENO);
      close(pipes[0]);
      close(pipes[1]);
      execl("/usr/bin/bash", "bash", "-c", value->get().data(), nullptr);
      throw std::runtime_error("execl failed");
  }
  // Parent case
  close(pipes[1]);
  pfd.fd = pipes[0];
}

poll::~poll() {
  if (pfd.fd) {
    close(pfd.fd);
    pfd.fd = pfd.events = 0;
  }
}

poll::operator string() const {
  if (!pfd.fd)
    start_cmd();
  if (::poll(&pfd, 1, 0) > 0 && pfd.revents & POLLIN) {
    // Return the last line
    char buffer[128];
    string result;
    while(1) {
      auto count = read(pfd.fd, buffer, 128);
      if (count == 0) return result;
      if (count < 0) {
        if (errno == EINTR) continue;
        start_cmd();
        throw std::runtime_error("read failed");
      }
      auto line_end = (char*)memrchr(buffer, '\n', count);
      if (!line_end) {
        result += string(buffer, count);
        if (count < 128)
          return result;
      } else {
        if (line_end == buffer + count-1) {
          if (line_end == buffer)
            return !result.empty() ? result : fallback ? fallback->get() : "";
          if (auto line_start = (char*)memrchr(buffer, '\n', line_end - buffer - 1))
            return string(line_start+1, line_end - line_start - 1);
          return result += string(buffer, line_end - buffer);
        }
        result = string(line_end + 1, count - (line_end - buffer) - 1);
        if (count < 128)
          return result;
      }
    }
  }
  return fallback ? fallback->get() : "";
}

base_s poll::clone(clone_context& context) const {
  return std::make_shared<poll>(*this, context);
}

bool poll::set(const string& value) {
  if (!pfd.fd)
    start_cmd();
  if (write(pfd.fd, value.data(), value.size()) == -1) {
    return false;
  }
  return write(pfd.fd, "\n", 1) != -1;
}

save::operator string() const {
  auto str = value->get();
  auto sep = str.rfind(delimiter);
  string result;
  if (sep == string::npos) {
    result = str;
  } else {
    result = str.substr(sep + 1);
    str.erase(sep);
  }
  if (!target)
    THROW_ERROR(node, "save: Target is empty");
  if (auto conv_target = std::dynamic_pointer_cast<settable<string>>(target);
      !conv_target || !conv_target->set(str)) {
    THROW_ERROR(node, "save: Can't set value to target");
  }
  return result;
}

bool save::set(const string& value) {
  if (auto conv_target = std::dynamic_pointer_cast<settable<string>>(target);
      !conv_target || !conv_target->set(value)) {
    return false;
  }
  return true;
}

base_s save::clone(clone_context& context) const {
  auto result = std::make_shared<save>();
  result->value = checked_clone<string>(value, context, "save::clone");
  result->target = checked_clone<string>(target, context, "save::clone");
  result->delimiter = delimiter;
  return result;
}

save::save(parse_context& context, parse_preprocessed& prep) {
  if (prep.token_count != 3 && prep.token_count != 4)
    THROW_ERROR(parse, "save: Expected 2 or 3 components, actual: " + std::to_string(prep.token_count - 1));
  target = checked_parse_raw<string>(context, prep.tokens[1]);
  value = checked_parse_raw<string>(context, prep.tokens[2]);
  if (prep.token_count == 4) {
    if (prep.tokens[3].size() != 1)
      THROW_ERROR(parse, "save: Only single character delimiters are accepted");
    delimiter = prep.tokens[3].front();
  }
}

inline float clamp(float value) {
  return value <= 0 ? 0 : value >= 1 ? 1 : value;
}

map::operator float() const {
  return to_min + to_range * clamp((value->operator float() - from_min)/from_range);
}

base_s map::clone(clone_context& context) const {
  if (context.optimize && is_fixed())
      return std::make_shared<plain<float>>(operator float());
  auto result = std::make_shared<map>();
  result->value = checked_clone<float>(value, context, "map::clone");
  result->from_min = from_min;
  result->from_range = from_range;
  result->to_min = to_min;
  result->to_range = to_range;
  return result;
}

std::shared_ptr<map> map::parse(parse_context& context, parse_preprocessed& prep) {
  if (prep.token_count != 4)
    THROW_ERROR(parse, "map: Expected 3 components");
  auto result = std::make_shared<map>();
  result->value = checked_parse_raw<float>(context, prep.tokens[prep.token_count - 1]);
  if (auto min = cut_front(prep.tokens[1], ':'); !min.untouched())
    result->from_min = convert<float, strtof>(min);
  result->from_range = convert<float, strtof>(prep.tokens[1]) - result->from_min;

  if (auto min = cut_front(prep.tokens[2], ':'); !min.untouched())
    result->to_min = convert<float, strtof>(min);
  result->to_range = convert<float, strtof>(prep.tokens[2]) - result->to_min;
  return result;
}

smooth::operator float() const {
  return current += velocity += (value->operator float() - current) * spring - velocity * drag;
}

std::shared_ptr<smooth> smooth::parse(parse_context& context, parse_preprocessed& prep) {
  std::shared_ptr<smooth> result;
  if (prep.token_count < 3)
    goto wrong_token_count;
  result = std::make_shared<smooth>();
  result->value = checked_parse_raw<float>(context, prep.tokens[prep.token_count - 1]);
  result->drag = node::parse<float>(prep.tokens[1], "smooth::parse");
  if (prep.token_count == 3)
    // In this case, drag shouldn't be greater than 1.2, or the resulting smooth will be jagged
    result->spring = result->drag * result->drag / 3;
  else if (prep.token_count == 4)
    result->spring = node::parse<float>(prep.tokens[2], "smooth::parse");
  else
    goto wrong_token_count;
  return result;
  wrong_token_count:
  THROW_ERROR(parse, "smooth: Expected 2 or 3 components");
}

base_s smooth::clone(clone_context& context) const {
  if (context.optimize && is_fixed()) {
    return std::make_shared<plain<float>>(value->operator float());
  }
  auto result = std::make_shared<smooth>();
  result->value = checked_clone<float>(value, context, "smooth::clone");
  result->spring = spring;
  result->drag = drag;
  return result;
}

clock::operator int() const {
  auto unlooped = (std::chrono::steady_clock::now() - zero_point) / tick_duration;
  return unlooped % loop;
}

base_s clock::clone(clone_context&) const {
  auto result = std::make_shared<clock>();
  result->tick_duration = tick_duration;
  result->loop = loop;
  result->zero_point = zero_point;
  return result;
}

std::shared_ptr<clock> clock::parse(parse_context&, parse_preprocessed& prep) {
  if (prep.token_count != 4)
    THROW_ERROR(parse, "clock: Expected 3 components");
  auto result = std::make_shared<clock>();
  result->tick_duration = std::chrono::milliseconds(
      node::parse<unsigned long>(prep.tokens[1], "clock::parse"));
  result->loop = node::parse<unsigned long>(prep.tokens[2], "clock::parse");
  result->zero_point = steady_time(
      result->tick_duration * node::parse<unsigned long>(prep.tokens[3], "clock::parse"));
  return result;
}

NAMESPACE_END
