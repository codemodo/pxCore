#include "rtRemoteTestCommon.h"
#include <rtRemote.h>
#include <rtObject.h>
#include <rtValue.h>
#include <functional>

rtRemoteEnvironment* env = nullptr;
static rtObjectRef sObjRef, cObjRef;
static char const* objectName = "com.xfinity.xsmart.Thermostat.JakesHouse";

extern "C" void setUpClient()
{
  env = rtEnvironmentGetGlobal();
  rtRemoteInit(env);
  rtRemoteLocateObject(env, objectName, cObjRef);
}

extern "C" void setUpServer()
{
  env = rtEnvironmentGetGlobal();
  rtRemoteInit(env);
  sObjRef = new rtTestObject();
  rtRemoteRegisterObject(env, objectName, sObjRef);
}

extern "C" void tearDownClient()
{
  rtRemoteShutdown(env);
}

extern "C" void tearDownServer()
{
  if (sObjRef)
    delete sObjRef;
  rtRemoteShutdown(env);
}

extern "C" void
Test_Client_Set_Get_Bool()
{
  rtLogInfo("***TEST STARTED*** -> Test_Client_Set_Get_Bool");
  setUpClient();
  for (int i = 0; i < 1000; i++)
  {
    cObjRef.set("p_bool", false);
    bool ret = cObjRef.get<bool>("p_bool");
    RT_ASSERT(ret == false);

    cObjRef.set("p_bool", true);
    ret = cObjRef.get<bool>("p_bool");
    RT_ASSERT(ret == true);
  }
  tearDownClient();
  rtLogInfo("***TEST PASSED*** -> Test_Client_Set_Get_Bool");
}

extern "C" void
Test_Client_Set_Get_Int8()
{
  rtLogInfo("***TEST STARTED*** -> Test_Client_Set_Get_Int8");
  setUpClient();
  for (int i = 0; i < 1000; i++)
  {
    cObjRef.set("p_int8", i);
    int8_t ret = cObjRef.get<int8_t>("p_int8");
    RT_ASSERT(ret == static_cast<int8_t>(i));
  }
  tearDownClient();
  rtLogInfo("***TEST PASSED*** -> Test_Client_Set_Get_Int8");
}

extern "C" void
Test_Server_Set_Get_Generic()
{
  printf("\nhere4\n");
  setUpServer();
  while (true)
  {
    rtError e = rtRemoteRun(env, 5000);
  }
  tearDownServer();
}

int main (int argc, char* argv[])
{
  return 0;
}