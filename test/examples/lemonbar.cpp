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
  node::wrapper wrapper;
  parse_yml(file, wrapper, err);
  file.close();

  if (!err.empty()) {
    std::cout << "Error while parsing:\n";
    for (auto& e : err)
      std::cout << "At " << e.first << ": " << e.second << std::endl;
  }

  std::cout << "If you have lemonbar installed,\n"
      "Pass the result of this program to lemonbar and get a simple status bar\n";
  while (true) {
    auto next_frame = std::chrono::system_clock::now() + std::chrono::milliseconds(100);
    try {
      auto result = wrapper.get_child("lemonbar"_ts);
      if (!result)
        std::cout << "Failed to retrieve the key at path 'lemonbar'";
      else
        std::cout << *result << std::endl;
    } catch (const std::exception& e) {
      std::cout << "Error while retrieving key: " << e.what() << std::endl;
    }
    std::this_thread::sleep_until(next_frame);
  }
}
