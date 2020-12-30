#pragma once

#ifdef PLATFORM_LINUX
  #define FORMAT_RESET  "\033[0m"
  #define FORMAT_SIZE 11
  #define FORMAT_EMPTY  "\033[0;36m\033[0m"
  #define FORMAT_EMPTY2 "\033[0;36m\033[0m\033[0;36m\033[0m"
  #define FORMAT_GREEN(content)  "\033[0;32m"#content FORMAT_RESET
  #define FORMAT_BLUE(content)   "\033[0;34m"#content FORMAT_RESET
  #define FORMAT_CYAN(content)   "\033[0;36m"#content FORMAT_RESET
  #define FORMAT_RED_BOLD(content)    "\033[1;31m"#content FORMAT_RESET
  #define FORMAT_GREEN_BOLD(content)  "\033[1;32m"#content FORMAT_RESET
  #define FORMAT_YELLOW_BOLD(content) "\033[1;33m"#content FORMAT_RESET
  #define FORMAT_BLUE_BOLD(content)   "\033[1;34m"#content FORMAT_RESET
#else
  #define FORMAT_RESET 
  #define FORMAT_SIZE 0
  #define FORMAT_EMPTY 
  #define FORMAT_EMPTY2
  #define FORMAT_GREEN(content)  #content
  #define FORMAT_BLUE(content)   #content
  #define FORMAT_CYAN(content)   #content
  #define FORMAT_RED_BOLD(content)    #content
  #define FORMAT_GREEN_BOLD(content)  #content
  #define FORMAT_YELLOW_BOLD(content) #content
  #define FORMAT_BLUE_BOLD(content)   #content
#endif
