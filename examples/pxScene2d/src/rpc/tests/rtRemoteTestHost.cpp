#include "rtRemoteTestCommon.h"

rtRemoteEnvironment* env = nullptr;

int main (int argc, char* argv[])
{
  // first arg is this executable name
  // second arg is client/server
  // third arg is func name
  // potentially func params after that?
  return 0;
}

void
Test_Client_Set_Get_Bool()
{
  rtLogInfo("***TEST STARTED*** -> Test_Client_Set_Get_Bool");
  for (int i = 0; i < 1000; i++)
  {
    cObjRef.set("p_bool", false);
    bool ret = cObjRef.get<bool>("p_bool");
    RT_ASSERT(ret == false);

    cObjRef.set("p_bool", true);
    ret = cObjRef.get<bool>("p_bool");
    RT_ASSERT(ret == true);
  }
  rtLogInfo("***TEST PASSED*** -> Test_Client_Set_Get_Bool");
}

void
Test_Client_Set_Get_Int8()
{
  rtLogInfo("***TEST STARTED*** -> Test_Client_Set_Get_Int8");
  for (int i = 0; i < 1000; i++)
  {
    cObjRef.set("p_int8", i);
    int8_t ret = cObjRef.get<int8_t>("p_int8");
    RT_ASSERT(ret == static_cast<int8_t>(i));
  }
  rtLogInfo("***TEST PASSED*** -> Test_Client_Set_Get_Int8");
}

void
Test_Server_Set_Get_Generic()
{
  bool conn = false;
  while (true)
  {
    rtError e = rtRemoteRun(env, 5000);
    if (clientCount(env) > 0)
      conn = true;
    if (conn && clientCount(env) == 0)
      break;
  }

}