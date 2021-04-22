#include "parse.hpp"
#include "replace.hpp"
#include <getopt.h>
#include <fstream>

using namespace std;

// Checks if `str` ends with `suffix` (case-insensitive)
bool ends_with(const char *str, const char *suffix) {
  if (!str || !suffix)
    return false;
  size_t lenstr = strlen(str);
  size_t lensuffix = strlen(suffix);
  if (lensuffix >  lenstr)
    return false;
  return !strncasecmp(str + lenstr - lensuffix, suffix, lensuffix);
}

// Parse the file at `path` and merge its tree to `tree`. Returns true if an error occourred
bool merge_file(const char* path, node::wrapper_s tree) {
  // Try to open the file
  std::ifstream ifs(path);
  if (ifs.fail()) {
    cerr << "Can't open file: " << path << endl;
    return true;
  }

  // Parse the file according to its extension
  node::errorlist err;
  if (ends_with(path, ".yml") || ends_with(path, ".yaml"))
    parse_yml(ifs, err, tree);
  else if (ends_with(path, ".ini"))
    parse_ini(ifs, err, tree);
  else {
    cerr << "Unsupported file type: " << path << endl;
    return true;
  }

  // Print out the errors
  if (!err.empty()) {
    cerr << "Parse errors of file:" << path << endl;
    for(auto& e : err)
      cerr << "At " << e.first << ": " << e.second << endl;
    return true;
  }
  return false;
}

void print_help(const char* name) {
  cout << "Syntax: " << name << " [-i dictionary-path]... [input-path output-path]" << endl;
}

int main(int argc, char** argv) {
  // The tree used for replacement of files
  auto replacements = std::make_shared<node::wrapper>();

  // Parse the options
  for (int ch; (ch = getopt(argc, argv, "i:h")) != -1;) {
    switch (ch) {
      case 'i':
        merge_file(optarg, replacements);
        break;
      case 'h':
        print_help(*argv);
        return 1;
    }
  }
  // Use pairs from the non-option arguments. If an odd number of argument remain, the last argument is ignored
  for (; optind < argc-1; optind+=2) {
    // Replace the content of the file in the first arg, output to the path of the second arg
    std::ifstream ifs(argv[optind]);
    std::ofstream ofs(argv[optind+1]);
    if (ifs.fail()) {
      cerr << "Failed to open file: " << argv[optind] << endl;
    } else if (ofs.fail()) {
      cerr << "Failed to open file: " << argv[optind+1] << endl;
    } else try {
      replace_text(ifs, ofs, replacements);
    } catch(const std::exception& e) {
      cerr << "Replace error in file: " << argv[optind] << " -> " << argv[optind+1] << endl << e.what();
    }
  }
}
