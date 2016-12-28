#include <iostream>
#include <vector>
#include <set>
#include <cstdlib>

#include <qapplication.h>
#include <qwidget.h>
#include <qpainter.h>
#include <qgroupbox.h>
#include <qprogressbar.h>
#include <qlineedit.h>

#include <AsyncQtApplication.h>
#include <AsyncAudioIO.h>
#include <AsyncTimer.h>
#include <AsyncAudioSplitter.h>
#include <SigCAudioSink.h>

#include "ToneDetector.h"
#include "DtmfDecoder.h"
#include "DtmfPlot.h"

using namespace std;
using namespace SigC;
using namespace Async;


set<int>	dtmf_k;
set<int>	dtmf_ko;


class BarPlot : public QWidget
{
  public:
    BarPlot(QWidget *parent, int bar_cnt) : QWidget(parent), bars(bar_cnt)
    {
      setMinimumSize(bar_cnt, 10);
      QSizePolicy size_policy(QSizePolicy::MinimumExpanding,
      	      	      	      QSizePolicy::MinimumExpanding);
      setSizePolicy(size_policy);
      setBackgroundColor(black);
    }
    ~BarPlot(void) {}
    
    void setHighlight(const set<int>& highlight_set)
    {
      hl_set = highlight_set;
    }
    
    void setBar(int bar, int value)
    {
      //cout << "Setting bar " << bar << " to value " << value << endl;
      if ((bar >= 0) && ((unsigned)bar < bars.size()))
      {
      	if (value < 1)	  value = 1;
	if (value > 100)  value = 100;
      	bars[bar] = value;
      	//repaint();
      }
    }
  
  private:
    QPainter p;
    vector<int> bars;
    set<int>  	hl_set;
    
    void paintEvent(QPaintEvent *)
    {
      //cout << "Painting...\n";
      p.begin(this);                        // start painting widget
      int bar_width = width() / bars.size();
      //cout << "bar_width=" << bar_width << endl;
      for (int i=0; (unsigned)i<bars.size(); ++i)
      {
      	int bar_height = height() * bars[i] / 100;
	if (hl_set.count(i) > 0)
	{
      	  p.setPen(red);
      	  p.setBrush(red);
	}
	else
	{
      	  p.setPen(yellow);
      	  p.setBrush(yellow);
	}
      	p.drawRect(i*bar_width, height()-bar_height,
	      	   bar_width, bar_height);
      }
      p.end();                              // painting done    
    }
};


DtmfPlot *plot_win = 0;
BarPlot *bar_plot = 0;
BarPlot *overtone_plot = 0;


class Det : public ToneDetector
{
  public:
    int index;
    Det(int index, int fq, int base_N)
      : ToneDetector(fq, base_N), index(index) {}
    
};


void detValueChanged(ToneDetector *tdet, double value)
{
  Det *det = dynamic_cast<Det*>(tdet);
  //bar_plot->setBar(det->index, (int)(value / 2500000000.0));
  bar_plot->setBar(det->index, (int)(value / 100000.0));
}


void overtoneValueChanged(ToneDetector *tdet, double value)
{
  Det *det = dynamic_cast<Det*>(tdet);
  //overtone_plot->setBar(det->index, (int)(value / 2500000000.0));
  overtone_plot->setBar(det->index, (int)(value / 100000.0));
}


void repaint(Timer *t)
{
  bar_plot->repaint();
  overtone_plot->repaint();
  if (t != 0)
  {
    t->reset();
  }
  else
  {
    t = new Timer(100);
    t->expired.connect(slot(&repaint));
  }
}

void dtmf_digit_detected(char digit)
{
  //printf("DTMF digit detected: %c\n", digit);
  char str[2];
  str[0] = digit;
  str[1] = 0;
  plot_win->digit_view->setCursorPosition(
      	  plot_win->digit_view->text().length());
  plot_win->digit_view->insert(str);
} /*  */


int update_peak_meter(float *samples, int count)
{
  static short peak = 0;
  static int tot_count = 0;
  
  for (int i=0; i<count; ++i)
  {
    short sample = static_cast<short>(samples[i] * 32767.0);
    if (abs(sample) > peak)
    {
      peak = abs(sample);
    }
    if (++tot_count >= INTERNAL_SAMPLE_RATE / 10)
    {
      tot_count = 0;
      plot_win->peak_meter->setProgress(peak);
      //printf("peak=%d\n", peak);
      if (peak >= 32700)
      {
      	plot_win->max_peak_indicator->setBackgroundColor("red");
      }
      else
      {
      	plot_win->max_peak_indicator->setBackgroundColor("black");      
      }
      peak = 0;
    }
  }
  
  return count;
  
} /*  */


int fq_to_k(int N, int fq)
{
  int k = (int)(0.5 + fq * (float)N / (float)INTERNAL_SAMPLE_RATE);
  //printf("k=%d\n", k);
  return k;
}

int main(int argc, char **argv)
{
  QtApplication app(argc, argv);
  
  const int basetone_N = 205;
  const int overtone_N = 201;
  const int start_k = fq_to_k(basetone_N, 697) - 3; /*15*/
  const int end_k = fq_to_k(basetone_N, 1633) + 3; /*50*/
  const int start_ko = 32;
  const int end_ko = 85;
    
  dtmf_k.insert(fq_to_k(basetone_N, 697)-start_k);
  dtmf_k.insert(fq_to_k(basetone_N, 770)-start_k);
  dtmf_k.insert(fq_to_k(basetone_N, 852)-start_k);
  dtmf_k.insert(fq_to_k(basetone_N, 941)-start_k);
  
  dtmf_k.insert(fq_to_k(basetone_N, 1209)-start_k);
  dtmf_k.insert(fq_to_k(basetone_N, 1336)-start_k);
  dtmf_k.insert(fq_to_k(basetone_N, 1477)-start_k);
  dtmf_k.insert(fq_to_k(basetone_N, 1633)-start_k);
  
  dtmf_ko.insert(35-start_ko);
  dtmf_ko.insert(39-start_ko);
  dtmf_ko.insert(43-start_ko);
  dtmf_ko.insert(47-start_ko);
  
  dtmf_ko.insert(61-start_ko);
  dtmf_ko.insert(67-start_ko);
  dtmf_ko.insert(74-start_ko);
  dtmf_ko.insert(82-start_ko);
  
  plot_win = new DtmfPlot;
  plot_win->base_plot->setColumns(1);
  plot_win->overtone_plot->setColumns(1);
  
  bar_plot = new BarPlot(plot_win->base_plot, end_k-start_k+1);
  bar_plot->setHighlight(dtmf_k);
  overtone_plot = new BarPlot(plot_win->overtone_plot, end_ko-start_ko+1);
  overtone_plot->setHighlight(dtmf_ko);
  
  AudioIO audio_io("/dev/dsp", 0);
  if (!audio_io.open(AudioIO::MODE_RD))
  {
    printf("*** ERROR: Could not open audio device /dev/dsp\n");
    exit(1);
  }
  
  AudioSplitter splitter;
  splitter.registerSource(&audio_io);
  
  ToneDetector *det;
  
  for (int k=start_k; k<end_k+1; ++k)
  {
    det = new Det(k - start_k, k * INTERNAL_SAMPLE_RATE / basetone_N +
      	      	      	       (INTERNAL_SAMPLE_RATE/2) / basetone_N,
		  basetone_N);
    det->valueChanged.connect(slot(&detValueChanged));
    splitter.addSink(det, true);
  }
  
  for (int k=start_ko; k<end_ko+1; ++k)
  {
    det = new Det(k - start_ko, k * INTERNAL_SAMPLE_RATE / overtone_N +
      	      	      	      	(INTERNAL_SAMPLE_RATE/2) / overtone_N,
		  overtone_N);
    det->valueChanged.connect(slot(&overtoneValueChanged));
    splitter.addSink(det, true);
  }

  SigCAudioSink sigc_sink;
  splitter.addSink(&sigc_sink);
  sigc_sink.sigWriteSamples.connect(slot(&update_peak_meter));
  
  DtmfDecoder dtmf_dec;
  //sigc_sink.sigWriteSamples.connect(
  //    slot(&dtmf_dec, &DtmfDecoder::processSamples));
  dtmf_dec.digitActivated.connect(slot(&dtmf_digit_detected));
  splitter.addSink(&dtmf_dec);
  
  repaint(0);
  
  app.setMainWidget(plot_win);
  plot_win->show();
  app.exec();

}

