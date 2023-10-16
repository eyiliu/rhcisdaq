#ifndef _CAMERA_HTTWS_HANDLER_HH_
#define _CAMERA_HTTWS_HANDLER_HH_

#include <string>

#include "mysystem.hh"

#include <TH1.h>
#include "THttpWSHandler.h"


// #include "THttpServer.h"
// #include "THttpCallArg.h"
#include "TString.h"
//#include "TSystem.h"
#include "TTimer.h"



class CameraHttpWSHandler : public THttpWSHandler {
public:
  TRandom3 random;
  UInt_t fWSId{0};
  Int_t fServCnt{0};
  std::string clientMsg = "";
  TH1F *hpx = nullptr;
  
  CameraHttpWSHandler(const char *name = nullptr, const char *title = nullptr, TH1F *hpx1=nullptr);

  // load custom HTML page when open correspondent address
  TString GetDefaultPageContent() override { return "file:htmpcb.htm"; }

  Bool_t ProcessWS(THttpCallArg *arg) override;

  /// per timeout sends data portion to the client
  Bool_t HandleTimer(TTimer *) override;

};

#endif
