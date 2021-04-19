#include "languages.hpp"
#include <getopt.h>
#include <fstream>

using namespace std;

int ends_with(const char *str, const char *suffix)
{
  if (!str || !suffix)
    return 0;
  size_t lenstr = strlen(str);
  size_t lensuffix = strlen(suffix);
  if (lensuffix >  lenstr)
    return 0;
  return strncasecmp(str + lenstr - lensuffix, suffix, lensuffix) == 0;
}

bool merge_file(const char* path, node::wrapper_s doc) {
  std::ifstream ifs(path);
  if (ifs.fail()) {
    cerr << "Can't open file: " << path << endl;
    return true;
  }

  node::errorlist err;
  if (ends_with(path, ".yml") || ends_with(path, ".yaml"))
    parse_yml(ifs, err, doc);
  else if (ends_with(path, ".ini"))
    parse_ini(ifs, err, doc);
  else {
    cerr << "Unsupported file type: " << path << endl;
    return true;
  }

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
  auto replacements = std::make_shared<node::wrapper>();

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
  for (; optind < argc-1; optind+=2) {
    std::ifstream ifs(argv[optind]);
    std::ofstream ofs(argv[optind+1]);
    if (ifs.fail()) {
      cerr << "Failed to open file: " << argv[optind] << endl;
    } else if (ofs.fail()) {
      cerr << "Failed to open file: " << argv[optind+1] << endl;
    } else try {
      replace_text(ifs, ofs, replacements);
    } catch(const std::exception& e) {
      cerr << "Replace error in file: " << argv[optind] << "->" << argv[optind+1] << endl << e.what();
    }
  }
}
