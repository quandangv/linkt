#include <linked_nodes/languages.hpp>
#include <iostream>
#include <fstream>
#include <chrono>
#include <thread>
#include <cmath>

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
  fps_display fps(40);
  while (true) {
    auto next_frame = std::chrono::steady_clock::now() + std::chrono::milliseconds(50);
    try {
      clock_t start_retrieve = clock();
      auto result = wrapper->get_child("lemonbar"_ts);
      double elapsed = double(clock() - start_retrieve) / CLOCKS_PER_SEC * 1000;
      if (!result)
        std::cout << "Failed to retrieve the key at path 'lemonbar'";
      else if (!wrapper->set<float>("lemonbar.fpms.calculation"_ts, round(100 / fps.feed(elapsed))/100))
        std::cout << "Can't set fps value";
      std::cout << *result << std::endl;
    } catch (const std::exception& e) {
      std::cout << "Error while retrieving key: " << e.what() << std::endl;
    }
    std::this_thread::sleep_until(next_frame);
  }
}
