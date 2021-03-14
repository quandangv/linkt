#include <linked_nodes/languages.hpp>
#include <iostream>
#include <fstream>
#include <chrono>
#include <thread>
#include <sstream>
#include <cmath>

struct fps_display {
  double current_sum{0.0}, avg{NAN};;
  unsigned int count{0}, loop;

  fps_display(unsigned int loop) : loop(loop) {}
  std::string feed(double value) {
    std::stringstream ss;
    current_sum += value;
    auto current_avg = current_sum / count;
    if (++count < loop) {
      if (std::isnan(avg)) {
        ss << current_avg;
      } else {
        auto diff = current_avg - avg;
        ss << avg << ' ' << std::showpos << diff;
      }
    } else {
      ss << (avg = current_avg);
      count = 0;
      current_sum = 0.0;
    }
    return ss.str();
  }
};

int main() {
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
    std::cout << "Error while parsing:\n";
    for (auto& e : err)
      std::cout << "At " << e.first << ": " << e.second << std::endl;
  }

  node::clone_context context;
  wrapper->optimize(context);
  if (!context.errors.empty()) {
    std::cout << "Error while optimizing:\n";
    for (auto& e : context.errors)
      std::cout << "At " << e.first << ": " << e.second << std::endl;
  }

  std::cout << "If you have lemonbar installed,\n"
      "Pass the result of this program to lemonbar and get a simple status bar\n";
  fps_display fps(20);
  while (true) {
    auto next_frame = std::chrono::steady_clock::now() + std::chrono::milliseconds(50);
    try {
      clock_t start_retrieve = clock();
      auto result = wrapper->get_child("lemonbar"_ts);
      double elapsed = double(clock() - start_retrieve) / CLOCKS_PER_SEC * 1000;
      if (!result)
        std::cout << "Failed to retrieve the key at path 'lemonbar'";
      else
        std::cout << *result << "%{F#fff B#000} " << fps.feed(elapsed) << "ms" << std::endl;
    } catch (const std::exception& e) {
      std::cout << "Error while retrieving key: " << e.what() << std::endl;
    }
    std::this_thread::sleep_until(next_frame);
  }
}
