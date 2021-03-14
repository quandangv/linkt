#include <linked_nodes/languages.hpp>
#include <iostream>
#include <fstream>
#include <chrono>
#include <thread>

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
  double avg_time = 0;
  constexpr double recent_weight = 0.05;
  while (true) {
    auto next_frame = std::chrono::steady_clock::now() + std::chrono::milliseconds(50);
    try {
      clock_t start_retrieve = clock();
      auto result = wrapper->get_child("lemonbar"_ts);
      double elapsed = double(clock() - start_retrieve) / CLOCKS_PER_SEC * 1000;
      avg_time = elapsed * recent_weight + avg_time * (1 - recent_weight);
      if (!result)
        std::cout << "Failed to retrieve the key at path 'lemonbar'";
      else
        std::cout << *result << "%{F#fff B#000} " << avg_time << "ms" << std::endl;
    } catch (const std::exception& e) {
      std::cout << "Error while retrieving key: " << e.what() << std::endl;
    }
    std::this_thread::sleep_until(next_frame);
  }
}
