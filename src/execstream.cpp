#include "execstream.hpp"
#include "logger.hpp"

#include <cstdlib>
#include <cerrno>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>

DEFINE_ERROR(execstream_error)
int devnull = open("/dev/null", O_WRONLY);

execstream::execstream(const char* command, int type) {
  if (type == 0) throw execstream_error("invalid type");
  int pdes[2];

  bool write = type_in & type,
       read = type_out_err & type;
  if (read && write) {
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, pdes) < 0)
      throw execstream_error("socketpair() failed");
  } else {
    if (pipe(pdes) < 0)
      throw execstream_error("pipe() failed");
  }
  switch (pid = fork()) {
  case -1:
    ::close(pdes[0]);
    ::close(pdes[1]);
    throw execstream_error("fork() failed");
  case 0:
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

execstream::~execstream() {
  close();
}
