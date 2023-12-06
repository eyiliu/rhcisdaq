#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <iostream>

#include <signal.h>

#include "getopt.h"
#include "linenoise.h"

#include "Camera.hh"

template<typename ... Args>
static std::string FormatString( const std::string& format, Args ... args ){
  std::size_t size = std::snprintf( nullptr, 0, format.c_str(), args ... ) + 1;
  std::unique_ptr<char[]> buf( new char[ size ] );
  std::snprintf( buf.get(), size, format.c_str(), args ... );
  return std::string( buf.get(), buf.get() + size - 1 );
}

namespace{
  std::string LoadFileToString(const std::string& path){
    std::ifstream ifs(path);
    if(!ifs.good()){
      std::cerr<<"LoadFileToString:: ERROR, unable to load file<"<<path<<">\n";
      throw;
    }

    std::string str;
    str.assign((std::istreambuf_iterator<char>(ifs) ),
               (std::istreambuf_iterator<char>()));
    return str;
  }
}


static  const std::string help_usage
(R"(
Usage:
-h : print usage information, and then quit

'help' command in interactive mode provides detail usage information
)"
  );


static  const std::string help_usage_linenoise
(R"(

keyword: help, info, quit, sensor, firmware, set, get, init, start, stop, reset, regcmd
example:
  A) init  (set firmware and sensosr to ready-run state after power-cirlce)
   > init

  B) start (set firmware and sensosr to running state from ready-run state)
   > start

  C) stop  (set firmware and sensosr to stop-run state)
   > stop

  D) scan  (scan once)
   > scan

  1) get firmware regiester
   > firmware get FW_REG_NAME

  2) set firmware regiester
   > firmware set FW_REG_NAME 10

  3) exit/quit command line
   > quit

  4) set adc changel 0 clock alignment register to 1 
   > set firmware ADC_ADJUST_CH0 1

  5) save data file into TTree ROOT file 
   > file data /path/to/root/data/filename.root

)"
  );

struct DummyDump;

int main(int argc, char **argv){
  std::string c_opt;
  int c;
  while ( (c = getopt(argc, argv, "h")) != -1) {
    switch (c) {
    case 'h':
      fprintf(stdout, "%s", help_usage.c_str());
      return 0;
      break;
    default:
      fprintf(stderr, "%s", help_usage.c_str());
      return 1;
    }
  }

  ///////////////////////
  std::unique_ptr<Camera> cam;
  std::unique_ptr<DummyDump> dummyDump;

  auto history_file_path = std::filesystem::temp_directory_path();
  history_file_path /=".regctrl.history"; 
  linenoiseHistoryLoad(history_file_path.c_str());
  linenoiseSetCompletionCallback([](const char* prefix, linenoiseCompletions* lc)
                                   {
                                     static const char* examples[] =
                                       {"help", "quit", "exit", "info",
                                        "init", "start", "stop","scan",
                                        "firmware", "set", "get",
                                        "dump", "file",
                                        NULL};
                                     size_t i;
                                     for (i = 0;  examples[i] != NULL; ++i) {
                                       if (strncmp(prefix, examples[i], strlen(prefix)) == 0) {
                                         linenoiseAddCompletion(lc, examples[i]);
                                       }
                                     }
                                   } );


  const char* prompt = "\x1b[1;32mregctrl\x1b[0m> ";
  while (1) {
    char* result = linenoise(prompt);
    if (result == NULL) {
      if(linenoiseKeyType()==1){
        if(cam){
          printf("ctrl: stopping\n");
          cam->fw_stop();
          cam->rd_stop();
          printf("ctrl: done\n");
        }
        continue;
      }
      break;
    }
    if ( std::regex_match(result, std::regex("\\s*((?:quit)|(?:exit))\\s*")) ){
      printf("quiting \n");
      linenoiseHistoryAdd(result);
      free(result);
      break;
    }

    if ( std::regex_match(result, std::regex("\\s*(help)\\s*")) ){
      fprintf(stdout, "%s", help_usage_linenoise.c_str());
    }
    else if ( std::regex_match(result, std::regex("\\s*(init)\\s*")) ){
      printf("ctrl: initializing\n");
      dummyDump.reset();
      cam.reset(new Camera());
      cam->fw_init();
      printf("ctrl: done\n");
    }
    else if ( std::regex_match(result, std::regex("\\s*(scan)\\s*")) ){
      if(cam){
	printf("ctrl: scaning once\n");
	cam->m_skip_push=1;
	cam->m_df_print=1;	
	cam->rd_start();
        cam->fw_start();
	std::this_thread::sleep_for(std::chrono::milliseconds(100));
        cam->fw_stop();
        cam->rd_stop();
      }
    }
    else if ( std::regex_match(result, std::regex("\\s*(start)\\s*")) ){
      if(cam){
        printf("ctrl: starting\n");
        cam->rd_start();
        cam->fw_start();
        printf("ctrl: done\n");
      }
    }
    else if ( std::regex_match(result, std::regex("\\s*(stop)\\s*")) ){
      if(cam){
        printf("ctrl: stopping\n");
        cam->fw_stop();
        cam->rd_stop();
        printf("ctrl: done\n");
      }
    }
    else if ( std::regex_match(result, std::regex("\\s*(info)\\s*"))){
      if(cam)
        std::cout<<cam->GetStatusString()<<std::endl;
    }
    else if ( std::regex_match(result, std::regex("\\s*(dump)\\s+(start)\\s*"))){
      if(cam)
        dummyDump = std::make_unique<DummyDump>(cam.get());
    }
    else if ( std::regex_match(result, std::regex("\\s*(dump)\\s+(stop)\\s*"))){
      dummyDump.reset();
    }
    else if ( std::regex_match(result, std::regex("\\s*(firmware)\\s+(set)\\s+(\\w+)\\s+(?:(0[Xx])?([0-9]+))\\s*")) ){
      std::cmatch mt;
      std::regex_match(result, mt, std::regex("\\s*(firmware)\\s+(set)\\s+(\\w+)\\s+(?:(0[Xx])?([0-9]+))\\s*"));
      std::string name = mt[3].str();
      if(cam && cam->m_fw){
        uint64_t value = std::stoull(mt[5].str(), 0, mt[4].str().empty()?10:16);
        cam->m_fw->SetFirmwareRegister(name, value);
      }
    }
    else if ( std::regex_match(result, std::regex("\\s*(firmware)\\s+(get)\\s+(\\w+)\\s*")) ){
      std::cmatch mt;
      std::regex_match(result, mt, std::regex("\\s*(firmware)\\s+(get)\\s+(\\w+)\\s*"));
      std::string name = mt[3].str();
      if(cam &&cam->m_fw){
        uint64_t value = cam->m_fw->GetFirmwareRegister(name);
        fprintf(stderr, "%s = %llu, %#llx\n", name.c_str(), value, value);
      }
    }
    else if ( std::regex_match(result, std::regex("\\s*(file)\\s+(data)\\s+(\\w+)\\s*")) ){
      std::cmatch mt;
      std::regex_match(result, mt, std::regex("\\s*(file)\\s+(data)\\s+(\\w+)\\s*"));
      std::string datafilepath = mt[3].str();
      //TODO test path 
      // if(cam &&cam->m_fw){
	

      // }
    }
    else{
      std::fprintf(stderr, "unknown command<%s>! consult possible commands by help....\n", result);
      linenoisePreloadBuffer("help");
    }

    linenoiseHistoryAdd(result);
    linenoiseHistorySave(history_file_path.c_str());
    free(result);
  }
  //linenoiseHistorySave(history_file_path.c_str());
  linenoiseHistoryFree();

  printf("reset signal from main thread.\n\n");
  dummyDump.reset();
  cam.reset();

  return 0;
}


struct DummyDump{
  std::future<uint64_t> fut;
  bool isRunning;

  DummyDump() = delete;
  DummyDump(const DummyDump&) =delete;
  DummyDump& operator=(const DummyDump&) =delete;
  DummyDump(Camera *cam){
    isRunning = true;
    fut = std::async(std::launch::async, &DummyDump::AsyncDump, &isRunning, cam);
  }
  ~DummyDump(){
    if(fut.valid()){
      isRunning = false;
      fut.get();
    }
  }

  static uint64_t AsyncDump(bool* isDumping, Camera* cam){
    auto now = std::chrono::system_clock::now();
    auto now_c = std::chrono::system_clock::to_time_t(now);
    // std::string now_str = TimeNowString("%y%m%d%H%M%S");

    uint64_t n_ev = 0;
    *isDumping = true;
    while (*isDumping){
      auto &ev_front = cam->Front();
      if(ev_front){
        ev_front->Print(std::cout,0);
        cam->PopFront();
        n_ev++;
      }
      else{
        std::this_thread::sleep_for(std::chrono::microseconds(1));
        continue;
      }
    }
    printf("AsyncDump exited\n");
    return n_ev;
  }

};
