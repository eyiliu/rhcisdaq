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

Layer::Layer(){
  m_fw.reset(new FirmwarePortal("builtin"));
  m_rd.reset(new DataReader());
}

Layer::~Layer(){


  m_is_async_reading = false;
  if(m_fut_async_rd.valid())
    m_fut_async_rd.get();

  m_is_async_watching = false;
  if(m_fut_async_watch.valid())
    m_fut_async_watch.get();

}

void Layer::fw_start(){
  if(!m_fw) return;
  // m_fw->SetCisRegister("CMU_DMU_CONF", 0x70);// token
  // m_fw->SetCisRegister("CHIP_MODE", 0x3d); //trigger MODE
  // m_fw->SetFirmwareRegister("FIRMWARE_MODE", 1); //run, fw forward trigger
  info_print( " fw start %s\n", m_fw->DeviceUrl().c_str());
}


void Layer::fw_stop(){
  if(!m_fw) return;
  // m_fw->SetFirmwareRegister("FIRMWARE_MODE", 0); // stop trigger, fw goes into configure mode 
  // m_fw->SetCisRegister("CHIP_MODE", 0x3c); // sensor goes to configure mode

  info_print(" fw stop  %s\n", m_fw->DeviceUrl().c_str());
}

void Layer::fw_conf(){
  if(!m_fw) return;
  info_print( " fw conf %s\n", m_fw->DeviceUrl().c_str());

 
  if(!m_jsdoc_conf.HasMember("firmware")){
    std::fprintf(stderr, "JSON configure file error: no firmware section \n");
    throw;
  }
  const auto& js_fw_conf =  m_jsdoc_conf["firmware"];
  for(const auto &reg: js_fw_conf.GetObject()){
    m_fw->SetFirmwareRegister(reg.name.GetString(), reg.value.GetUint64());
  }

  // if(!m_jsdoc_conf.HasMember("sensor")){
  //   std::fprintf(stderr, "JSON configure file error: no sensor section \n");
  //   throw;
  // }
  // const auto& js_sn_conf =  m_jsdoc_conf["sensor"];
  // for(const auto &reg: js_sn_conf.GetObject()){
  //   m_fw->SetCisRegister(reg.name.GetString(), reg.value.GetUint64());
  // }
}

void Layer::fw_init(){
  if(!m_fw) return;  
  
  // m_fw->SetFirmwareRegister("FIRMWARE_MODE", 0); // stop trigger, go into configure mode 
  // m_fw->SetFirmwareRegister("ADDR_CHIP_ID", 0x10); //OB

  //user init
  //
  //
  // m_fw->SetFirmwareRegister("DEVICE_ID", 0xff);
  //
  //end of user init

  info_print("fw init  %s\n", m_fw->DeviceUrl().c_str());
}

void Layer::rd_start(){
  if(m_is_async_reading){
    std::fprintf(stderr, "old AsyncReading() has not been stopped\n");
    return;
  }

  m_fut_async_rd = std::async(std::launch::async, &Layer::AsyncPushBack, this);
  if(!m_is_async_watching){
    m_fut_async_watch = std::async(std::launch::async, &Layer::AsyncWatchDog, this);
  }
}

void Layer::rd_stop(){
  m_is_async_reading = false;
  if(m_fut_async_rd.valid())
    m_fut_async_rd.get();

  m_is_async_watching = false;
  if(m_fut_async_watch.valid())
    m_fut_async_watch.get();
}

uint64_t Layer::AsyncPushBack(){ // IMPROVE IT AS A RING
  m_vec_ring_ev.clear();
  m_vec_ring_ev.resize(m_size_ring);
  m_count_ring_write = 0;
  m_count_ring_read = 0;
  m_hot_p_read = m_size_ring -1; // tail

  uint32_t tg_expected = 0;
  uint32_t flag_wait_first_event = true;

  m_rd->Open();
  info_print(" rd start  %s\n", m_rd->DeviceUrl().c_str());
  m_is_async_reading = true;

  m_st_n_tg_ev_now =0;
  m_st_n_ev_input_now =0;
  m_st_n_ev_output_now =0;
  m_st_n_ev_bad_now =0;
  m_st_n_ev_overflow_now =0;
  m_st_n_tg_ev_begin = 0;
  
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

    uint16_t tg_l16 = 0xffff & df->GetCounter();

    if(flag_wait_first_event){
      flag_wait_first_event = false;
      m_extension = df->GetExtension();
      tg_expected = tg_l16;
      m_st_n_tg_ev_begin = tg_expected;
      debug_print("set first event TID #%5u",  tg_expected);
    }

    uint16_t tg_expected_l16 = (tg_expected & 0xffff);
    debug_print("TID16 #%5hu, expecting #%5hu \n",  tg_l16, tg_expected_l16);

    if(tg_l16 != (tg_expected & 0xffff)){
      uint32_t tg_guess_0 = (tg_expected & 0xffff0000) + tg_l16;
      uint32_t tg_guess_1 = (tg_expected & 0xffff0000) + 0x10000 + tg_l16;
      if(tg_guess_0 > tg_expected && tg_guess_0 - tg_expected < 0x8000){
	info_print("missing trigger ID @dev#%-10" PRIu64 ", expecting %5hu,  provided %5hu\n",
		   m_extension, tg_expected_l16,  tg_l16);
        tg_expected =tg_guess_0;
      }
      else if (tg_guess_1 > tg_expected && tg_guess_1 - tg_expected < 0x8000){
	info_print("missing trigger ID @dev#%-10" PRIu64 ", expecting %5hu,  provided %5hu\n",
		   m_extension, tg_expected_l16,  tg_l16);
	tg_expected =tg_guess_1;
      }
      else{
	info_print("broken trigger ID  @dev#%-10" PRIu64 ", expecting %5hu,  provided %5hu, skipped\n",
		   m_extension, tg_expected_l16,  tg_l16);
        tg_expected ++;
        m_st_n_ev_bad_now ++;
        // permanent data lose
        continue;
      }
    }
    //TODO: fix tlu firmware, mismatch between modes AIDA start at 1, EUDET start at 0
    df->SetTrigger(tg_expected);
    m_st_n_tg_ev_now = tg_expected;

    m_vec_ring_ev[next_p_ring_write] = df;
    m_count_ring_write ++;
    tg_expected ++;
  }
  m_rd->Close();
  info_print(" rd stop  %s\n", m_rd->DeviceUrl().c_str());
  return m_count_ring_write;
}

uint64_t Layer::AsyncWatchDog(){
  m_tp_run_begin = std::chrono::system_clock::now();
  m_tp_old = m_tp_run_begin;
  m_is_async_watching = true;

  m_st_n_tg_ev_old =0;
  m_st_n_ev_input_old = 0;
  m_st_n_ev_bad_old =0;
  m_st_n_ev_overflow_old = 0;
  
  while(m_is_async_watching){
    std::this_thread::sleep_for(std::chrono::seconds(1));
    uint64_t st_n_tg_ev_begin = m_st_n_tg_ev_begin;
    uint64_t st_n_tg_ev_now = m_st_n_tg_ev_now;
    uint64_t st_n_ev_input_now = m_st_n_ev_input_now;
    //uint64_t st_n_ev_output_now = m_st_n_ev_output_now;
    uint64_t st_n_ev_bad_now = m_st_n_ev_bad_now;
    uint64_t st_n_ev_overflow_now = m_st_n_ev_overflow_now;

    // time
    auto tp_now = std::chrono::system_clock::now();
    std::chrono::duration<double> dur_period_sec = tp_now - m_tp_old;
    std::chrono::duration<double> dur_accu_sec = tp_now - m_tp_run_begin;
    double sec_period = dur_period_sec.count();
    double sec_accu = dur_accu_sec.count();


    // period
    uint64_t st_n_tg_ev_period = st_n_tg_ev_now - m_st_n_tg_ev_old;
    uint64_t st_n_ev_input_period = st_n_ev_input_now - m_st_n_ev_input_old;
    uint64_t st_n_ev_bad_period = st_n_ev_bad_now - m_st_n_ev_bad_old;
    uint64_t st_n_ev_overflow_period = st_n_ev_overflow_now - m_st_n_ev_overflow_old;
    
    // ratio
    //double st_output_vs_input_accu = st_n_ev_input_now? st_ev_output_now / st_ev_input_now : 1;
    double st_bad_vs_input_accu = st_n_ev_input_now? 1.0 * st_n_ev_bad_now / st_n_ev_input_now : 0;
    double st_overflow_vs_input_accu = st_n_ev_input_now? 1.0 *  st_n_ev_overflow_now / st_n_ev_input_now : 0;
    double st_input_vs_trigger_accu = st_n_ev_input_now? 1.0 * st_n_ev_input_now / (st_n_tg_ev_now - st_n_tg_ev_begin + 1) : 1;
    
    //double st_output_vs_input_period = st_ev_input_period? st_ev_output_period / st_ev_input_period : 1;
    double st_bad_vs_input_period = st_n_ev_input_period? 1.0 * st_n_ev_bad_period / st_n_ev_input_period : 0;
    double st_overflow_vs_input_period = st_n_ev_input_period? 1.0 *  st_n_ev_overflow_period / st_n_ev_input_period : 0;
    double st_input_vs_trigger_period = st_n_tg_ev_period? 1.0 *  st_n_ev_input_period / st_n_tg_ev_period : 1;
    
    // hz
    double st_hz_tg_accu = (st_n_tg_ev_now - st_n_tg_ev_begin + 1) / sec_accu ;
    double st_hz_input_accu = st_n_ev_input_now / sec_accu ; 

    double st_hz_tg_period = st_n_tg_ev_period / sec_period ;
    double st_hz_input_period = st_n_ev_input_period / sec_period ;

    std::string st_string_new =
      FirmwarePortal::FormatString("L<%-4" PRIu64 "> event(%10" PRIu64 ")/trigger(%8" PRIu64 " - %" PRIu64 ")=Ev/Tr(%6.4f) dEv/dTr(%6.4f) tr_accu(%8.2f hz) ev_accu(%8.2f hz) tr_period(%8.2f hz) ev_period(%8.2f hz)",
                                   m_extension, st_n_ev_input_now, st_n_tg_ev_now, st_n_tg_ev_begin,
				   st_input_vs_trigger_accu, st_input_vs_trigger_period,
                                   st_hz_tg_accu, st_hz_input_accu, st_hz_tg_period, st_hz_input_period
                                   );
    
    {
      std::unique_lock<std::mutex> lk(m_mtx_st);
      m_st_string = std::move(st_string_new);
    }
    
    //write to old
    m_st_n_tg_ev_old = st_n_tg_ev_now;
    m_st_n_ev_input_old = st_n_ev_input_now;
    m_st_n_ev_bad_old = st_n_ev_bad_now;
    m_st_n_ev_overflow_old = st_n_ev_overflow_now;
    m_tp_old = tp_now;
  }
  return 0;
}

std::string  Layer::GetStatusString(){
  std::unique_lock<std::mutex> lk(m_mtx_st);
  return m_st_string;
}

DataFrameSP& Layer::Front(){
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

void Layer::PopFront(){
  if(m_count_ring_write > m_count_ring_read) {
    uint64_t next_p_ring_read = m_count_ring_read % m_size_ring;
    m_hot_p_read = next_p_ring_read;
    // keep hot read to prevent write-overlapping
    m_vec_ring_ev[next_p_ring_read].reset();
    m_count_ring_read ++;
  }
}

uint64_t Layer::Size(){
  return  m_count_ring_write - m_count_ring_read;
}

void Layer::ClearBuffer(){
  m_count_ring_write = m_count_ring_read;
  m_vec_ring_ev.clear();
}
