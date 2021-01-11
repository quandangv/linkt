#include "execstream.hpp"
#include "logger.hpp"

#include <array>
#include <cstdlib>
#include <cerrno>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>

using namespace std;

int devnull = open("/dev/null", O_WRONLY);

execstream::execstream(const char* command, int type) {
  if (type % 8 == 0) throw error("invalid type");
  bool write = type_in & type,
       read = type_out_err & type;

  // Open a two-way pipe for read-write streams, otherwise, open a one-way pipe
  int pdes[2];
  if (read && write) {
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, pdes) < 0)
      throw error("socketpair() failed");
  } else {
    if (pipe(pdes) < 0)
      throw error("pipe() failed");
  }
  switch (pid = fork()) {
  case -1:
    // Fail
    ::close(pdes[0]);
    ::close(pdes[1]);
    throw error("fork() failed");
  case 0:
    // Child process
    if (read) {
      dup2(type_out & type ? pdes[1] : devnull, STDOUT_FILENO);
      dup2(type_err & type ? pdes[1] : devnull, STDERR_FILENO);
      if (write && pdes[1] != STDIN_FILENO)
        dup2(pdes[1], STDIN_FILENO);
    } else
      dup2(pdes[0], STDIN_FILENO);
    ::close(pdes[0]);
    ::close(pdes[1]);
    execl("/usr/bin/sh", "sh", "-c", command, nullptr);
    exit(127);
  }

  // Parent process
  if (read) {
    fp = fdopen(pdes[0], write ? "r+" : "r");
    ::close(pdes[1]);
  } else {
    fp = fdopen(pdes[1], "w");
    ::close(pdes[0]);
  }
}

execstream::execstream(execstream&& other) : fp(other.fp), pid(other.pid) {
  other.fp = nullptr;
}

execstream::~execstream() {
  close();
}

int execstream::close() {
  if (fp == nullptr)
    return -1;
  fclose(fp);
  fp = nullptr;
  int pstat;
  do {
    pid = waitpid(pid, &pstat, 0);
  } while (pid == -1 && errno == EINTR);
  return (pid == -1 ? -1 : pstat);
}

int execstream::write(const string& s) {
  return fputs(s.data(), fp);
}

string execstream::readall() {
  string result;
  array<char, 128> buffer;
  while (fgets(buffer.data(), buffer.size(), fp) != nullptr)
    result += buffer.data();
  return result;
}

