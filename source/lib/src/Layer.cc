#include <stdio.h>
#include <inttypes.h>
#include <regex>
#include "Layer.hh"

//using namespace std::chrono_literals;

#ifndef DEBUG_PRINT
#define DEBUG_PRINT 0
#endif
#define debug_print(fmt, ...)                                           \
  do { if (DEBUG_PRINT) std::fprintf(stdout, fmt, ##__VA_ARGS__); } while (0)

#ifndef INFO_PRINT
#define INFO_PRINT 1
#endif
#define info_print(fmt, ...)                                           \
  do { if (INFO_PRINT) std::fprintf(stdout, fmt, ##__VA_ARGS__); } while (0)

Camera::Camera(){
  m_fw.reset(new FirmwarePortal("builtin"));
  m_rd.reset(new DataReader());
}

Camera::~Camera(){

  m_is_async_reading = false;
  if(m_fut_async_rd.valid())
    m_fut_async_rd.get();

  m_is_async_watching = false;
  if(m_fut_async_watch.valid())
    m_fut_async_watch.get();

}

void Camera::fw_start(){
  if(!m_fw) return;
  // m_fw->SetCisRegister("CMU_DMU_CONF", 0x70);// token
  // m_fw->SetCisRegister("CHIP_MODE", 0x3d); //trigger MODE
  // m_fw->SetFirmwareRegister("FIRMWARE_MODE", 1); //run, fw forward trigger
  info_print( " fw start \n");
}


void Camera::fw_stop(){
  if(!m_fw) return;
  // m_fw->SetFirmwareRegister("FIRMWARE_MODE", 0); // stop trigger, fw goes into configure mode 
  // m_fw->SetCisRegister("CHIP_MODE", 0x3c); // sensor goes to configure mode

  info_print(" fw stop \n");
}

void Camera::fw_conf(){
  if(!m_fw) return;
  info_print( " fw conf \n");

 
  if(!m_jsdoc_conf.HasMember("firmware")){
    std::fprintf(stderr, "JSON configure file error: no firmware section \n");
    throw;
  }
  const auto& js_fw_conf =  m_jsdoc_conf["firmware"];
  for(const auto &reg: js_fw_conf.GetObject()){
    m_fw->SetFirmwareRegister(reg.name.GetString(), reg.value.GetUint64());
  }

}

void Camera::fw_init(){
  if(!m_fw) return;  
  
  info_print("fw init \n");
}

void Camera::rd_start(){
  if(m_is_async_reading){
    std::fprintf(stderr, "old AsyncReading() has not been stopped\n");
    return;
  }

  m_fut_async_rd = std::async(std::launch::async, &Camera::AsyncPushBack, this);
  if(!m_is_async_watching){
    m_fut_async_watch = std::async(std::launch::async, &Camera::AsyncWatchDog, this);
  }
}

void Camera::rd_stop(){
  m_is_async_reading = false;
  if(m_fut_async_rd.valid())
    m_fut_async_rd.get();

  m_is_async_watching = false;
  if(m_fut_async_watch.valid())
    m_fut_async_watch.get();
}

uint64_t Camera::AsyncPushBack(){ // IMPROVE IT AS A RING
  m_vec_ring_ev.clear();
  m_vec_ring_ev.resize(m_size_ring);
  m_count_ring_write = 0;
  m_count_ring_read = 0;
  m_hot_p_read = m_size_ring -1; // tail

  uint32_t flag_wait_first_event = true;

  m_rd->Open();
  info_print(" rd start  \n");
  m_is_async_reading = true;

  m_st_n_ev_input_now =0;
  m_st_n_ev_output_now =0;
  m_st_n_ev_overflow_now =0;
  
  while (m_is_async_reading){
    auto df = m_rd? m_rd->Read(std::chrono::seconds(1)):nullptr; // TODO: read a vector
    if(!df){
      continue;
    }
    m_st_n_ev_input_now ++;

    uint64_t next_p_ring_write = m_count_ring_write % m_size_ring;
    if(next_p_ring_write == m_hot_p_read){
      // buffer full, permanent data lose
      m_st_n_ev_overflow_now ++;
      continue;
    }

    
    if(flag_wait_first_event){
      flag_wait_first_event = false;
    }

    m_vec_ring_ev[next_p_ring_write] = df;
    m_count_ring_write ++;
  }
  m_rd->Close();
  info_print(" rd stop  \n");
  return m_count_ring_write;
}

uint64_t Camera::AsyncWatchDog(){
  m_tp_run_begin = std::chrono::system_clock::now();
  m_tp_old = m_tp_run_begin;
  m_is_async_watching = true;

  m_st_n_ev_input_old = 0;
  m_st_n_ev_overflow_old = 0;
  
  while(m_is_async_watching){
    std::this_thread::sleep_for(std::chrono::seconds(1));
    uint64_t st_n_ev_input_now = m_st_n_ev_input_now;
    uint64_t st_n_ev_overflow_now = m_st_n_ev_overflow_now;

    // time
    auto tp_now = std::chrono::system_clock::now();
    std::chrono::duration<double> dur_period_sec = tp_now - m_tp_old;
    std::chrono::duration<double> dur_accu_sec = tp_now - m_tp_run_begin;
    double sec_period = dur_period_sec.count();
    double sec_accu = dur_accu_sec.count();

    // period
    uint64_t st_n_ev_input_period = st_n_ev_input_now - m_st_n_ev_input_old;
    uint64_t st_n_ev_overflow_period = st_n_ev_overflow_now - m_st_n_ev_overflow_old;
        
    // hz
    double st_hz_input_accu = st_n_ev_input_now / sec_accu ; 
    double st_hz_input_period = st_n_ev_input_period / sec_period ;

    std::string st_string_new =
      FirmwarePortal::FormatString(" event(%10" PRIu64 ")  ev_accu(%8.2f hz)  ev_period(%8.2f hz)",
				   st_n_ev_input_now,  st_hz_input_accu,  st_hz_input_period
                                   );
    
    {
      std::unique_lock<std::mutex> lk(m_mtx_st);
      m_st_string = std::move(st_string_new);
    }
    
    //write to old
    m_st_n_ev_input_old = st_n_ev_input_now;
    m_st_n_ev_overflow_old = st_n_ev_overflow_now;
    m_tp_old = tp_now;
  }
  return 0;
}

std::string  Camera::GetStatusString(){
  std::unique_lock<std::mutex> lk(m_mtx_st);
  return m_st_string;
}

DataFrameSP& Camera::Front(){
  if(m_count_ring_write > m_count_ring_read) {
    uint64_t next_p_ring_read = m_count_ring_read % m_size_ring;
    m_hot_p_read = next_p_ring_read;
    // keep hot read to prevent write-overlapping
    return m_vec_ring_ev[next_p_ring_read];
  }
  else{
    return m_ring_end;
  }
}

void Camera::PopFront(){
  if(m_count_ring_write > m_count_ring_read) {
    uint64_t next_p_ring_read = m_count_ring_read % m_size_ring;
    m_hot_p_read = next_p_ring_read;
    // keep hot read to prevent write-overlapping
    m_vec_ring_ev[next_p_ring_read].reset();
    m_count_ring_read ++;
  }
}

uint64_t Camera::Size(){
  return  m_count_ring_write - m_count_ring_read;
}

void Camera::ClearBuffer(){
  m_count_ring_write = m_count_ring_read;
  m_vec_ring_ev.clear();
}
