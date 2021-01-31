#include <iostream>
#include <string>
#include <fstream>
#include <cstdlib>
#include <cmath>

#include <AsyncConfig.h>
#include <AsyncAudioNoiseAdder.h>
#include <AsyncAudioFilter.h>
#include <AsyncAudioPassthrough.h>
#include <CppStdCompat.h>

#include "DtmfDecoder.h"
#include "DtmfEncoder.h"

using namespace std;
using namespace Async;


class FileWriter : public Async::AudioProcessor
{
  public:
    FileWriter(const string &path)
      : samppos(0)
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

    int sampPos(void) { return samppos; }

  protected:
    void processSamples(float *dest, const float *src, int count)
    {
      ofs.write(reinterpret_cast<const char*>(src), count * sizeof(*src));
      for (int i=0; i<count; ++i)
      {
        dest[i] = src[i];
      }
      samppos += count;
    }

  private:
    ofstream ofs;
    int samppos;
};


namespace {
string send_digits = "00112233445566778899AABBCCDD**##";
//string send_digits = "0123456789ABCD*#";
//string send_digits = "1";
string received_digits;
FileWriter *fwriter = 0;

void digit_detected(char ch, int duration)
{
  cout << " pos=" << fwriter->sampPos()
       << " (" << (static_cast<float>(fwriter->sampPos()) / (60*16000)) << "m)";
  cout << endl;
  /*
  cout << " digit=" << ch
       << " duration=" << duration << endl;
  */
  received_digits += ch;
}
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
        if (samp_cnt == 0)
        {
          break;
        }
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


class PowerPlotter : public Async::AudioPassthrough
{
  public:
    PowerPlotter(void) : prev_power(0.0f) { }

    virtual int writeSamples(const float *samples, int count)
    {
      double power = 0.0;
      for (int i=0; i<count; ++i)
      {
        power += static_cast<double>(samples[i]) * samples[i];
      }
      power /= count;
      power = ALPHA * power + (1-ALPHA) * prev_power;
      prev_power = power;
      cout << "pwr=" << power << endl;
      return sinkWriteSamples(samples, count);
    }

  private:
    static CONSTEXPR float ALPHA = 0.05;
    float prev_power;

}; /* class PowerPlotter */


int main()
{
  Config cfg;
  cfg.setValue("Test", "DTMF_DEC_TYPE", "INTERNAL");
  //cfg.setValue("Test", "DTMF_DEC_TYPE", "DH1DM");
  //cfg.setValue("Test", "DTMF_MAX_FWD_TWIST", "6");
  //cfg.setValue("Test", "DTMF_MAX_REV_TWIST", "6");
  //cfg.setValue("Test", "DTMF_HANGTIME", "100");
  cfg.setValue("Test", "DTMF_DEBUG", "1");

  AudioSource *prev_src = 0;
#define SIMULATE
#ifdef SIMULATE
  DtmfEncoder enc;
  enc.setDigitDuration(40);
  enc.setDigitSpacing(40);
  enc.setDigitPower(-10);
  prev_src = &enc;

  //float noise_pwr = 0;
  //float noise_pwr = -10;
  //float noise_pwr = -13;
  //float noise_pwr = -16;
  float noise_pwr = -22;
  //float noise_pwr = -100;
  noise_pwr += 10.0f * log10f(0.5f / 0.30f); // 300-5000Hz filter
  //noise_pwr += 10.0f * log10f(0.5f / 0.18f); // 480-3400Hz filter
  AudioNoiseAdder *noise_adder = new AudioNoiseAdder(noise_pwr);
  prev_src->registerSink(noise_adder, true);
  prev_src = noise_adder;

  AudioFilter *voiceband_filter = new AudioFilter("BpCh10/-0.1/300-5000");
  //AudioFilter *voiceband_filter = new AudioFilter("BpCh10/-0.1/480-3400");
  prev_src->registerSink(voiceband_filter, true);
  prev_src = voiceband_filter;

  /*
  PowerPlotter pp;
  prev_src->registerSink(&pp);
  prev_src = &pp;
  */
#else
  FileReader rdr;
  prev_src = &rdr;
#endif

  fwriter = new FileWriter("/tmp/samples.raw");
  prev_src->registerSink(fwriter, true);
  prev_src = fwriter;

  DtmfDecoder *dec = DtmfDecoder::create(0, cfg, "Test");
  if (!dec->initialize())
  {
    cout << "*** ERROR: Could not initialize DTMF decoder\n";
    exit(1);
  }
  dec->digitDeactivated.connect(sigc::ptr_fun(digit_detected));
  prev_src->registerSink(dec, true);
  prev_src = 0;

#ifdef SIMULATE
  int tone_amp = enc.digitPower();
  enc.setDigitPower(-100);
  enc.send("0");
  enc.setDigitPower(tone_amp);
  string digits;
  for (int i=0; i<50; ++i)
  {
    digits += send_digits;
  }
  enc.send(digits);
  enc.setDigitPower(-100);
  enc.send("0");
#else
  if (!rdr.open("/tmp/output.raw"))
  //if (!rdr.open("/tmp/selected.raw"))
  //if (!rdr.open("/tmp/false11.raw"))
  {
    exit(1);
  }
#endif
  if (!received_digits.empty())
  {
    cout << "Digits detected(" << received_digits.size() << "): "
         << received_digits << endl;
  }
#ifdef SIMULATE
  if (received_digits != digits)
  {
    cout << "received(" << received_digits.size() << ") != sent("
         << digits.size() << ") => "
         << (100.0 * received_digits.size() / digits.size()) << "%\n";
    return 1;
  }
#else
  if (!received_digits.empty())
  {
    return 1;
  }
#endif
  return 0;
}

