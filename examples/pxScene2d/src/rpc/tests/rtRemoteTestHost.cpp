#include <dlfcn.h>
#include <iostream>
#include <cassert>

#define RT_REMOTE_TEST_CASE(F) extern "C" F

int main (int argc, char* argv[])
{
  void (*f)();
  assert(argc == 3);
  void* handle = dlopen( "./librtRemoteTests.so", RTLD_NOW );
  *(void **)(&f) = dlsym(handle, argv[2]);
  (*f)();
  return 1;
}


