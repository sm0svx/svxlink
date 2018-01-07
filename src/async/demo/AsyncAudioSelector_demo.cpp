#include <string.h>

#include <AsyncAudioSelector.h>
#include <AsyncAudioDebugger.h>
#include <AsyncSigCAudioSource.h>

using namespace Async;

int main()
{
  AudioSelector selector;
  AudioDebugger dbg(&selector);
  dbg.setName("selector");

  SigCAudioSource s1;
  selector.addSource(&s1);
  //selector.selectSource(&s1);
  selector.enableAutoSelect(&s1, 0);
  selector.setFlushWait(&s1, false);
  SigCAudioSource s2;
  selector.addSource(&s2);
  //selector.selectSource(&s2);
  selector.enableAutoSelect(&s2, 0);

  float samples[256];
  memset(samples, 0, sizeof(samples));
  s1.writeSamples(samples, 1);
  s2.writeSamples(samples, 2);
  s1.flushSamples();
  s2.flushSamples();
}
