
#include <iostream>


#include <TRandom3.h>

#include "CameraHttpWSHandler.hh"
#include "myrapidjson.h"

#include "TDatime.h"


CameraHttpWSHandler::CameraHttpWSHandler(const char *name, const char *title, TH1F *hpx1)
  : THttpWSHandler(name, title),hpx(hpx1){
  
}


Bool_t CameraHttpWSHandler::ProcessWS(THttpCallArg *arg) {
  if (!arg || (arg->GetWSId()==0)) return kTRUE;

  // printf("Method %s\n", arg->GetMethod());

  if (arg->IsMethod("WS_CONNECT")) {
    // accept only if connection not established
    return fWSId == 0;
  }

  if (arg->IsMethod("WS_READY")) {
    fWSId = arg->GetWSId();
    printf("Client connected %d\n", fWSId);
    return kTRUE;
  }

  if (arg->IsMethod("WS_CLOSE")) {
    fWSId = 0;
    printf("Client disconnected\n");
    return kTRUE;
  }

  if (arg->IsMethod("WS_DATA")) {
    TString str;
    str.Append((const char *)arg->GetPostData(), arg->GetPostDataLength());
    clientMsg = str.Data();
    printf("Client msg: %s\n", str.Data());
    /*json::value root;  
      std::string errors;  
      if (json::parse(message, root, errors)) {
                
      }*/
    std::string str1 = "";
    try {

      rapidjson::Document json_obj;
      json_obj.Parse(clientMsg);
      if(json_obj.HasParseError()){
	fprintf(stderr, "JSON parse error: %s (at string positon %lu)", rapidjson::GetParseError_En(json_obj.GetParseError()), json_obj.GetErrorOffset());
	fprintf(stderr, "skip problomatic message\n");
	throw;
      }
      
      std::string msg_json = json_obj["msg"].GetString();
      std::string arg1 = json_obj["arg1"].GetString();
      int number = std::stoi(arg1);
 
      if(msg_json == "randomTH1F"){
	hpx->Reset();
	Float_t px, py;
	for(int i=0;i<10000+random.Uniform(50, 500);i++){
	  random.Rannor(px,py);
	  hpx->Fill(px);
	}
      }else if(msg_json == "setTH1F"){
	hpx->Reset();
	Float_t px, py;
	for(int i=0;i<number;i++){
	  random.Rannor(px,py);
	  hpx->Fill(px);
	}
      }   
                 
                 
    } catch (std::exception& e) {  
      std::cout << "Error: " << e.what() << std::endl;  
      std::cout << "String is not a valid JSON string." << std::endl;  
    } 

    TDatime now;//now.AsString()
    SendCharStarWS(arg->GetWSId(), Form("get\" %s \" from client, %s server counter:%d", clientMsg.c_str(), str1.c_str(), fServCnt++));
    return kTRUE;
  }

  return kFALSE;
}


/// per timeout sends data portion to the client
Bool_t CameraHttpWSHandler::HandleTimer(TTimer *){
  //TDatime now;
  // if (fWSId) SendCharStarWS(fWSId, Form("Server sends data:%s server counter:%d", now.AsString(), fServCnt++));
  return kTRUE;
}
