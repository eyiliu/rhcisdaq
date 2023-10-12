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

DataFrame::DataFrame(const rapidjson::Value &js){
  fromJSON<>(js);
}

DataFrame::DataFrame(const rapidjson::GenericValue<
                     rapidjson::UTF8<char>,
                     rapidjson::CrtAllocator> &js){
  fromJSON<rapidjson::CrtAllocator>(js);
}

DataFrame::DataFrame(std::vector<MeasRaw>&& meas_col)
  :m_measraw_col(std::move(meas_col)){
  
  fromMeasRaws(m_measraw_col);
  
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
  rapidjson::CrtAllocator allo;
  JSON(allo).Accept(writer);
}
