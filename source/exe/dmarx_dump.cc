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


#include "TFile.h"
#include "TTree.h"


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




TFile* tfile_createopen(const std::string& strFilePath){
  TFile *tf = nullptr;
  if(strFilePath.empty()){
    std::fprintf(stderr, "file path is not set\n\n");
    throw;
  }
  std::filesystem::path filepath(strFilePath);
  std::filesystem::path path_dir_output = std::filesystem::absolute(filepath).parent_path();
  std::filesystem::file_status st_dir_output = std::filesystem::status(path_dir_output);
  if (!std::filesystem::exists(st_dir_output)) {
    std::fprintf(stdout, "Output folder does not exist: %s\n\n",
		 path_dir_output.c_str());
    std::filesystem::file_status st_parent = std::filesystem::status(path_dir_output.parent_path());
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
    std::fprintf(stdout, "WARNING: File < %s > exists. Recreate\n\n", filepath.c_str());
  }

  tf = TFile::Open(filepath.c_str(),"recreate");
  // tf = TFile::Open("data.root","recreate");
  if (!tf) {
    std::fprintf(stderr, "File opening failed: %s \n\n", filepath.c_str());
    throw;
  }
  else{
    std::fprintf(stdout, "File opening sucess: %s \n\n", filepath.c_str());
  }
  return tf;
}


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
	  std::fprintf(stderr, "ERROR<%s>: wrong header of dataword (%s), shift one byte to be ", __func__, binToHexString((char*)(meas.data.raw8), size_filled).c_str());
	  MeasRaw::dropbyte(meas); //shift and remove a byte 
	  std::fprintf(stderr, "(%s)\n", binToHexString((char*)(meas.data.raw8), size_filled).c_str());
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
	      //std::fprintf(stderr, "INFO<%s>: no data receving.\n",  __func__);
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


  DataFrameSP Read(int fd_rx, const std::chrono::milliseconds &timeout_idle){ //timeout_read_interval 
    std::vector<MeasRaw> meas_col;
    DataFrameSP df;
    std::chrono::system_clock::time_point tp_timeout_idel;
    while(1){
      auto meas = readMeasRaw(fd_rx, tp_timeout_idel, timeout_idle);
      if(meas==0){
	return nullptr;
      }
      if(meas.isFrontMeasRaw()){
	meas_col.clear();
	meas_col.reserve(64*32);
      }
      meas_col.push_back(meas);

      if(meas.isEndMeasRaw()){
	if(meas_col.size()!=64*32){
	  std::fprintf(stderr, "ERROR: reach end package at size %d \n", meas_col.size());
	}
	break;
      }
      if(meas_col.size()==64*32){
      	std::fprintf(stderr, "ERROR: reach 64*32 packages, but no end%d \n");
      }
    }
    df = std::make_shared<DataFrame>(std::move(meas_col));
    return df;
  }

  DataFrameSP readdf(int fd_rx, const std::chrono::milliseconds &timeout_idel){ //timeout_read_interval
    // std::fprintf(stderr, "-");
    std::chrono::system_clock::time_point tp_timeout_idel;
    MeasRaw frontMeas;
    uint32_t skipN=0;
    while(1){
      MeasRaw aMeas = readMeasRaw(fd_rx, tp_timeout_idel, timeout_idel);
      if(aMeas.isFrontMeasRaw()){
	frontMeas = aMeas;
	break;
      }
      else if(aMeas.data.raw64==0){
	//error, or timeout.
	return nullptr;
      }
      else{ //none front meas;
	// std::fprintf(stderr, "INFO<%s>: skip none front meas:  %s\n",  __func__, binToHexString((char*)(aMeas.data.raw8),sizeof(aMeas.data)).c_str());
	skipN++;
	continue;
      }
    }
    if(skipN){
      std::fprintf(stderr, "INFO<%s>: skip none front meas size:  %d\n",  __func__, skipN);
    }
    
    const size_t packNumInFrame = 2048;
    std::vector<MeasRaw> meas_col(packNumInFrame, 0);
    meas_col[0] = frontMeas;
    MeasRaw* meas_col_p=&(meas_col[1]);
    constexpr size_t size_buf = sizeof(MeasRaw)*(packNumInFrame-1);
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
	  continue;
	}
	else{
	  if(std::chrono::system_clock::now() > tp_timeout_idel){ // timeout overflow reached
	    if(size_filled == 0){
	      std::fprintf(stderr, "ERROR<%s>: no data receving after front meas.\n",  __func__);
	      return nullptr;
	    }
	    else{
	      std::fprintf(stderr, "ERROR<%s>: timeout error of incomplete data reading after front meas (size = %d)\n", __func__ , size_filled);
	      break;
	    }
	  }
	  else{
	    continue;
	  }
	}
      }
      else{
	std::fprintf(stderr, "ERROR<%s>: read(...) returns error code %d\n", __func__,  errno);
	throw;
      }
    }
    meas_col.resize(size_filled/sizeof(MeasRaw)+1);
    if(!meas_col.back().isEndMeasRaw()){
      	std::fprintf(stderr, "\n\nERROR<%s>: the last pack is not the endMeasRaw\n\n", __func__);
	// for(const auto& mr : meas_col){
	//   std::fprintf(stdout, "%s    ", binToHexString((char*)(mr.data.raw8),sizeof(mr.data)).c_str());
	// }
      	std::fprintf(stderr, "\n\nERROR<%s>: \n number of MeasRaw: %d. recent broken dataframe is dropped\n\n", __func__, meas_col.size()); 
	return nullptr;
    }
    
    auto df = std::make_shared<DataFrame>(std::move(meas_col));
    return df;
  }
  
}


static const std::string help_usage = R"(
Usage:
  -help                        help message
  -verbose                     verbose flag
  -rootFile      <path>        path of ROOT TTree format file to save
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
  std::string rootFilePath;
  int exitTimeSecond = 30;
  bool do_rawPrint = false;
  bool do_formatPrint = false;

  int do_verbose = 0;
  {////////////getopt begin//////////////////
    struct option longopts[] = {{"help",      no_argument, NULL, 'h'},//option -W is reserved by getopt
                                {"verbose",   no_argument, NULL, 'v'},//val
                                {"rootFile",   required_argument, NULL, 'r'},
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
      case 'r':
        rootFilePath = optarg;
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



  TFile* tfx = nullptr;
  TTree* pixTree = nullptr;
  uint32_t frameN;
  uint8_t colN;
  uint8_t rowN;
  uint16_t adc;
  if(!rootFilePath.empty()){ 
    tfx = tfile_createopen(rootFilePath);
    if(tfx){
      pixTree = tfx->Get<TTree>("tree_pixel");
      if(!pixTree){
	pixTree = new TTree("tree_pixel","tree_pixel");
	// pixTree->SetDirectory(tf);
      }
    }
    pixTree->Branch("frameN", &frameN);
    pixTree->Branch("colN", &colN);
    pixTree->Branch("rowN", &rowN);
    pixTree->Branch("adc", &adc);
  }

  std::filesystem::path fsp_axidmard("/dev/axidmard");
  std::fprintf(stdout, " connecting to %s\n", fsp_axidmard.c_str());
  if (!std::filesystem::exists(std::filesystem::status(fsp_axidmard))){
    std::fprintf(stderr, "path %s does not exist. connection fail\n", fsp_axidmard.c_str());
    std::fprintf(stderr, "check if fpga-firmware and kernel-moulde is loaded\n", fsp_axidmard.c_str());
    throw;
  }
  int fd_rx = open(fsp_axidmard.c_str(), O_RDONLY | O_NONBLOCK);
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
  while(!g_done){
    if(exitTimeSecond && std::chrono::system_clock::now() > tp_timeout_exit){
      std::fprintf(stdout, "run %d seconds, nornal exit\n", exitTimeSecond);
      break;
    }
    auto df = readdf(fd_rx, std::chrono::seconds(1));
    if(!df){
      std::fprintf(stdout, "Data reveving timeout\n");
      continue;
    }
    std::fprintf(stdout, "\nframe %d:\n", dfN);
    if(do_formatPrint){
      df->Print(std::cout, 0);
      std::cout<<std::endl<<std::flush;
    }
    if(pixTree){
      for(const auto& [rc,v]: df->m_map_pos_adc){
	frameN = dfN;
	rowN= rc.first;
	colN = rc.second;
	adc = v;
	pixTree->Fill();
      }
      pixTree->Write("tree_pixel",TObject::kOverwrite);
    }

    if(do_rawPrint){
      for(const auto& mr : df->m_measraw_col){
	std::fprintf(stdout, "%s    ", binToHexString((char*)(mr.data.raw8),sizeof(mr.data)).c_str());
      }
      std::fprintf(stdout, "\n");
      std::fflush(stdout);
    }
    if(format_ofs.is_open()){
      df->Print(format_ofs, 0);
    }
    if(raw_fp){
      for(const auto& mr : df->m_measraw_col){
	std::fprintf(raw_fp, "%s    ", binToHexString((char*)(mr.data.raw8),sizeof(mr.data)).c_str());
      }
      std::fprintf(raw_fp, "\n");
    }
    dfN++;
  }
  
  close(fd_rx);
  if(raw_fp){
    std::fflush(raw_fp);
    std::fclose(raw_fp);
  }
  if(format_ofs.is_open()){
    format_ofs.close();
  }
  if(tfx){
    tfx->Close();
    delete tfx;
    tfx=nullptr;
    pixTree=nullptr;
  }
  
  g_done= 1;
  if(fut_async_watch.valid())
    fut_async_watch.get();
  
  return 0;
}
