#include <iostream>
#include <string>
#include <fstream>
#include <cstdlib>

#include <AsyncConfig.h>
#include <AsyncAudioNoiseAdder.h>

#include "DtmfDecoder.h"
#include "DtmfEncoder.h"

using namespace std;
using namespace Async;

namespace {
string send_digits = "00112233445566778899AABBCCDD**##";
//string send_digits = "0123456789ABCD*#";
//string send_digits = "1";
string received_digits;

void digit_detected(char ch, int duration)
{
  //cout << ch << " " << duration << endl;
  received_digits += ch;
}
};


class FileWriter : public Async::AudioProcessor
{
  public:
    FileWriter(const string &path)
    {
      ofs.open(path.c_str(), ios::out | ios::binary);
      if (ofs.fail())
      {
        cout << "Could not open output file: " << path << endl;
        exit(1);
      }
    }

    ~FileWriter(void)
    {
      ofs.close();
    }

  protected:
    void processSamples(float *dest, const float *src, int count)
    {
      ofs.write(reinterpret_cast<const char*>(src), count * sizeof(*src));
      for (int i=0; i<count; ++i)
      {
        dest[i] = src[i];
      }
    }

  private:
    ofstream ofs;

};


int main()
{
  Config cfg;
  cfg.setValue("Test", "DTMF_DEC_TYPE", "EXPERIMENTAL");
  //cfg.setValue("Test", "DTMF_DEC_TYPE", "INTERNAL");
  //cfg.setValue("Test", "DTMF_MAX_FWD_TWIST", "6");
  //cfg.setValue("Test", "DTMF_MAX_REV_TWIST", "6");

  DtmfEncoder *enc = new DtmfEncoder;
  enc->setToneLength(40);
  enc->setToneSpacing(20);
  enc->setToneAmplitude(-8);
  AudioSource *prev_src = enc;

  AudioNoiseAdder *noise_adder = new AudioNoiseAdder(0.39);
  //AudioNoiseAdder *noise_adder = new AudioNoiseAdder(0.0);
  prev_src->registerSink(noise_adder);
  prev_src = noise_adder;

  FileWriter *fwriter = new FileWriter("/tmp/samples.raw");
  prev_src->registerSink(fwriter);
  prev_src = fwriter;

  DtmfDecoder *dec = DtmfDecoder::create(cfg, "Test");
  if (!dec->initialize())
  {
    cout << "*** ERROR: Could not initialize DTMF decoder\n";
    exit(1);
  }
  dec->digitDeactivated.connect(sigc::ptr_fun(digit_detected));
  prev_src->registerSink(dec);
  prev_src = 0;

  //for (int i=0; i<1000; ++i)
  {
    enc->send(send_digits);
  }
  cout << received_digits << endl;
  if (received_digits != send_digits)
  {
    return 1;
  }
  return 0;
}

