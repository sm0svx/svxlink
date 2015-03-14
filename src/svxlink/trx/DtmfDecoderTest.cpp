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


class FileReader : public AudioSource
{
  public:
    FileReader(void)
    {
    }

    virtual ~FileReader(void)
    {
      ifs.close();
    }

    bool open(const string &path)
    {
      ifs.open(path.c_str(), ios::in | ios::binary);
      if (ifs.fail())
      {
        cout << "Could not open input file: " << path << endl;
        return false;
      }
      writeAudio();
      return true;
    }

    virtual void resumeOutput(void)
    {
      writeAudio();
    }
    

  protected:
    virtual void allSamplesFlushed(void)
    {
    }

  private:
    ifstream ifs;

    void writeAudio()
    {
      if (!ifs.good())
      {
        return;
      }

      int samp_written = 0;
      do
      {
        int16_t buf[256];
        ifs.read(reinterpret_cast<char*>(buf), sizeof(buf));
        int samp_cnt = ifs.gcount() / sizeof(*buf);
        float samples[samp_cnt];
        for (int i=0; i<samp_cnt; ++i)
        {
          samples[i] = static_cast<float>(buf[i]) / 32767.0f;
        }
        samp_written = sinkWriteSamples(samples, samp_cnt);
        if (samp_written < 0)
        {
          cout << "*** ERROR: Could not write samples\n";
          return;
        }
        int left =  samp_cnt - samp_written;
        if (left > 0)
        {
          ifs.seekg(left, istream::cur);
        }
      } while ((samp_written > 0) && ifs.good());
    }
};


int main()
{
  Config cfg;
  cfg.setValue("Test", "DTMF_DEC_TYPE", "EXPERIMENTAL");
  //cfg.setValue("Test", "DTMF_DEC_TYPE", "INTERNAL");
  //cfg.setValue("Test", "DTMF_MAX_FWD_TWIST", "6");
  //cfg.setValue("Test", "DTMF_MAX_REV_TWIST", "6");

  AudioSource *prev_src = 0;
  DtmfEncoder *enc = 0;
#define SIMULATE
#ifdef SIMULATE
  enc = new DtmfEncoder;
  enc->setToneLength(40);
  enc->setToneSpacing(20);
  enc->setToneAmplitude(-8);
  prev_src = enc;

  AudioNoiseAdder *noise_adder = new AudioNoiseAdder(0.39);
  //AudioNoiseAdder *noise_adder = new AudioNoiseAdder(0.0);
  prev_src->registerSink(noise_adder);
  prev_src = noise_adder;
#else
  FileReader *rdr = new FileReader;
  prev_src = rdr;
#endif

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

#ifndef SIMULATE
  //for (int i=0; i<1000; ++i)
  {
    enc->send(send_digits);
  }
  if (!rdr->open("/tmp/output.raw"))
  {
    exit(1);
  }
#endif
  cout << received_digits << endl;
  if (!received_digits.empty())
  {
    return 1;
  }
#ifdef SIMULATE
  if (received_digits != send_digits)
  {
    return 1;
  }
#endif
  return 0;
}

