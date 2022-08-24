#include <cstdlib>
#include <iostream>

#include <AsyncCppApplication.h>
#include <AsyncConfig.h>
#include <AsyncAudioIO.h>
#include <AsyncAudioFifo.h>

#include <Tx.h>

using namespace std;
using namespace Async;

int main()
{
  CppApplication app;
  Config cfg;
  string cfg_file = "tx_test.cfg";
  if (!cfg.open(cfg_file))
  {
    cout << "*** ERROR: Could not open config file " << cfg_file << "\n";
    exit(1);
  }

  AudioIO audio_in("udp:localhost:10001", 0);
  if (!audio_in.open(AudioIO::MODE_RD))
  {
    cout << "*** ERROR: Could not open audio input device\n";
    exit(1);
  }
  AudioSource *prev_src = &audio_in;

  AudioFifo input_fifo(16384);
  prev_src->registerSink(&input_fifo);
  prev_src = &input_fifo;

  Tx *tx = TxFactory::createNamedTx(cfg, "Tx");
  if ((tx == 0) || !tx->initialize())
  {
    cout << "*** ERROR: Could not set up TX object\n";
    exit(1);
  }
  tx->setTxCtrlMode(Tx::TX_AUTO);
  prev_src->registerSink(tx);

#if 1
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
    tx->sendData(frame);
  }
#endif

  app.exec();
  return 0;
}

