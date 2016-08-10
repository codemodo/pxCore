#include <rtRemote.h>
#include <rtObject.h>
#include <rtValue.h>
#include <functional>

#include <unistd.h>

//typedef void (*testFunction_t)();

struct TestCase
{
  TestCase(std::string c, std::string s)
    : client(c), server(s)
  { }
  std::string client;
  std::string server;
};

// // Client function declarations
// void Test_Client_Set_Get_Bool();
// void Test_Client_Set_Get_Int8();
// void Test_Client_Set_Get_Uint8();

// // Server function declarations
// void Test_Server_Set_Get_Generic();

class rtTestObject : public rtObject
{
public:
  rtDeclareObject(rtTestObject, rtObject);
  rtProperty(p_bool, get_p_bool, set_p_bool, bool);
  rtProperty(p_int8, get_p_int8, set_p_int8, int8_t);
  rtProperty(p_uint8, get_p_uint8, set_p_uint8, uint8_t);

  bool get_p_bool() const { return m_p_bool; }
  rtError  get_p_bool(bool& s) const { s = m_p_bool; return RT_OK; }
  rtError  set_p_bool(bool s) { m_p_bool = s; return RT_OK; }

  bool get_p_int8() const { return m_p_int8; }
  rtError  get_p_int8(int8_t& s) const { s = m_p_int8; return RT_OK; }
  rtError  set_p_int8(int8_t s) { m_p_int8 = s; return RT_OK; }

  bool get_p_uint8() const { return m_p_uint8; }
  rtError  get_p_uint8(uint8_t& s) const { s = m_p_uint8; return RT_OK; }
  rtError  set_p_uint8(uint8_t s) { m_p_uint8 = s; return RT_OK; }

private:
  bool    m_p_bool;
  int8_t  m_p_int8;
  uint8_t m_p_uint8;
};

rtDefineObject(rtTestObject, rtObject);
rtDefineProperty(rtTestObject, p_bool);
rtDefineProperty(rtTestObject, p_int8);
rtDefineProperty(rtTestObject, p_uint8);