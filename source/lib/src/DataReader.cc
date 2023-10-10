#include <cstdio>
#include <cctype>
#include <cstdlib>
#include <cstring>

#include <iostream>
#include <fstream>

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "myrapidjson.h"
#include "DataReader.hh"


#define HEADER_BYTE  (0b01010101)

namespace{
  std::string CStringToHexString(const char *bin, const int len){
    constexpr char hexmap[] = {'0', '1', '2', '3', '4', '5', '6', '7',
                               '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
    const unsigned char* data = (const unsigned char*)(bin);
    std::string s(len * 2, ' ');
    for (int i = 0; i < len; ++i) {
      s[2 * i]     = hexmap[(data[i] & 0xF0) >> 4];
      s[2 * i + 1] = hexmap[data[i] & 0x0F];
    }
    return s;
  }

  std::string StringToHexString(const std::string& bin){
    return CStringToHexString(bin.data(), bin.size());
  }

}


DataReader::~DataReader(){
  Close();
}

DataReader::DataReader(){
  m_fd = 0;
  m_file_path = "/dev/axidmard"; 
};


bool DataReader::Open(){
  m_fd = open(m_file_path.c_str(), O_RDONLY | O_NONBLOCK);
  if(!m_fd)
    return false;
  return true;
}

void DataReader::Close(){
  if(!m_fd)
    return;

  close(m_fd);
  m_fd = 0;
}

std::vector<DataFrameSP> DataReader::Read(size_t size_max_pkg,
                                           const std::chrono::milliseconds &timeout_idle,
                                           const std::chrono::milliseconds &timeout_total){
  std::chrono::system_clock::time_point tp_timeout_total = std::chrono::system_clock::now() + timeout_total;
  std::vector<DataFrameSP> pkg_v;
  while(1){
    DataFrameSP pkg = Read(timeout_idle);
    if(pkg){
      pkg_v.push_back(pkg);
      if(pkg_v.size()>=size_max_pkg){
        break;
      }
    }
    else{
      break; 
    }
    if(std::chrono::system_clock::now() > tp_timeout_total){
      break;
    }
  }
  return pkg_v;
}


MeasRaw readMeasRaw(int fd_rx, std::chrono::system_clock::time_point &tp_timeout_idel, const std::chrono::milliseconds &timeout_idel){ //timeout_read_interval
  // std::fprintf(stderr, "-");
  MeasRaw meas;
  size_t size_buf = sizeof(meas);
  size_t size_filled = 0;  
  bool can_time_out = false;
  int read_len_real = 0;
  while(size_filled < size_buf){
    read_len_real = read(fd_rx, &(meas.data.raw8[size_filled]), size_buf-size_filled);
    if(read_len_real>0){
      // debug_print(">>>read  %d Bytes \n", read_len_real);
      size_filled += read_len_real;
      can_time_out = false; // with data incomming, timeout counter is reset and stopped. 
      if(size_filled >=4 && meas.head() != HEADER_BYTE){	
	std::fprintf(stderr, "ERROR<%s>: wrong header of dataword (%s), shift one byte to be ", __func__, CStringToHexString((char*)(meas.data.raw8), size_filled).c_str());
	MeasRaw::dropbyte(meas); //shift and remove a byte 
	std::fprintf(stderr, "(%s)\n", CStringToHexString((char*)(meas.data.raw8), size_filled).c_str());
	size_filled -= 1;
	continue;
      }
    }
    else if (read_len_real== 0 || (read_len_real < 0 && errno == EAGAIN)){ // empty readback, read again
      if(!can_time_out){ // first hit here, if timeout counter was not yet started.
	can_time_out = true;
	tp_timeout_idel = std::chrono::system_clock::now() + timeout_idel; // start timeout counter
      }
      else{
	if(std::chrono::system_clock::now() > tp_timeout_idel){ // timeout overflow reached
	  if(size_filled == 0){
	    // debug_print("INFO<%s>: no data receving.\n",  __func__);
	    return MeasRaw(0);
	  }
	  std::fprintf(stderr, "ERROR<%s>: timeout error of incomplete data reading \n", __func__ );
	  std::fprintf(stderr, "=");
	  return MeasRaw(0);
	}
      }
      continue;
    }
    else{
      std::fprintf(stderr, "ERROR<%s>: read(...) returns error code %d\n", __func__,  errno);
      throw;
    }
  }
  return meas;
}


DataFrameSP DataReader::Read(const std::chrono::milliseconds &timeout_idle){ //timeout_read_interval  
  std::vector<MeasRaw> meas_col;
  while(1){
    auto meas = readMeasRaw(m_fd, tp_timeout_idel, timeout_idle);
    if(meas==0){
      return nullptr;
    }
    if(meas.isFrontMeasRaw()){
      meas_col.clear();
      meas_col.reserve(64*32);
    }
    meas_col.push_back(meas);    
    if(meas.isEndMeasRaw()){
      break;
    }
  }
  return std::make_shared<DataFrame>(std::move(meas_col));
}

std::string DataReader::LoadFileToString(const std::string& path){
  std::ifstream ifs(path);
  if(!ifs.good()){
    std::fprintf(stderr, "ERROR<%s>: unable to load file<%s>\n", __func__, path.c_str());
    throw;
  }
  std::string str;
  str.assign((std::istreambuf_iterator<char>(ifs) ),
             (std::istreambuf_iterator<char>()));
  return str;
}
