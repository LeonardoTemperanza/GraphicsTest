
#ifdef _WIN32
#include "os/os_base_windows.cpp"
#elif defined(__linux__)
#error "Linux operating systems not supported."
#elif defined(__APPLE__)
#error "Apple Operating systems not supported."
#else
#error "Unknown operating system."
#endif