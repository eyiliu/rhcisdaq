#ifndef _FIRMWARE_PORTAL_HH_
#define _FIRMWARE_PORTAL_HH_

#include <string>
#include <iostream>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <chrono>
#include <thread>
#include <ctime>
#include <regex>
#include <map>
#include <vector>
#include <utility>
#include <algorithm>

#include "mysystem.hh"
#include "myrapidjson.h"

class FirmwarePortal{
public:
  FirmwarePortal(const std::string &json_str_options);

  void SetFirmwareRegister(const std::string& name, uint64_t value);
  uint64_t GetFirmwareRegister(const std::string& name);

  static std::string LoadFileToString(const std::string& path);
  
private:

  void DeviceOpen();
  void DeviceClose();
  int   m_fd{0};
  char* m_virt_addr_base{0};
  char* m_virt_addr_end{0};
  
public:

  template <typename T>
  void Write(const uint64_t &offset, const T &value){
    *((T*)(m_virt_addr_base + offset))=value;
  }

  template <typename T>
  T Read(const uint64_t &offset){
    T* virt_addr = reinterpret_cast<T*>(m_virt_addr_base + offset);
    return  *virt_addr;
  }
  
  template<typename ... Args>
  static std::string FormatString( const std::string& format, Args ... args ){
    std::size_t size = snprintf( nullptr, 0, format.c_str(), args ... ) + 1;
    std::unique_ptr<char[]> buf( new char[ size ] ); 
    std::snprintf( buf.get(), size, format.c_str(), args ... );
    return std::string( buf.get(), buf.get() + size - 1 );
  }

  template<typename ... Args>
  static std::size_t FormatPrint(std::ostream &os, const std::string& format, Args ... args ){
    std::size_t size = snprintf( nullptr, 0, format.c_str(), args ... ) + 1;
    std::unique_ptr<char[]> buf( new char[ size ] ); 
    std::snprintf( buf.get(), size, format.c_str(), args ... );
    std::string formated_string( buf.get(), buf.get() + size - 1 );
    os<<formated_string<<std::flush;
    return formated_string.size();
  }

  template<typename ... Args>
  static std::size_t DebugFormatPrint(std::ostream &os, const std::string& format, Args ... args ){
    //    return 0;
    std::size_t size = snprintf( nullptr, 0, format.c_str(), args ... ) + 1;
    std::unique_ptr<char[]> buf( new char[ size ] ); 
    std::snprintf( buf.get(), size, format.c_str(), args ... );
    std::string formated_string( buf.get(), buf.get() + size - 1 );
    os<<formated_string<<std::flush;
    return formated_string.size();
  }


  
  template<typename T>
  static const std::string Stringify(const T& o){
    rapidjson::StringBuffer sb;
    rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
    o.Accept(writer);
    return sb.GetString();
  }

  template<typename T>
  static void PrintJson(const T& o){
    rapidjson::StringBuffer sb;
    rapidjson::PrettyWriter<rapidjson::StringBuffer> w(sb);
    o.Accept(w);
    std::fwrite(sb.GetString(), 1, sb.GetSize(), stdout);
  }

  rapidjson::CrtAllocator m_jsa;
  rapidjson::GenericValue<rapidjson::UTF8<char>, rapidjson::CrtAllocator> m_js_conf;
  rapidjson::GenericValue<rapidjson::UTF8<char>, rapidjson::CrtAllocator> m_js_reg_cmd;  
  rapidjson::Document m_json;
};


#endif
