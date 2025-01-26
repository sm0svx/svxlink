/******************************************************************************
 *
 * Test with something like:
 * sox TNC_Test_Ver-1.1-01.wav -t raw -esigned-integer -c1 -r 16000 - | \
 * valgrind --leak-check=full svxlink/digital/afsk_test
 *
 ******************************************************************************/

#include <iostream>
#include <iomanip>
#include <string>

#include <cstdlib>
#include <cstring>
#include <unistd.h>

#include <AsyncCppApplication.h>
#include <AsyncAudioIO.h>
#include <AsyncAudioFifo.h>
#include <AsyncAudioDebugger.h>
#include <AsyncAudioSplitter.h>
#include <AsyncAudioPacer.h>
#include <AsyncAudioFsf.h>

#include <Rx.h>

#include "AfskDemodulator.h"
#include "Synchronizer.h"
#include "HdlcDeframer.h"
#include "AfskModulator.h"
#include "HdlcFramer.h"


using namespace std;
using namespace Async;


struct AX25Frame
{
  struct Address
  {
    string callsign;
    uint8_t ssid;
  };

  struct DigiAddress : public Address
  {
    bool has_been_repeated;
  };

  Address src;
  Address dest;
  vector<DigiAddress> digis;
  uint8_t ctrl;
  uint8_t pid;
  vector<uint8_t> info;

  AX25Frame(void) {}

  AX25Frame(const AX25Frame &fm) { *this = fm; }

  virtual ~AX25Frame(void) {}

  void decode(vector<uint8_t> frame)
  {
    unsigned byteno = 0;

    if (frame.size() < 14)
    {
      return;
    }

    for (int i=0; i<6; ++i)
    {
      char ch = frame[byteno++] >> 1;
      if (ch != ' ')
      {
        dest.callsign += ch;
      }
    }
    dest.ssid = (frame[byteno++] >> 1) & 0x0f;

    for (int i=0; i<6; ++i)
    {
      char ch = frame[byteno++] >> 1;
      if (ch != ' ')
      {
        src.callsign += ch;
      }
    }
    src.ssid = (frame[byteno] >> 1) & 0x0f;
    bool has_digis = ((frame[byteno++] & 0x01) == 0);

    while (has_digis)
    {
      if (frame.size() < 7)
      {
        return;
      }
      AX25Frame::DigiAddress digi;
      for (int i=0; i<6; ++i)
      {
        char ch = frame[byteno++] >> 1;
        if (ch != ' ')
        {
          digi.callsign += ch;
        }
      }
      digi.ssid = (frame[byteno] >> 1) & 0x0f;
      digis.push_back(digi);
      digi.has_been_repeated = (frame[byteno] & 0x80);
      has_digis = ((frame[byteno++] & 0x01) == 0);
    }

    if (frame.size() < 2)
    {
      return;
    }
    ctrl = frame[byteno++];
    if (((ctrl & 0x01) == 0) || ((ctrl & 0xec)) == 0)
    {
      pid = frame[byteno++];
      for (size_t i=byteno; i<frame.size(); ++i)
      {
        info.push_back(frame[byteno++]);
      }
    }
  }

  void print(void) const
  {
    time_t t = time(NULL);
    struct tm tm;
    char buf[256];
    strftime(buf, sizeof(buf), "%Y-%m-%d %T", localtime_r(&t, &tm));
    //cout << "\a";
    cout << "-- [" << buf << "] ";
    cout << src.callsign;
    if (src.ssid != 0)
    {
      cout << "-" << (unsigned)src.ssid;
    }
    cout << ">";
    cout << dest.callsign;
    if (dest.ssid != 0)
    {
      cout << "-" << (unsigned)dest.ssid;
    }
    for (size_t i=0; i<digis.size(); ++i)
    {
      cout << "," << digis[i].callsign << "-"
           << (unsigned)digis[i].ssid;
      if (digis[i].has_been_repeated)
      {
        cout << "*";
      }
    }
    cout << ": ";

    if ((ctrl & 0x01) == 0)
    {
      cout << "INFO";
    }
    else if ((ctrl & 0x03) == 0x01)
    {
      cout << "S";
    }
    else
    {
      switch (ctrl & 0xec)
      {
        case 0x2c:
          cout << "SABM";
          break;
        case 0x40:
          cout << "DISC";
          break;
        case 0x0c:
          cout << "DM";
          break;
        case 0x60:
          cout << "UA";
          break;
        case 0x84:
          cout << "FRMR";
          break;
        case 0x00:
          cout << "UI";
          break;
        default:
          cout << "???";
          break;
      }

    }
    cout << " --" << endl;

    if (!info.empty())
    {
      for (size_t i=0; i<info.size(); ++i)
      {
        if (isprint(info[i]))
        {
          cout << (char)info[i];
        }
        else
        {
          cout << "<" << hex << setw(2) << setfill('0')
               << (int)info[i] << dec << ">";
        }
      }
      cout << endl;
    }
  }


};


class AX25Decoder : public sigc::trackable
{
  public:
    AX25Decoder(void) : frameno(0), pkt_cnt(0) {}

    void frameReceived(vector<uint8_t> frame)
    {
      AX25Frame ax25_frame;
      ax25_frame.decode(frame);
#if 1
      //cout << "\a";
      cout << setw(3) << ++frameno << ": ";
      ax25_frame.print();
#else
      ++pkt_cnt;
      cout << "\r" << dec << pkt_cnt << " " << flush;
#endif
    }

  private:
    unsigned frameno;
    unsigned pkt_cnt;
};


class StdInSource : public Async::AudioSource, public sigc::trackable
{
  public:
    StdInSource(void)
      : bufpos(0), watch(STDIN_FILENO, FdWatch::FD_WATCH_RD)
    {
      watch.activity.connect(hide(mem_fun(*this, &StdInSource::readAudio)));
    }

    void readAudio(void)
    {
      size_t bufspace = sizeof(buf) - bufpos;
      if (bufspace > 0)
      {
        ssize_t cnt = read(STDIN_FILENO, buf+bufpos, bufspace);
        if (cnt == -1)
        {
          perror("read");
          exit(1);
        }
        else if (cnt == 0)
        {
          sinkFlushSamples();
          cout << "\n### End of file\n";
          watch.setEnabled(false);
        }

        bufpos += cnt;
      }

      if (bufpos == sizeof(buf))
      {
        int16_t *isamples = reinterpret_cast<int16_t *>(buf);
        float samples[sizeof(buf)/2];
        for (size_t i=0; i<sizeof(buf)/2; ++i)
        {
          samples[i] = isamples[i] / 32768.0f;
        }
        int written = sinkWriteSamples(samples, sizeof(buf) / 2);
        if (written == 0)
        {
          watch.setEnabled(false);
        }
        else if (written != sizeof(buf) / 2)
        {
          /*
          cout << "### Only " << written << " samples of "
               << (sizeof(buf) / 2) << " were written\n";
          */

          memmove(buf, buf+written*2, sizeof(buf)-written*2);
          bufpos -= written*2;
        }
        else
        {
          bufpos = 0;
        }
      }
    }

    virtual void resumeOutput(void)
    {
      //cout << "### resumeOutput\n";
      watch.setEnabled(true);
    }

    virtual void allSamplesFlushed(void)
    {
      //cout << "### allSamplesFlushed\n";
      Application::app().quit();
    }

  private:
    uint8_t buf[512];
    unsigned bufpos;
    FdWatch watch;

}; /* class StdInSource */


class StdInSourceDev : public StdInSource
{
  public:
    bool open(int mode) { return true; }
};


int main(int argc, const char **argv)
{
  // 1200Bd AMPR
#if 1
  unsigned baudrate = 1200;
  unsigned f0 = 1200;
  unsigned f1 = 2200;
#endif

  // Test
#if 0
  unsigned baudrate = 1600;
  unsigned f0 = 1250;
  unsigned f1 = 2750;
#endif

  // 2400Bd AMPR
#if 0
  unsigned baudrate = 2400;
  unsigned f0 = 2165;
  unsigned f1 = 3970;
#endif

  // SvxLink out of band AFSK
#if 0
  unsigned baudrate = 300;
  unsigned f0 = 5415;
  unsigned f1 = 5585;
#endif

  // 300Bd AMPR
#if 0
  unsigned baudrate = 300;
  unsigned f0 = 1600;
  unsigned f1 = 1800;
#endif

  // RTTY 50Bd, 170Hz
#if 0
  unsigned baudrate = 50;
  unsigned f0 = 900;
  unsigned f1 = 1070;
#endif

  // RTTY 50Bd, 425Hz
#if 0
  unsigned baudrate = 50;
  unsigned f0 = 788;
  unsigned f1 = 1213;
#endif

  // RTTY 50Bd, 850Hz
#if 0
  unsigned baudrate = 50;
  unsigned f0 = 576;
  unsigned f1 = 1426;
#endif

  // RTTY 50Bd, 450Hz (SYNOP/SHIP)
#if 0
  unsigned baudrate = 50;
  unsigned f0 = 550;
  unsigned f1 = 1000;
#endif

  unsigned sample_rate = 16000;

  CppApplication app;

  AudioIO::setSampleRate(sample_rate);
  AudioIO::setBlocksize(256);
  AudioIO::setBlockCount(4);

  AudioSource *prev_src = 0;

  //AudioIO audio_in("udp:127.0.0.1:10000", 0);
  //AudioIO audio_io("alsa:plughw:0", 0);

  //StdInSourceDev audio_in;
  StdInSource audio_in;
  prev_src = &audio_in;
  //AudioFifo fifo(2048);
  //prev_src->registerSink(&fifo);
  //prev_src = &fifo;

  if ((f0 == 5415) && (f1 == 5585) && (baudrate == 300))
  {
    const size_t N = 128;
    float coeff[N/2+1];
    memset(coeff, 0, sizeof(coeff));
    coeff[42] = 0.39811024;
    coeff[43] = 1.0;
    coeff[44] = 1.0;
    coeff[45] = 1.0;
    coeff[46] = 0.39811024;
    AudioFsf *fsf = new AudioFsf(N, coeff);
    prev_src->registerSink(fsf, true);
    prev_src = fsf;
  }

  /*
  AudioFilter deemph_filt("HpBu1/50 x LpBu1/150");
  deemph_filt.setOutputGain(2.27);
  prev_src->registerSink(&deemph_filt);
  prev_src = &deemph_filt;
  */

  string cfg_filename(getenv("HOME"));
  cfg_filename += "/.svxlink/svxlink.conf";
  Async::Config cfg;
  if (!cfg.open(cfg_filename))
  {
    cout << "*** ERROR: Could not open configuration file: "
         << cfg_filename << endl;
    exit(1);
  }

  //string rx_name("RxRtl");
  //Rx *rx = RxFactory::createNamedRx(cfg, rx_name);
  //if ((rx == 0) || !rx->initialize())
  //{
  //  cout << "*** ERROR: Could not initialize receiver " << rx_name << endl;
  //  exit(1);
  //}
  //rx->setMuteState(Rx::MUTE_NONE);
  //prev_src = rx;

  //AudioSplitter audio_splitter;
  //prev_src->registerSink(&audio_splitter);
  //prev_src = &audio_splitter;

  //AudioIO audio_out("alsa:default", 0);
  //if (!audio_io.open(AudioIO::MODE_RDWR))
  //{
  //  cerr << "*** ERROR: Could not open audio device\n";
  //  exit(1);
  //}
  //audio_splitter.addSink(&audio_io);

  AfskDemodulator fsk_demod(f0, f1, baudrate, sample_rate);
  prev_src->registerSink(&fsk_demod);
  prev_src = &fsk_demod;

  AudioSplitter splitter;
  prev_src->registerSink(&splitter);
  prev_src = 0;

#if 0
  HdlcFramer framer;

  AfskModulator fsk_mod(f0, f1, baudrate, -6, sample_rate);
  framer.sendBits.connect(mem_fun(fsk_mod, &AfskModulator::sendBits));
  prev_src = &fsk_mod;

  AudioPacer pacer(sample_rate, 256, 0);
  prev_src->registerSink(&pacer);
  prev_src = &pacer;

#if 0
  AudioDebugger d;
  prev_src->registerSink(&d);
  prev_src = &d;
#endif

  AudioIO audio_out("udp:127.0.0.1:10002", 0);
  splitter.addSink(&audio_out);
  //AudioIO audio_out("alsa:plughw:0", 0);
  //prev_src->registerSink(&audio_out);
  prev_src = 0;
#endif

  Synchronizer sync(baudrate, sample_rate);
  splitter.addSink(&sync);

  HdlcDeframer deframer;
  sync.bitsReceived.connect(mem_fun(deframer, &HdlcDeframer::bitsReceived));

  AX25Decoder decoder;
  deframer.frameReceived.connect(mem_fun(decoder, &AX25Decoder::frameReceived));

  //if (!audio_out.open(AudioIO::MODE_WR))
  //{
  //  cerr << "*** ERROR: Could not open audio device for writing\n";
  //  exit(1);
  //}
  //splitter.addSink(&audio_out);
  //if (!audio_in.open(AudioIO::MODE_RD))
  //{
  //  cerr << "*** ERROR: Could not open audio device for reading\n";
  //  exit(1);
  //}

  /*
  vector<bool> bits;
  for (int i=0; i<12000/2; ++i)
  {
    bits.push_back(false);
    bits.push_back(true);
  }
  fsk_mod.sendBits(bits);
  */

#if 0
  vector<uint8_t> frame;
  frame.push_back(0x96);
  frame.push_back(0x70);
  frame.push_back(0x9a);
  frame.push_back(0x9a);
  frame.push_back(0x9e);
  frame.push_back(0x40);
  frame.push_back(0xe0);
  frame.push_back(0xae);
  frame.push_back(0x84);
  frame.push_back(0x68);
  frame.push_back(0x94);
  frame.push_back(0x8c);
  frame.push_back(0x92);
  frame.push_back(0x61);
  frame.push_back(0x3e);
  frame.push_back(0xf0);
  /*
  frame.push_back(0xaa);
  frame.push_back(0x55);
  */
  for (int i=0; i<100; ++i)
  {
    framer.sendBytes(frame);
  }
#endif

  app.exec();

  return 0;
}

