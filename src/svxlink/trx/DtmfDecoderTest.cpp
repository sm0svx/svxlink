#include <iostream>
#include <string>

#include <AsyncConfig.h>

#include "DtmfDecoder.h"
#include "DtmfEncoder.h"

using namespace std;
using namespace Async;

namespace {
//string send_digits = "00112233445566778899AABBCCDD**##";
string send_digits = "1";
string received_digits;

void digit_detected(char ch, int duration)
{
  //cout << ch << " " << duration << endl;
  received_digits += ch;
}
};


int main()
{
  Config cfg;
  cfg.setValue("Test", "DTMF_DEC_TYPE", "EXPERIMENTAL");
  //cfg.setValue("Test", "DTMF_DEC_TYPE", "INTERNAL");

  DtmfEncoder *enc = new DtmfEncoder;
  enc->setToneLength(40);
  enc->setToneSpacing(20);
  //enc->setToneAmplitude(-6);
  AudioSource *prev_src = enc;

  DtmfDecoder *dec = DtmfDecoder::create(cfg, "Test");
  dec->digitDeactivated.connect(sigc::ptr_fun(digit_detected));
  prev_src->registerSink(dec);
  prev_src = 0;

  //for (int i=0; i<1000; ++i)
  {
    enc->send(send_digits);
  }
  //enc->setToneAmplitude(-100);
  //enc->send("0");
  //cout << received_digits << endl;
  if (received_digits != send_digits)
  {
    return 1;
  }
  return 0;
}

