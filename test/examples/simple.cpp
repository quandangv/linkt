#include <linked_nodes/languages.hpp>
#include <iostream>
#include <fstream>

int main() {
  std::ifstream file {"config.yml"};
  if (file.fail()) {
    std::cout << "Failed to load file 'config.yml'. "
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

  while (true) {
    std::cout << "Enter path to get: ";
    std::string path;
    std::getline(std::cin, path);
    auto result = wrapper.get_child(path);
    if (!result)
      std::cout << "Path '" << path << "' have no value." << std::endl;
    else
      std::cout << *result << std::endl;
  }
}
