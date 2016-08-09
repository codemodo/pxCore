#include "rtRemoteTestCommon.h"
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

std::vector<TestCase>
buildTestCaseMap()
{
  std::vector< TestCase > v;
  v.push_back( TestCase("Test_Client_Set_Get_Bool", "Test_Server_Set_Get_Generic") );
  v.push_back( TestCase("Test_Client_Set_Get_Int8", "Test_Server_Set_Get_Generic") );
  return v;
}

int main (int argc, char* /*argv*/[])
{
  char * exe = "testHost";
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
      char * sArgs[4] = { exe, "server", tc.server, NULL} ;
      execvp(sArgs[0], sArgs);
      perror("execve failed");
    }
    else // parent
    {
      pid_t c_pid;
      c_pid = fork();
      
      if (c_pid < 0) // fail
      {
        perror("failed to fork client for %s", tc.client);
        _exit(1);
      }
      else if (c_pid == 0) // child
      {
        char * cArgs[4] = { exe, "client", tc.client, NULL} ;
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
          kill(s_pid, sig);
          _exit(1);
        }
          
        // time to kill server
        if ( kill(s_pid, sig) < 0 )
        {
          perror("failed while killing server");
          _exit(1);
        } 
      } // parent inner
    } // parent outer
  }  // for loop
  return 0;
} // main