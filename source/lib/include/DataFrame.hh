#ifndef _DATAFRAME_HH_
#define _DATAFRAME_HH_

#include <string>
#include <vector>
#include <memory>
#include <map>


#include "mysystem.hh"
#include "myrapidjson.h"


#define FRONT_MEASRAW_32 (0x5500383c)
#define END_MEASRAW_32 (0x553f0307)


struct MeasRaw;
struct MeasPixel;


struct MeasPixel{
  uint8_t row;
  uint8_t col;
  uint16_t adc;

  inline bool operator<(const MeasPixel &rh) const{
    return static_cast<uint16_t>(row)<<8+col < static_cast<uint16_t>(rh.row)<<8+col;
  }
};


//adc0_h adc0_l adc1_h adc1_l head row col0 col1  (right lowend)

struct MeasRaw{
  union {
    uint64_t raw64;
    uint32_t raw32[2];
    uint16_t raw16[4];
    unsigned char raw8[8];
  } data{0};
  MeasRaw()
    :data{ .raw64 = 0 }{};
  
  MeasRaw(uint64_t h64)
    :data{ .raw64 = h64 }{};
  // MeasRaw(unsigned char head, unsigned char brow, unsigned char col1, unsigned char col2, uint16_t adc1, uint16_t adc2)
  //   :data{ .raw16[0]=adc2, .raw16[1]=adc1, .raw8[4]=col2, .raw8[5]=col1, .raw8[6]=brow, .raw8[7]=head}{};
  
  inline bool operator==(const MeasRaw &rh) const{
    return data.raw64 == rh.data.raw64;
  }

  inline bool operator==(const uint64_t &rh) const{
    return data.raw64 == rh;
  }
  
  inline bool operator<(const MeasRaw &rh) const{
    return data.raw64 < rh.data.raw64;
  }

  inline const uint64_t& raw64() const  {return data.raw64;}
  inline const unsigned char& head() const  {return data.raw8[3];}
  inline const unsigned char& brow() const  {return data.raw8[2];}
  inline const unsigned char& col0() const  {return data.raw8[1];}
  inline const unsigned char& col1() const  {return data.raw8[0];}
  inline const uint16_t& adc0() const  {return data.raw16[3];} // raw8[7]<<8+raw8[6]
  inline const uint16_t& adc1() const  {return data.raw16[2];} // raw8[5]<<8+raw8[4]
  inline static void dropbyte(MeasRaw &meas){
    meas.data.raw64>>8;
  }

  inline MeasPixel getPixel0() const {return {brow(),col0(),adc0()};}
  inline MeasPixel getPixel1() const {return {brow(),col1(),adc1()};}

  inline bool isFrontMeasRaw(){
    return (data.raw32[0]==FRONT_MEASRAW_32);
  }

  inline bool isEndMeasRaw(){
    return (data.raw32[0]==END_MEASRAW_32);
  }  
};


class DataFrame;
using DataFrameSP = std::shared_ptr<DataFrame>;

class DataFrame {
public:
  DataFrame(const std::string& raw);
  DataFrame(std::string&& raw);
  DataFrame(std::vector<MeasRaw>&& meas_col);
  
  DataFrame(){};
  
  void Print(std::ostream& os, size_t ws = 0) const;

  void fillJsdoc();
  const rapidjson::Document& jsdoc() const;
  
  void fromRaw(const std::string &raw);
  void fromMeasRaws(const std::vector<MeasRaw> &meas_col);

  std::vector<MeasRaw> m_measraw_col;
  std::vector<MeasPixel> m_pixel_col;

  std::map<std::pair<uint8_t, uint8_t>, int16_t> m_map_pos_adc;
  std::string m_raw;
  
  rapidjson::Document m_jsdoc;
};


#endif
