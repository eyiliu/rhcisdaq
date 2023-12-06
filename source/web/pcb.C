/// \file
/// \ingroup tutorial_http
///  This program demonstrate WebSocket usage with THttpServer
///  Custom htmpcb.htm page is loaded and regularly sends messages to server
///
/// \macro_code
///
/// \author Sergey Linev
#include <TH1.h>
#include <TRandom3.h>
#include "THttpServer.h"
#include "THttpWSHandler.h"
#include "THttpCallArg.h"
#include "TString.h"
#include <string>
#include "TSystem.h"
#include "TDatime.h"
#include "TTimer.h"
#include <nlohmann/json.hpp> 
#include <cstdio>

using json = nlohmann::json;

Bool_t bFillHist = kTRUE;
class TUserHandler : public THttpWSHandler {
   public:
      TRandom3 random;
      UInt_t fWSId{0};
      Int_t fServCnt{0};
      std::string clientMsg = "";
      TH1F *hpx = nullptr;
      TUserHandler(const char *name = nullptr, const char *title = nullptr, TH1F *hpx1=nullptr) : THttpWSHandler(name, title),hpx(hpx1){}

      // load custom HTML page when open correspondent address
      TString GetDefaultPageContent() override { return "file:htmpcb.htm"; }

      Bool_t ProcessWS(THttpCallArg *arg) override
      {
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
           string str1 = "";
            try {  
                 nlohmann::json json_obj = nlohmann::json::parse(clientMsg);  
                 std::string msg_json = json_obj["msg"];
                 //std::cout << "msg: " << json_obj["msg"] << std::endl;  
                 //std::cout << "arg1: " << json_obj["arg1"] << std::endl;
                 std::string arg1 = json_obj["arg1"];
                 int number = std::stoi(arg1);
                            //json json_obj = json::parse(clientMsg);
 
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
      Bool_t HandleTimer(TTimer *) override
      {
        //TDatime now;
        // if (fWSId) SendCharStarWS(fWSId, Form("Server sends data:%s server counter:%d", now.AsString(), fServCnt++));
         return kTRUE;
      }

};

void pcb()
{
   TH1F *hpx = new TH1F("hpx","This is the px distribution",100,-4,4);
   TRandom3 random;
   random.SetSeed(time(NULL));
   Float_t px, py;
   for(int i=0;i<1000;i++){
      random.Rannor(px,py);
      hpx->Fill(px);
   }
   
   THttpServer *serv = new THttpServer("http:8090");

   TUserHandler *handler = new TUserHandler("name1", "title1", hpx);

   serv->Register("/folder1", handler);
   serv->Register("/", hpx);
   serv->RegisterCommand("/Start", "bFillHist=kTRUE;", "button;rootsys/icons/ed_execute.png");
   serv->RegisterCommand("/Stop",  "bFillHist=kFALSE;", "button;rootsys/icons/ed_interrupt.png");
   
   serv->RegisterCommand("/ResetHPX","/hpx/->Reset()");//button;rootsys/icons/ed_delete.png
      // use custom web page as default
   serv->SetDefaultPage("htmpcb.htm");
   
   const char *addr = "http://localhost:8090/folder1/name1/";

   //printf("Starting browser with URL address %s\n", addr);
   printf("In browser content of htmpcb.htm file should be loaded\n");
   printf("Please be sure that htmpcb.htm is provided in current directory\n");
/*
   if (gSystem->InheritsFrom("TMacOSXSystem"))
      gSystem->Exec(Form("open %s", addr));
   else if (gSystem->InheritsFrom("TWinNTSystem"))
      gSystem->Exec(Form("start %s", addr));
   else
      gSystem->Exec(Form("xdg-open %s &", addr));
*/
   //when connection will be established, data will be send to the client
   //TTimer *tm = new TTimer(handler, 3700);
   //tm->Start();

}
