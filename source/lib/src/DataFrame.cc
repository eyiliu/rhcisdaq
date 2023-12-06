#include "DataFrame.hh"
#include "mysystem.hh"
#include <iostream>


#ifndef DEBUG_PRINT
#define DEBUG_PRINT 0
#endif
#define debug_print(fmt, ...)                                           \
  do { if (DEBUG_PRINT) std::fprintf(stdout, fmt, ##__VA_ARGS__); } while (0)

#ifndef INFO_PRINT
#define INFO_PRINT 0
#endif
#define info_print(fmt, ...)                                           \
  do { if (INFO_PRINT) std::fprintf(stdout, fmt, ##__VA_ARGS__); } while (0)


DataFrame::DataFrame(std::string&& raw)
  : m_raw(std::move(raw))
{
  fromRaw(m_raw);
}

DataFrame::DataFrame(const std::string& raw)
  : m_raw(raw)
{
  fromRaw(m_raw);

}


DataFrame::DataFrame(std::vector<MeasRaw>&& meas_col)
  :m_measraw_col(std::move(meas_col)){
  
  fromMeasRaws(m_measraw_col);

  fillJsdoc();  
}


//TODO
void DataFrame::fromRaw(const std::string &raw){
    const uint8_t* p_raw_beg = reinterpret_cast<const uint8_t *>(raw.data());
    const uint8_t* p_raw = p_raw_beg;
}

void DataFrame::fromMeasRaws(const std::vector<MeasRaw>& meas_col){
  for(auto& mr: meas_col){
    auto pix0= mr.getPixel0();
    auto pix1= mr.getPixel1();
    m_map_pos_adc[{pix0.row, pix0.col}] = pix0.adc;
    m_map_pos_adc[{pix1.row, pix1.col}] = pix1.adc;

    m_pixel_col.push_back(pix0);
    m_pixel_col.push_back(pix1);
  } 
}

void DataFrame::Print(std::ostream& os, size_t ws) const
{ 
  rapidjson::OStreamWrapper osw(os);
  rapidjson::Writer<rapidjson::OStreamWrapper> writer(osw);
  m_jsdoc.Accept(writer);
}

void  DataFrame::fillJsdoc(){
  auto& allo = m_jsdoc.GetAllocator();
  m_jsdoc.SetObject();
  m_jsdoc.AddMember("sensor", "rhcis1", allo);
  rapidjson::Value js_rcv_frame;
  js_rcv_frame.SetArray();
  for(const auto& [rc,v]: m_map_pos_adc){
    rapidjson::Value js_rcv;
    js_rcv.SetArray();
    js_rcv.PushBack(rapidjson::Value(rc.first), allo);
    js_rcv.PushBack(rapidjson::Value(rc.second), allo);
    js_rcv.PushBack(rapidjson::Value(v), allo);
    js_rcv_frame.PushBack(std::move(js_rcv), allo);
  }
  m_jsdoc.AddMember("rcv_frame", std::move(js_rcv_frame) , allo);
  //https://rapidjson.org/classrapidjson_1_1_generic_object.html
  //https://rapidjson.org/md_doc_tutorial.html#CreateModifyValues
}
