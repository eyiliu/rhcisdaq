#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include "FirmwarePortal.hh"

static const std::string reg_cmd_list_content =
#include "reg_cmd_list_json.hh"
  ;

FirmwarePortal::FirmwarePortal(const std::string &json_str){
  if(json_str == "builtin"){
    m_json.Parse(reg_cmd_list_content.c_str());
    // std::cout<< "<<<<<<"<<reg_cmd_list_content<<std::endl;
    // std::cout<< "<<<<<<"<<std::endl;
  }
  else{
    m_json.Parse(json_str.c_str());
  }
  if(m_json.HasParseError()){
    fprintf(stderr, "JSON parse error: %s (at string positon %lu)", rapidjson::GetParseError_En(m_json.GetParseError()), m_json.GetErrorOffset());
    throw;
  }
  
  
  DeviceOpen();
}

void FirmwarePortal::DeviceOpen(){
  std::cout<<"deviceOpen "<<std::endl;

  if(m_fd){
    printf("Error:  m_fd was opened. do nothing \n");
    return;
  }

  auto& js_fwctrl = m_json["FIRMWARE_CTRL"];
  if(js_fwctrl.Empty()){
    FormatPrint(std::cerr, "ERROR<%s>:   unable to find FIRMWARE_CTRL\n", __func__);
    throw;
  }
  
  
  std::string device_file_name = js_fwctrl["device_file"].GetString();

  uint64_t address_base = std::stoull(js_fwctrl["address_base"].GetString(), 0, 16);
  uint64_t length_require = std::stoull(js_fwctrl["memory_size"].GetString(), 0, 16);
  
  if ((m_fd = open(device_file_name.c_str(), O_RDWR|O_SYNC)) < 0 ) {
    printf("Error opening memory file for map. \n");
    DeviceClose();
    exit(-1);
  }
  std::cout<<"mmap open "<<std::endl;
  
  off_t phy_addr = address_base;
  size_t len = length_require;
  size_t page_size = getpagesize();  
  off_t offset_in_page = phy_addr & (page_size - 1);

  size_t mapped_size = page_size * ( (offset_in_page + len)/page_size + 1);
  
  void *map_base = mmap(NULL,
			mapped_size,
			PROT_READ | PROT_WRITE,
			MAP_SHARED,
			m_fd,
			phy_addr & ~(off_t)(page_size - 1));

  if (map_base == MAP_FAILED){
    perror("Memory mapped failed\n");
    DeviceClose();
    exit(-1);
  }

  std::cout<<"MMAP sucess "<<std::endl;

  
  m_virt_addr_base = (char*)map_base + offset_in_page;  
  m_virt_addr_end = (char*)map_base + mapped_size-1;
}


void FirmwarePortal::DeviceClose(){
  if(m_fd){
    close(m_fd);
    m_virt_addr_base=0;
    m_virt_addr_end=0;
    m_fd=0;
  }
}

void FirmwarePortal::SetFirmwareRegister(const std::string& name, uint64_t value){
  DebugFormatPrint(std::cout, "INFO<%s>: %s( name=%s ,  value=%#016llx )\n", __func__, __func__, name.c_str(), value);
  static const std::string array_name("FIRMWARE_REG_LIST");
  auto& json_array = m_json[array_name];
  if(json_array.Empty()){
    FormatPrint(std::cerr, "ERROR<%s>:   unable to find array<%s>\n", __func__, array_name.c_str());
    throw;
  }
  bool flag_found_reg = false;
  for(auto& json_reg: json_array.GetArray()){
    if( json_reg["name"] != name )
      continue;
    auto& json_addr = json_reg["address"];
    if(!json_addr.IsString()){
      FormatPrint(std::cerr, "ERROR<%s>: unknown address format<%s>\n", __func__, Stringify(json_addr).c_str());
      throw;
    }

    uint64_t address = std::stoull(json_reg["address"].GetString(), 0, 16);
    auto& json_bytes = json_reg["bytes"];
    if(!json_bytes.IsUint64()){
      FormatPrint(std::cerr, "ERROR<%s>:   bytes<%s> is not an int\n", __func__, Stringify(json_bytes).c_str());
      throw;
    }
    uint64_t nbyte = json_bytes.GetUint64();
    if(nbyte==1){
      Write<uint8_t>(address, value);
    }
    else if(nbyte==2){
      Write<uint16_t>(address, value);
    }
    else if(nbyte==4){
      Write<uint32_t>(address, value);
    }
    else{
      FormatPrint(std::cerr, "ERROR<%s>:   n bytes does not fit\n", __func__);
      throw;
    }
    flag_found_reg = true;
    break;
  }
  if(!flag_found_reg){
    FormatPrint(std::cerr, "ERROR<%s>: unable to find register<%s> in array<%s>\n", __func__, name.c_str(), array_name.c_str());
    throw;
  }
}

 
uint64_t FirmwarePortal::GetFirmwareRegister(const std::string& name){
  DebugFormatPrint(std::cout, "INFO<%s>:  %s( name=%s )\n", __func__, __func__, name.c_str());
  static const std::string array_name("FIRMWARE_REG_LIST");
  auto& json_array = m_json[array_name];
  if(json_array.Empty()){
    FormatPrint(std::cerr, "ERROR<%s>:   unable to find array<%s>\n", __func__, array_name.c_str());
    throw;
  }
  uint64_t value;
  bool flag_found_reg = false;
  for(auto& json_reg: json_array.GetArray()){
    if( json_reg["name"] != name )
      continue;
    auto& json_addr = json_reg["address"];
    if(!json_addr.IsString()){
      FormatPrint(std::cerr, "ERROR<%s>: unknown address format<%s>\n", __func__, Stringify(json_addr).c_str());
      throw;
    }

    uint64_t address = std::stoull(json_reg["address"].GetString(), 0, 16);
    auto& json_bytes = json_reg["bytes"];
    if(!json_bytes.IsUint64()){
      FormatPrint(std::cerr, "ERROR<%s>:   bytes<%s> is not an int\n", __func__, Stringify(json_bytes).c_str());
      throw;
    }
    uint64_t nbyte = json_bytes.GetUint64();
    if(nbyte==1){
      value = Read<uint8_t>(address);
    }
    else if(nbyte==2){
      value = Read<uint16_t>(address);
    }
    else if(nbyte==4){
      value = Read<uint32_t>(address);
    }
    else{
      FormatPrint(std::cerr, "ERROR<%s>:   n bytes does not fit\n", __func__);
      throw;
    }
    flag_found_reg = true;
    break;
  }
  if(!flag_found_reg){
    FormatPrint(std::cerr, "ERROR<%s>: unable to find register<%s> in array<%s>\n", __func__, name.c_str(), array_name.c_str());
    throw;
  }
  DebugFormatPrint(std::cout, "INFO<%s>: %s( name=%s ) return value=%#016x \n", __func__, __func__, name.c_str(), value);
  return value;
}

std::string FirmwarePortal::LoadFileToString(const std::string& path){

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
