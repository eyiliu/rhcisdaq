#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <fstream>
#include <chrono>
#include <filesystem>
#include <future>

#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "getopt.h"
#include "DataFrame.hh"

#define HEADER_BYTE  (0b01010101)


using std::uint8_t;
using std::uint16_t;
using std::uint32_t;
using std::size_t;


#define DEBUG_PRINT 0

#define debug_print(fmt, ...)                                           \
  do { if (DEBUG_PRINT) std::fprintf(stdout, fmt, ##__VA_ARGS__); } while (0)


std::FILE* createfile_and_open(std::filesystem::path filepath){
  std::FILE *fp = nullptr;
  if(filepath.empty()){
    std::fprintf(stderr, "Empty filepath\n\n");
    throw;  
  }
  std::filesystem::path path_dir_output = std::filesystem::absolute(filepath).parent_path();
  std::filesystem::file_status st_dir_output =
    std::filesystem::status(path_dir_output);
  if (!std::filesystem::exists(st_dir_output)) {
    std::fprintf(stdout, "Output folder does not exist: %s\n\n",
		 path_dir_output.c_str());
    std::filesystem::file_status st_parent =
      std::filesystem::status(path_dir_output.parent_path());
    if (std::filesystem::exists(st_parent) &&
	std::filesystem::is_directory(st_parent)) {
      if (std::filesystem::create_directory(path_dir_output)) {
	std::fprintf(stdout, "Create output folder: %s\n\n", path_dir_output.c_str());
      } else {
	std::fprintf(stderr, "Unable to create folder: %s\n\n", path_dir_output.c_str());
	throw;
      }
    } else {
      std::fprintf(stderr, "Unable to create folder: %s\n\n", path_dir_output.c_str());
      throw;
    }
  }

  std::filesystem::file_status st_file = std::filesystem::status(filepath);
  if (std::filesystem::exists(st_file)) {
    std::fprintf(stderr, "File < %s > exists.\n\n", filepath.c_str());
    throw;
  }

  fp = std::fopen(filepath.c_str(), "w");
  if (!fp) {
    std::fprintf(stderr, "File opening failed: %s \n\n", filepath.c_str());
    throw;
  }
  return fp;
}


bool test_filepath(std::filesystem::path filepath){
  std::FILE *fp = nullptr;
  if(filepath.empty()){
    std::fprintf(stderr, "Empty filepath\n\n");
    throw;  
  }
  std::filesystem::path path_dir_output = std::filesystem::absolute(filepath).parent_path();
  std::filesystem::file_status st_dir_output =
    std::filesystem::status(path_dir_output);
  if (!std::filesystem::exists(st_dir_output)) {
    std::fprintf(stdout, "Output folder does not exist: %s\n\n",
		 path_dir_output.c_str());
    std::filesystem::file_status st_parent =
      std::filesystem::status(path_dir_output.parent_path());
    if (std::filesystem::exists(st_parent) &&
	std::filesystem::is_directory(st_parent)) {
      if (std::filesystem::create_directory(path_dir_output)) {
	std::fprintf(stdout, "Create output folder: %s\n\n", path_dir_output.c_str());
      } else {
	std::fprintf(stderr, "Unable to create folder: %s\n\n", path_dir_output.c_str());
	throw;
      }
    } else {
      std::fprintf(stderr, "Unable to create folder: %s\n\n", path_dir_output.c_str());
      throw;
    }
  }

  std::filesystem::file_status st_file = std::filesystem::status(filepath);
  if (std::filesystem::exists(st_file)) {
    std::fprintf(stderr, "File < %s > exists.\n\n", filepath.c_str());
    throw;
  }
  return true;
}




static sig_atomic_t g_done = 0;

std::atomic<size_t> ga_unexpectedN = 0;
std::atomic<size_t> ga_dataFrameN = 0;
std::atomic<uint16_t>  ga_lastTriggerId = 0;

uint64_t AsyncWatchDog(){
  auto tp_run_begin = std::chrono::system_clock::now();
  auto tp_old = tp_run_begin;
  size_t st_old_dataFrameN = 0;
  size_t st_old_unexpectedN = 0;
  
  while(!g_done){
    std::this_thread::sleep_for(std::chrono::seconds(1));
    auto tp_now = std::chrono::system_clock::now();
    std::chrono::duration<double> dur_period_sec = tp_now - tp_old;
    std::chrono::duration<double> dur_accu_sec = tp_now - tp_run_begin;
    double sec_period = dur_period_sec.count();
    double sec_accu = dur_accu_sec.count();
  }
  return 0;
}


namespace{
  std::string binToHexString(const char *bin, int len){
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

  std::string hexToBinString(const char *hex, int len){
    if(len%2){
      throw;
    }
    size_t blen  = len/2;
    const unsigned char* data = (const unsigned char*)(hex);
    std::string s(blen, ' ');
    for (int i = 0; i < blen; ++i){
      unsigned char d0 = data[2*i];
      unsigned char d1 = data[2*i+1];
      unsigned char v0;
      unsigned char v1;
      if(d0>='0' && d0<='9'){
        v0 = d0-'0';
      }
      else if(d0>='a' && d0<='f'){
        v0 = d0-'a'+10;
      }
      else if(d0>='A' && d0<='F'){
        v0 = d0-'A'+10;
      }
      else{
        std::fprintf(stderr, "wrong hex string\n");
        throw;
      }
      if(d1>='0' && d1<='9'){
        v1 = d1-'0';
      }
      else if(d1>='a' && d1<='f'){
        v1 = d1-'a'+10;
      }
      else if(d1>='A' && d1<='F'){
        v1 = d1-'A'+10;
      }
      else{
        std::fprintf(stderr, "wrong hex string\n");
        throw;
      }
      s[i]= (v0<<4) + v1;
    }
    return s;
  }

  std::string binToHexString(const std::string& bin){
    return binToHexString(bin.data(), bin.size());
  }

  std::string hexToBinString(const std::string& hex){
    return hexToBinString(hex.data(), hex.size());
  }

  
  int64_t readtest(int fd_rx, std::chrono::system_clock::time_point &tp_timeout_idel, const std::chrono::milliseconds &timeout_idel, uint32_t dfN){ //timeout_read_interval
    // std::fprintf(stderr, "-");
    const size_t packNumInFrame = 2048;
    std::array<MeasRaw, packNumInFrame> meas_col;
    constexpr size_t size_buf = sizeof(MeasRaw)*packNumInFrame;
    MeasRaw* meas_col_p=meas_col.data();
    unsigned char* meas_col_buffer_rx_p = reinterpret_cast<unsigned char*>(meas_col_p);
    size_t size_filled = 0;  
    bool can_time_out = false;
    int read_len_real = 0;
    while(size_filled < size_buf){
      read_len_real = read(fd_rx, meas_col_buffer_rx_p+size_filled, size_buf-size_filled);
      if(read_len_real>0){
	// debug_print(">>>read  %d Bytes \n", read_len_real);
	size_filled += read_len_real;
	can_time_out = false; // with data incomming, timeout counter is reset and stopped. 
      }
      else if (read_len_real== 0 || (read_len_real < 0 && errno == EAGAIN)){ // empty readback, read again
	if(!can_time_out){ // first hit here, if timeout counter was not yet started.
	  can_time_out = true;
	  tp_timeout_idel = std::chrono::system_clock::now() + timeout_idel; // start timeout counter
	}
	else{
	  if(std::chrono::system_clock::now() > tp_timeout_idel){ // timeout overflow reached
	    if(size_filled == 0){
	      std::fprintf(stderr, "INFO<%s>: no data receving.\n",  __func__);
	      return 0;
	    }
	    std::fprintf(stderr, "ERROR<%s>: timeout error of incomplete data reading (size = %d)\n", __func__ , size_filled);
	    break;
	  }
	}
	continue;
      }
      else{
	std::fprintf(stderr, "ERROR<%s>: read(...) returns error code %d\n", __func__,  errno);
	throw;
      }
    }

    if(size_filled){
      int measN=0;
      std::vector<uint32_t> vfrontN;
      std::vector<uint32_t> vendN;
      std::fprintf(stdout, "\nframe %d:\n", dfN);
      for(const auto &mr: meas_col){
	if(mr.isFrontMeasRaw()){
	  vfrontN.push_back(measN);
	}
	else if(mr.isEndMeasRaw()){
	  vendN.push_back(measN);
	}
	// if( (measN*sizeof(MeasRaw)<size_filled) && mr.data.raw64){
	//   std::fprintf(stdout, "%s    ", binToHexString((char*)(mr.data.raw8),sizeof(mr.data)).c_str());
	// }
	if((measN*sizeof(MeasRaw)<size_filled)){
	  measN++;
	}
      }
      
      std::fprintf(stdout, "\nframe %d has %d measraw\n\n", dfN, measN);
      for(const auto &fN: vfrontN){
	std::fprintf(stdout, "front meas at position %d\n", fN);
      }
      for(const auto &eN: vendN){
	std::fprintf(stdout, "end meas at position %d\n", eN);
      }
    }
    return size_filled;
  }
}


static const std::string help_usage = R"(
Usage:
  -help                        help message
  -verbose                     verbose flag
  -rawPrint                    print data by hex format in terminal
  -rawFile        <path>       path of raw file to save
  -formatPrint                 print data by json format in terminal
  -formatFile     <path>       path of json format file to save
  -exitTime       <n>          exit after n seconds (0=NoLimit, default 30)

examples:
#1. save raw data and print
./rhcisdump -rawPrint -rawFile test.dat

#2. save raw data only
./rhcisdump -rawFile test.dat

#3. print raw data only
./rhcisdump -rawPrint

#4. print json format data only
./rhcisdump -formatPrint

#5. print json format data and save format data
./rhcisdump -formatPrint -formateFile testfile.json

#6. print, exit after 60 seconds
./rhcisdump -rawPrint -exitTime 60


)";

int main(int argc, char *argv[]) {
  signal(SIGINT, [](int){g_done+=1;});

  std::string rawFilePath;
  std::string formatFilePath;
  std::string ipAddressStr;
  int exitTimeSecond = 30;
  bool do_rawPrint = false;
  bool do_formatPrint = false;

  int do_verbose = 0;
  {////////////getopt begin//////////////////
    struct option longopts[] = {{"help",      no_argument, NULL, 'h'},//option -W is reserved by getopt
                                {"verbose",   no_argument, NULL, 'v'},//val
                                {"rawPrint",  no_argument, NULL, 's'},
                                {"rawFile",   required_argument, NULL, 'f'},
                                {"formatPrint",   required_argument, NULL, 'S'},
                                {"formatFile",   required_argument, NULL, 'F'},
                                {"exitTime",  required_argument, NULL, 'e'},
                                {0, 0, 0, 0}};

    if(argc == 1){
      std::fprintf(stderr, "%s\n", help_usage.c_str());
      std::exit(1);
    }
    int c;
    int longindex;
    opterr = 1;
    while ((c = getopt_long_only(argc, argv, "-", longopts, &longindex)) != -1) {
      // // "-" prevents non-option argv
      // if(!optopt && c!=0 && c!=1 && c!=':' && c!='?'){ //for debug
      //   std::fprintf(stdout, "opt:%s,\targ:%s\n", longopts[longindex].name, optarg);;
      // }
      switch (c) {
      case 'e':
        exitTimeSecond = std::stoi(optarg);
        break;
      case 'f':
        rawFilePath = optarg;
        break;
      case 's':
        do_rawPrint = true;
        break;
      case 'F':
        formatFilePath = optarg;
        break;
      case 'S':
        do_formatPrint = true;
        break;
	// help and verbose
      case 'v':
        do_verbose=1;
        //option is set to no_argument
        if(optind < argc && *argv[optind] != '-'){
          do_verbose = std::stoul(argv[optind]);
          optind++;
        }
        break;
      case 'h':
        std::fprintf(stdout, "%s\n", help_usage.c_str());
        std::exit(0);
        break;
        /////generic part below///////////
      case 0:
        // getopt returns 0 for not-NULL flag option, just keep going
        break;
      case 1:
        // If the first character of optstring is '-', then each nonoption
        // argv-element is handled as if it were the argument of an option
        // with character code 1.
        std::fprintf(stderr, "%s: unexpected non-option argument %s\n",
                     argv[0], optarg);
        std::exit(1);
        break;
      case ':':
        // If getopt() encounters an option with a missing argument, then
        // the return value depends on the first character in optstring:
        // if it is ':', then ':' is returned; otherwise '?' is returned.
        std::fprintf(stderr, "%s: missing argument for option %s\n",
                     argv[0], longopts[longindex].name);
        std::exit(1);
        break;
      case '?':
        // Internal error message is set to print when opterr is nonzero (default)
        std::exit(1);
        break;
      default:
        std::fprintf(stderr, "%s: missing getopt branch %c for option %s\n",
                     argv[0], c, longopts[longindex].name);
        std::exit(1);
        break;
      }
    }
  }/////////getopt end////////////////


  std::fprintf(stdout, "\n");
  std::fprintf(stdout, "rawPrint:  %d\n", do_rawPrint);
  std::fprintf(stdout, "rawFile:   %s\n", rawFilePath.c_str());
  std::fprintf(stdout, "formatPrint:  %d\n", do_formatPrint);
  std::fprintf(stdout, "formatFile:   %s\n", formatFilePath.c_str());
  std::fprintf(stdout, "\n");


  std::FILE *raw_fp = nullptr;
  if(!rawFilePath.empty()){
    raw_fp=createfile_and_open(rawFilePath);
  }

  std::ofstream format_ofs;
  if(!formatFilePath.empty()){
    test_filepath(formatFilePath);
    format_ofs.open(formatFilePath.c_str(), std::ofstream::out | std::ofstream::app);
  }

  std::filesystem::path fsfp_axidmard("/dev/axidmard");
  std::fprintf(stdout, " connecting to %s\n", fsfp_axidmard.c_str());
  if (!std::filesystem::exists(std::filesystem::status(fsfp_axidmard))){
    std::fprintf(stderr, "path %s does not exist. connection fail\n", fsfp_axidmard.c_str());
    std::fprintf(stderr, "check if fpga-firmware and kernel-moulde is loaded\n", fsfp_axidmard.c_str());
    throw;
  }
  
  int fd_rx = open(fsfp_axidmard.c_str(), O_RDONLY | O_NONBLOCK);
  if(!fd_rx){
    std::fprintf(stdout, " connection fail\n");
    throw;
  }
  std::fprintf(stdout, " connected\n");
  
  std::chrono::system_clock::time_point tp_timeout_exit  = std::chrono::system_clock::now() + std::chrono::seconds(exitTimeSecond);

  std::future<uint64_t> fut_async_watch;
  fut_async_watch = std::async(std::launch::async, &AsyncWatchDog);
  std::chrono::system_clock::time_point tp_timeout;
  uint32_t dfN=0;
  std::chrono::system_clock::time_point tp_readtest_timeout;
  char* buffer[8*2048];
  size_t readback_len= 0;
  while(!g_done){
    if(exitTimeSecond && std::chrono::system_clock::now() > tp_timeout_exit){
      std::fprintf(stdout, "run %d seconds, nornal exit\n", exitTimeSecond);
      break;
    }

    if(0){
      auto a_readback_len= read(fd_rx, buffer, 8*2048);
      if(a_readback_len>0){
	readback_len += a_readback_len;
      }
      continue;
    }
    
    auto re = readtest(fd_rx, tp_readtest_timeout, std::chrono::seconds(1), dfN);
    if(re==0){
      std::fprintf(stdout, "Data reveving timeout\n");
      continue;
    }
    // std::fprintf(stdout, "\nframe %d:\n", dfN);
    // if(do_rawPrint){
    //   for(const auto& mr : df->m_measraw_col){
    // 	std::fprintf(stdout, "%s    ", binToHexString((char*)(mr.data.raw8),sizeof(mr.data)).c_str());
    //   }
    //   std::fprintf(stdout, "\n");
    //   std::fflush(stdout);
    // }
    // if(raw_fp){
    //   for(const auto& mr : df->m_measraw_col){
    // 	std::fprintf(raw_fp, "%s    ", binToHexString((char*)(mr.data.raw8),sizeof(mr.data)).c_str());
    //   }
    //   std::fprintf(raw_fp, "\n");
    // }    
    dfN++;
  }

  std::fprintf(stdout, "\n this run gets data size %d bytes, %d word32,  %d frames \n\n",  readback_len, readback_len/4, readback_len/(2048*8));
  
  close(fd_rx);
  if(raw_fp){
    std::fflush(raw_fp);
    std::fclose(raw_fp);
  }
  if(format_ofs.is_open()){
    format_ofs.close();
  }

  g_done= 1;
  if(fut_async_watch.valid())
    fut_async_watch.get();
  
  return 0;
}

