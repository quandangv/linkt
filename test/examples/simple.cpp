#include <linkt/languages.hpp>
#include <iostream>
#include <fstream>

int main() {
  std::ifstream file {"simple.yml"};
  if (file.fail()) {
    std::cout << "Failed to load file 'simple.yml'. "
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

  std::cout << "Available keys: \n";
  std::cout << "  status\n";
  std::cout << "  status.greeting\n";
  std::cout << "  status.cpu\n";
  std::cout << "  status.memory\n";
  std::cout << "  status.battery\n";
  std::cout << "  status.date\n";
  while (true) {
    std::cout << "Enter key to get value: ";
    std::string path;
    std::getline(std::cin, path);
    try {
      auto result = wrapper->get_child(path);
      if (!result)
        std::cout << "Path '" << path << "' have no value." << std::endl;
      else
        std::cout << *result << std::endl;
    } catch (const std::exception& e) {
      std::cout << "Error while retrieving key: " << e.what() << std::endl;
    }
  }
}
