#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <errno.h>
#include <vector>
#include <string>

struct TestCase
{
  TestCase(std::string c, std::string s)
    : client(c), server(s)
  { }
  std::string client;
  std::string server;
};

std::vector<TestCase>
buildTestCaseMap()
{
  std::vector< TestCase > v;
  v.push_back( TestCase("Test_Client_Set_Get_Bool", "Test_Server_Set_Get_Generic") );
  v.push_back( TestCase("Test_Client_Set_Get_Int8", "Test_Server_Set_Get_Generic") );
  return v;
}

int main (int argc, char* argv[])
{
  printf("path: %s", argv[0]);
  char * exe = "./testHost";
  std::vector< TestCase > testCases = buildTestCaseMap();

  for (auto const& tc : testCases)
  {
    pid_t s_pid;
    s_pid = fork();
    if (s_pid < 0) // fail
    {
      perror("failed to fork server");
      _exit(1);
    }
    else if (s_pid == 0) // child
    {
      char * sArgs[4] = { exe, "server", const_cast<char*>(tc.server.c_str()), NULL} ;
      execvp(sArgs[0], sArgs);
      perror("execve failed");
    }
    else // parent
    {
      pid_t c_pid;
      c_pid = fork();
      
      if (c_pid < 0) // fail
      {
        printf("failed to fork client for %s", tc.client.c_str());
        perror("");
        _exit(1);
      }
      else if (c_pid == 0) // child
      {
        char * cArgs[4] = { exe, "client", const_cast<char*>(tc.client.c_str()), NULL} ;
        execvp(cArgs[0], cArgs);
        perror("execve failed");
      }
      else // parent
      {
        int status;
        
        // parent waiting on client
        if( waitpid(c_pid, &status, 0) < 0)
        {
          perror("failed while waiting");
          _exit(1);
        }

        // done waiting for client
        if ( !WIFEXITED(status) )
        {
          printf("Parent: Child exited with status: %d\n", WEXITSTATUS(status));
          perror("client exited abnormally");
          // kill server before leaving
          kill(s_pid, SIGINT);
          _exit(1);
        }
          
        // time to kill server
        if ( kill(s_pid, SIGINT) < 0 )
        {
          perror("failed while killing server");
          _exit(1);
        } 
      } // parent inner
    } // parent outer
  }  // for loop
  return 0;
} // main