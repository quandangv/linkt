#include <linked_nodes/languages.hpp>
#include <iostream>
#include <fstream>
#include <thread>
#include <unistd.h>
#include <signal.h>
#include <poll.h>
#include <sys/socket.h>

struct fps_display {
  float current_sum{0}, avg{0};;
  unsigned int count{0}, loop;

  fps_display(unsigned int loop) : loop(loop) {}
  float feed(double value) {
    current_sum += value;
    count++;
    if (count == loop) {
      avg = current_sum / loop;
      count = 0;
      current_sum = 0.0;
    }
    return avg;
  }
};

FILE* lemonbar;

void sighandle(int signal) {
  if (signal == SIGINT || signal == SIGTERM)
    exit(0);
}

void cleanup() {
  if (lemonbar) {
    fclose(lemonbar);
    lemonbar = nullptr;
  }
}

int main(int argc, char** argv) {
  std::ifstream file {"lemonbar.yml"};
  if (file.fail()) {
    std::cout << "Failed to load file 'lemonbar.yml'. "
        "Make sure that you are working in the directory test/example." << std::endl;
    return 1;
  }
  node::errorlist err;
  auto wrapper = parse_yml(file, err);
  file.close();

  if (!err.empty()) {
    std::cerr << "Error while parsing:\n";
    for (auto& e : err)
      std::cerr << "At " << e.first << ": " << e.second << std::endl;
  }
  node::clone_context context;
  wrapper->optimize(context);
  if (!context.errors.empty()) {
    std::cerr << "Error while optimizing:\n";
    for (auto& e : context.errors)
      std::cerr << "At " << e.first << ": " << e.second << std::endl;
  }

  // Fork and call lemonbar
  atexit(cleanup);
  signal(SIGINT, sighandle);
  signal(SIGINT, sighandle);
  int pipes[2];
  if (socketpair(AF_UNIX, SOCK_STREAM, 0, pipes) < 0)
    throw std::runtime_error("socketpair failed");
  switch(auto pid = fork()) {
    case -1:
      close(pipes[0]);
      close(pipes[1]);
      throw std::runtime_error("fork failed");
    case 0: // Child case
      dup2(pipes[1], STDOUT_FILENO);
      dup2(pipes[1], STDIN_FILENO);
      close(pipes[0]);
      close(pipes[1]);
      execl("/usr/bin/sh", "sh", "-c", argc > 1 ? argv[1] : "lemonbar", nullptr);
      return 127;
  }
  // Parent case
  lemonbar = fdopen(pipes[0], "r+");
  close(pipes[1]);
  pollfd pollin{pipes[0], POLLIN, 0};

  fps_display fps(40);
  while (true) {
    if (poll(&pollin, 1, 0) > 0 && pollin.revents & POLLIN) {
      char buffer[128];
      char* buffer_end = buffer;
      while (true) {
        auto count = read(pipes[0], buffer, 128);
        if (count == 0) break;
        if (count < 0) return -1;
        buffer_end += count;
        char* line_start = buffer, *newline;
        while ((newline = (char*)memchr(line_start, '\n', buffer_end - line_start))) {
          std::cerr << line_start;
          *newline = 0;
          if (!memcmp(line_start, "export ", 7)) {
            line_start += 7;
            for (; line_start < newline; line_start++)
              if (!std::isspace(*line_start))
                break;
            auto sep = (char*)memchr(line_start, '=', buffer_end - line_start);
            if (sep) {
              *sep = 0;
              std::cerr << "set " << line_start << " to " << sep + 1 << "\n";
              setenv(line_start, sep + 1, 1);
            } else std::cerr << "Invalid: " << line_start << std::endl;
          } else {
            system(line_start);
          }
          line_start = newline + 1;
        }
        if (count < 128 || buffer[127] == '\n') break;
      }
    }

    auto next_frame = std::chrono::steady_clock::now() + std::chrono::milliseconds(50);
    try {
      clock_t start_retrieve = clock();
      auto result = wrapper->get_child("lemonbar"_ts);
      double elapsed = double(clock() - start_retrieve) / CLOCKS_PER_SEC * 1000;
      if (!result)
        std::cerr << "Failed to retrieve the key at path 'lemonbar'";
      else if (!wrapper->set<float>("lemonbar.fpms.calculation"_ts
          , round(100/fps.feed(elapsed))/100))
        std::cerr << "Can't set fps value";
      fputs(result->data(), lemonbar);
      fflush(lemonbar);
    } catch (const std::exception& e) {
      std::cerr << "Error while retrieving key: " << e.what() << std::endl;
    }
    std::this_thread::sleep_until(next_frame);
  }
}
