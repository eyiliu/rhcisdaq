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
  }
  else{
    m_json.Parse(json_str.c_str());
  }
  if(m_json.HasParseError()){
    fprintf(stderr, "JSON parse error: %s (at string positon %lu)", rapidjson::GetParseError_En(m_json.GetParseError()), m_json.GetErrorOffset());
    throw;
  }
}


void  FirmwarePortal::WriteByte(uint64_t address, uint64_t value){
  DebugFormatPrint(std::cout, "INFO<%s>: %se( address=%#016llx ,  value=%#016llx )\n", __func__, __func__, address, value);
    
  int fd;
  if ((fd = open("/dev/mem", O_RDWR|O_SYNC)) < 0 ) {
    printf("Error opening file. \n");
    exit(-1);
  }
  
  off_t phy_addr = address;
  size_t len = 1;
  size_t page_size = getpagesize();  
  off_t offset_in_page = phy_addr & (page_size - 1);

  size_t mapped_size = page_size * ( (offset_in_page + len)/page_size + 1);
  
  void *map_base = mmap(NULL,
			mapped_size,
			PROT_READ | PROT_WRITE,
			MAP_SHARED,
			fd,
			phy_addr & ~(off_t)(page_size - 1));

  if (map_base == MAP_FAILED){
    perror("Memory mapped failed\n");
    exit(-1);
  }

  char* virt_addr = (char*)map_base + offset_in_page;
  *virt_addr = (char)value;  

  close(fd);

};

uint64_t FirmwarePortal::ReadByte(uint64_t address){
  DebugFormatPrint(std::cout, "ReadByte( address=%#016x)\n", address);

  int fd;
  if ((fd = open("/dev/mem", O_RDWR|O_SYNC)) < 0 ) {
    printf("Error opening file. \n");
    exit(-1);
  }
  
  off_t phy_addr = address;
  size_t len = 1;
  size_t page_size = getpagesize();  
  off_t offset_in_page = phy_addr & (page_size - 1);

  size_t mapped_size = page_size * ( (offset_in_page + len)/page_size + 1);
  
  void *map_base = mmap(NULL,
			mapped_size,
			PROT_READ | PROT_WRITE,
			MAP_SHARED,
			fd,
			phy_addr & ~(off_t)(page_size - 1));

  if (map_base == MAP_FAILED){
    perror("Memory mapped failed\n");
    exit(-1);
  }

  char* virt_addr = (char*)map_base + offset_in_page;
  uint8_t reg_value = *virt_addr;  

  close(fd);
  
  DebugFormatPrint(std::cout, "ReadByte( address=%#016x) return value=%#016x\n", address, reg_value);
  return reg_value;
};



void FirmwarePortal::SetFirmwareRegister(const std::string& name, uint64_t value){
  DebugFormatPrint(std::cout, "INFO<%s>: %s( name=%s ,  value=%#016llx )\n", __func__, __func__, name.c_str(), value);
  static const std::string array_name("FIRMWARE_REG_LIST_V3");
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
    if(json_addr.IsString()){
      uint64_t address = std::stoull(json_reg["address"].GetString(), 0, 16);
      WriteByte(address, value); //word = byte
    }
    else if(json_addr.IsArray()){
      auto& json_bytes = json_reg["bytes"];
      auto& json_words = json_reg["words"];
      if(!json_bytes.IsUint64()){
	FormatPrint(std::cerr, "ERROR<%s>:   bytes<%s> is not an int\n", __func__, Stringify(json_bytes).c_str());
	throw;
      }
      if(!json_words.IsUint64()){
	FormatPrint(std::cerr, "ERROR<%s>:   words<%s> is not an int\n", __func__, Stringify(json_words).c_str());
	throw;
      }
      uint64_t n_bytes = json_bytes.GetUint64();
      uint64_t n_words = json_words.GetUint64();
      if((!n_bytes)||(!n_words)||(n_bytes%n_words)){
	FormatPrint(std::cerr, "ERROR<%s>: incorrect bytes<%u> or words<%u>\n" , __func__, json_bytes.GetUint64(), json_words.GetUint64());
	throw;
      }
      uint64_t n_bits_per_word = 8*n_bytes/n_words;
      
      auto& json_endian = json_reg["endian"];
      bool flag_is_little_endian;
      if(json_endian=="LE"){
	flag_is_little_endian = true;
      }
      else if(json_endian=="BE"){
	flag_is_little_endian = false;
      }
      else{
	FormatPrint(std::cerr, "ERROR<%s>: unknown endian<%s>\n", __func__, Stringify(json_endian).c_str());
	throw;
      }

      if(n_words != json_addr.Size()){
	FormatPrint(std::cerr, "ERROR<%s>: address<%s> array's size does not match the word number which is %u\n", __func__ , Stringify(json_addr).c_str(), n_words);
	throw;
      }
      uint64_t i=0;
      for(auto& name_in_array: json_addr.GetArray()){
	if(!name_in_array.IsString()){
	  FormatPrint(std::cerr, "ERROR<%s>: name<%s> is not a string value\n", __func__, Stringify(name_in_array).c_str());
	  throw;
	}
	std::string name_in_array_str = name_in_array.GetString();
	uint64_t sub_value;
	if(flag_is_little_endian){
	  size_t f = 8*sizeof(value)-n_bits_per_word*(i+1);
	  size_t b = 8*sizeof(value)-n_bits_per_word;
	  sub_value = (value<<f)>>b;
	  // DebugFormatPrint(std::cout, "INFO<%s>: %s value=%#016x (<<%z)  (>>%z) sub_value=%#016llx \n", __func__, "LE", value, f, b, sub_value);
	}
	else{
	  size_t f = 8*sizeof(value)-n_bits_per_word*(n_words-i);
	  size_t b = 8*sizeof(value)-n_bits_per_word;
	  sub_value = (value<<f)>>b;
	  // DebugFormatPrint(std::cout, "INFO<%s>: %s value=%#016x (<<%z)  (>>%z) sub_value=%#016llx \n", __func__, "BE", value, f, b, sub_value);
	}
	SetFirmwareRegister(name_in_array_str, sub_value);
	i++;
      }
    }
    else{
      FormatPrint(std::cerr, "ERROR<%s>: unknown address format<%s>\n", __func__, Stringify(json_addr).c_str());
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

void FirmwarePortal::SetCisRegister(const std::string& name, uint64_t value){
  DebugFormatPrint(std::cout, "INFO<%s>: %s( name=%s ,  value=%#016llx )\n", __func__, __func__, name.c_str(), value);
  static const std::string array_name("CHIP_REG_LIST");
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
    if(json_addr.IsString()){
      uint64_t address = std::stoull(json_reg["address"].GetString(), 0, 16);
      SetFirmwareRegister("ADDR_CHIP_REG", address);
      SetFirmwareRegister("DATA_WRITE", value);
      SendFirmwareCommand("WRITE");
    }
    else if(json_addr.IsArray()){
      auto& json_bytes = json_reg["bytes"];
      auto& json_words = json_reg["words"];
      if(!json_bytes.IsUint64()){
	FormatPrint(std::cerr, "ERROR<%s>:   bytes<%s> is not an int\n", __func__, Stringify(json_bytes).c_str());
	throw;
      }
      if(!json_words.IsUint64()){
	FormatPrint(std::cerr, "ERROR<%s>:   words<%s> is not an int\n", __func__, Stringify(json_words).c_str());
	throw;
      }
      uint64_t n_bytes = json_bytes.GetUint64();
      uint64_t n_words = json_words.GetUint64();
      if((!n_bytes)||(!n_words)||(n_bytes%n_words)){
	FormatPrint(std::cerr, "ERROR<%s>: incorrect bytes<%u> or words<%u>\n" , __func__, json_bytes.GetUint64(), json_words.GetUint64());
	throw;
      }
      uint64_t n_bits_per_word = 8*n_bytes/n_words;

      auto& json_endian = json_reg["endian"];
      bool flag_is_little_endian;
      if(json_endian=="LE"){
	flag_is_little_endian = true;
      }
      else if(json_endian=="BE"){
	flag_is_little_endian = false;
      }
      else{
	FormatPrint(std::cerr, "ERROR<%s>: unknown endian<%s>\n", __func__, Stringify(json_endian).c_str());
	throw;
      }
      
      if(n_words != json_addr.Size()){
	FormatPrint(std::cerr, "ERROR<%s>: address<%s> array's size does not match the word number which is %u\n", __func__ ,Stringify(json_addr).c_str(), n_words);
	throw;
      }
      uint64_t i=0;
      for(auto& name_in_array: json_addr.GetArray() ){
	if(!name_in_array.IsString()){
	  FormatPrint(std::cerr, "ERROR<%s>: name<%s> is not a string value\n", __func__, Stringify(name_in_array).c_str());
	  throw;
	}
	std::string name_in_array_str = name_in_array.GetString();
	uint64_t sub_value;
	if(flag_is_little_endian){
	  uint64_t f = (8*sizeof(value)-n_bits_per_word*(i+1));
	  uint64_t b = (8*sizeof(value)-n_bits_per_word);
	  sub_value = (value<<f)>>b;
	  DebugFormatPrint(std::cout, "INFO<%s>:  %s value=%#016x << %u  >>%u sub_value=%#016x \n", __func__, "LE", value, f, b, sub_value);
	}
	else{
	  uint64_t f = (8*sizeof(value)-n_bits_per_word*(n_words-i));
	  uint64_t b = (8*sizeof(value)-n_bits_per_word);
	  sub_value = (value<<f)>>b;
	  DebugFormatPrint(std::cout, "INFO<%s>:  %s value=%#016x << %u  >>%u sub_value=%#016x \n", __func__, "BE", value, f, b, sub_value);
	}
	SetCisRegister(name_in_array_str, sub_value);
	i++;  
      }
    }
    else{
      FormatPrint(std::cerr, "ERROR<%s>: unknown address format<%s>\n", __func__, Stringify(json_addr).c_str());
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

void FirmwarePortal::SendFirmwareCommand(const std::string& name){
  DebugFormatPrint(std::cout, "INFO<%s>:  %s( name=%s )\n", __func__, __func__, name.c_str());
  static const std::string array_name("FIRMWARE_CMD_LIST_V3");
  auto& json_array = m_json[array_name];
  if(json_array.Empty()){
    FormatPrint(std::cerr, "ERROR<%s>:   unable to find array<%s>\n", __func__, array_name.c_str());
    throw;
  }
  bool flag_found_cmd = false;
  for(auto& json_cmd: json_array.GetArray()){
    if( json_cmd["name"] != name )
      continue;
    auto& json_value = json_cmd["value"];
    if(!json_value.IsString()){
      FormatPrint(std::cerr, "ERROR<%s>:   command value<%s> is not a string\n", __func__, Stringify(json_value).c_str());
      throw;
    }
    uint64_t cmd_value = std::stoull(json_value.GetString(),0,16);
    SetFirmwareRegister("FIRMWARE_CMD", cmd_value);
    flag_found_cmd = true;
  }
  if(!flag_found_cmd){
    FormatPrint(std::cerr, "ERROR<%s>: unable to find command<%s> in array<%s>\n", __func__, name.c_str(), array_name.c_str());
    throw;
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
}

void FirmwarePortal::SendCisCommand(const std::string& name){
  DebugFormatPrint(std::cout, "INFO<%s>:  %s( name=%s )\n", __func__, __func__, name.c_str());
  static const std::string array_name("CHIP_CMD_LIST");
  auto& json_array = m_json[array_name];
  if(json_array.Empty()){
    FormatPrint(std::cerr, "ERROR<%s>:   unable to find array<%s>\n", __func__, array_name.c_str());
    throw;
  }
  bool flag_found_cmd = false;
  for(auto& json_cmd: json_array.GetArray()){
    if( json_cmd["name"] != name )
      continue;
    auto& json_value = json_cmd["value"];
    if(!json_value.IsString()){
      FormatPrint(std::cerr, "ERROR<%s>:   command value<%s> is not a string\n", __func__, Stringify(json_value).c_str());
      throw;
    }
    uint64_t cmd_value = std::stoull(json_value.GetString(),0,16);
    SetCisRegister("CHIP_CMD", cmd_value);
    flag_found_cmd = true;
  }
  if(!flag_found_cmd){
    FormatPrint(std::cerr, "ERROR<%s>: unable to find command<%s> in array<%s>\n", __func__, name.c_str(), array_name.c_str());
    throw;
  } 
}


uint64_t FirmwarePortal::GetFirmwareRegister(const std::string& name){
  DebugFormatPrint(std::cout, "INFO<%s>:  %s( name=%s )\n", __func__, __func__, name.c_str());
  static const std::string array_name("FIRMWARE_REG_LIST_V3");
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
    if(json_addr.IsString()){
      uint64_t address = std::stoull(json_reg["address"].GetString(), 0, 16);
      value = ReadByte(address);
    }
    else if(json_addr.IsArray()){
      auto& json_bytes = json_reg["bytes"];
      auto& json_words = json_reg["words"];
      if(!json_bytes.IsUint64()){
	FormatPrint(std::cerr, "ERROR<%s>:   bytes<%s> is not an int\n", __func__, Stringify(json_bytes).c_str());
	throw;
      }
      if(!json_words.IsUint64()){
	FormatPrint(std::cerr, "ERROR<%s>:   words<%s> is not an int\n", __func__, Stringify(json_words).c_str());
	throw;
      }
      uint64_t n_bytes = json_bytes.GetUint64();
      uint64_t n_words = json_words.GetUint64();
      if((!n_bytes)||(!n_words)||(n_bytes%n_words)){
	FormatPrint(std::cerr, "ERROR<%s>: incorrect bytes<%u> or words<%u>\n" , __func__, json_bytes.GetUint64(), json_words.GetUint64());
	throw;
      }
      uint64_t n_bits_per_word = 8*n_bytes/n_words;

      auto& json_endian = json_reg["endian"];
      bool flag_is_little_endian;
      if(json_endian=="LE"){
	flag_is_little_endian = true;
      }
      else if(json_endian=="BE"){
	flag_is_little_endian = false;
      }
      else{
	FormatPrint(std::cerr, "ERROR<%s>: unknown endian<%s>\n", __func__, Stringify(json_endian).c_str());
	throw;
      }

      if(n_words != json_addr.Size()){
	FormatPrint(std::cerr, "ERROR<%s>: address<%s> array's size does not match the word number which is %u\n", __func__ , Stringify(json_addr).c_str(), n_words);
	throw;
      }
      uint64_t i=0;
      value = 0;
      for(auto& name_in_array: json_addr.GetArray() ){
	if(!name_in_array.IsString()){
	  FormatPrint(std::cerr, "ERROR<%s>: name<%s> is not a string value\n", __func__, Stringify(name_in_array).c_str());
	  throw;
	}
	std::string name_in_array_str = name_in_array.GetString();
	uint64_t sub_value = GetFirmwareRegister(name_in_array_str);
	uint64_t add_value;
	if(flag_is_little_endian){
	  uint64_t f = n_bits_per_word*i;
	  uint64_t b = 0;
	  add_value = (sub_value<<f)>>b;
	  DebugFormatPrint(std::cout, "INFO<%s>:  %s sub_value=%#016x << %u  >>%u add_value=%#016x \n", __func__, "LE", sub_value, f, b, add_value);
	}
	else{
	  uint64_t f = n_bits_per_word*(n_words-1-i);
	  uint64_t b = 0;
	  add_value = (sub_value<<f)>>b;
	  DebugFormatPrint(std::cout, "INFO<%s>:  %s sub_value=%#016x << %u  >>%u add_value=%#016x \n", __func__, "BE", sub_value, f, b, add_value);
	}
	value += add_value;
	i++;
      }
    }
    else{
      FormatPrint(std::cerr, "ERROR<%s>: unknown address format<%s>\n", __func__, Stringify(json_addr).c_str());
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


uint64_t FirmwarePortal::GetCisRegister(const std::string& name){  
  DebugFormatPrint(std::cout, "INFO<%s>:  %s( name=%s )\n",__func__, __func__, name.c_str());
  static const std::string array_name("CHIP_REG_LIST");
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
    if(json_addr.IsString()){
      uint64_t address = std::stoull(json_reg["address"].GetString(), 0, 16);
      SetFirmwareRegister("ADDR_CHIP_REG", address);
      uint64_t nr_old = GetFirmwareRegister("COUNT_READ");
      SendFirmwareCommand("READ");
      std::chrono::system_clock::time_point  tp_timeout = std::chrono::system_clock::now() +  std::chrono::milliseconds(1000);
      bool flag_enable_counter_check = true; //TODO: enable it for a real hardware;
      if(!flag_enable_counter_check){
	std::this_thread::sleep_for(std::chrono::milliseconds(100));
	FormatPrint(std::cout, "WARN<%s>: checking of the read count is disabled\n", __func__);
      }
      while(flag_enable_counter_check){
	uint64_t nr_new = GetFirmwareRegister("COUNT_READ");
	if(nr_new != nr_old){
	  break;
	}
	std::this_thread::sleep_for(std::chrono::milliseconds(10));
	if(std::chrono::system_clock::now() > tp_timeout){
	  FormatPrint(std::cerr, "ERROR<%s>:  timeout to read back Alpide register<%s>\n", __func__, name.c_str());
	  throw;
	}
      }
      value = GetFirmwareRegister("DATA_READ");
    }
    else if(json_addr.IsArray()){
      auto& json_bytes = json_reg["bytes"];
      auto& json_words = json_reg["words"];
      if(!json_bytes.IsUint64()){
	FormatPrint(std::cerr, "ERROR<%s>:   bytes<%s> is not an int\n", __func__, Stringify(json_bytes).c_str());
	throw;
      }
      if(!json_words.IsUint64()){
	FormatPrint(std::cerr, "ERROR<%s>:   words<%s> is not an int\n", __func__, Stringify(json_words).c_str());
	throw;
      }
      uint64_t n_bytes = json_bytes.GetUint64();
      uint64_t n_words = json_words.GetUint64();
      if((!n_bytes)||(!n_words)||(n_bytes%n_words)){
	FormatPrint(std::cerr, "ERROR<%s>: incorrect bytes<%u> or words<%u>\n" , __func__, json_bytes.GetUint64(), json_words.GetUint64());
	throw;
      }
      uint64_t n_bits_per_word = 8*n_bytes/n_words;

      auto& json_endian = json_reg["endian"];
      bool flag_is_little_endian;
      if(json_endian=="LE"){
	flag_is_little_endian = true;
      }
      else if(json_endian=="BE"){
	flag_is_little_endian = false;
      }
      else{
	FormatPrint(std::cerr, "ERROR<%s>: unknown endian<%s>\n", __func__, Stringify(json_endian).c_str());
	throw;
      }

      if(n_words != json_addr.Size()){
	FormatPrint(std::cerr, "ERROR<%s>: address<%s> array's size does not match the word number which is %u\n", __func__ , Stringify(json_addr).c_str(), n_words);
	throw;
      }
      uint64_t i=0;
      value = 0;
      for(auto& name_in_array: json_addr.GetArray() ){
	if(!name_in_array.IsString()){
	  FormatPrint(std::cerr, "ERROR<%s>: name<%s> is not a string value\n", __func__, Stringify(name_in_array).c_str());
	  throw;
	}
	std::string name_in_array_str = name_in_array.GetString();
	uint64_t sub_value = GetCisRegister(name_in_array_str);
	uint64_t add_value;
	if(flag_is_little_endian){
	  uint64_t f = n_bits_per_word*i;
	  uint64_t b = 0;
	  add_value = (sub_value<<f)>>b;
	  DebugFormatPrint(std::cout, "INFO<%s>:  %s sub_value=%#016x << %u  >>%u add_value=%#016x \n", __func__, "LE", sub_value, f, b, add_value);
	}
	else{
	  uint64_t f = n_bits_per_word*(n_words-1-i);
	  uint64_t b = 0;
	  add_value = (sub_value<<f)>>b;
	  DebugFormatPrint(std::cout, "INFO<%s>:  %s sub_value=%#016x << %u  >>%u add_value=%#016x \n", __func__, "BE", sub_value, f, b, add_value);
	}
	value += add_value;
	i++;
      }
    }
    else{
      FormatPrint(std::cerr, "ERROR<%s>: unknown address format<%s>\n", __func__, Stringify(json_addr).c_str());
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
