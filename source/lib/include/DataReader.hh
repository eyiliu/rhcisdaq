#ifndef _DATA_READER_WS_
#define _DATA_READER_WS_

#include <string>
#include <vector>
#include <chrono>

#include "mysystem.hh"
#include "DataFrame.hh"

class DataReader{
public:
  ~DataReader();
  DataReader();

  DataFrameSP Read(const std::chrono::milliseconds &timeout);

  // std::vector<DataFrameSP> Read(size_t size_max_pkg,
  //                               const std::chrono::milliseconds &timeout_idle,
  //                               const std::chrono::milliseconds &timeout_total);

  bool Open();
  void Close();

  static std::string LoadFileToString(const std::string& path);

private:
  int m_fd{0};
  std::chrono::system_clock::time_point tp_timeout_idel;
  std::string m_file_path;
  JsonDocument m_jsdoc_conf;

  
  
};

#endif
