#include <iostream>
#include <vector>
#include <set>

#include <qapplication.h>
#include <qwidget.h>
#include <qpainter.h>

#include <AsyncQtApplication.h>
#include <AsyncAudioIO.h>
#include <AsyncTimer.h>

#include "ToneDetector.h"
#include "DtmfDecoder.h"

using namespace std;
using namespace SigC;
using namespace Async;


set<int>	dtmf_k;


class BarPlot : public QWidget
{
  public:
    BarPlot(int bar_cnt) : bars(bar_cnt)
    {
      
    }
    ~BarPlot(void) {}
    
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
    
    void paintEvent(QPaintEvent *)
    {
      //cout << "Painting...\n";
      setBackgroundColor(black);
      p.begin(this);                        // start painting widget
      int bar_width = width() / bars.size();
      //cout << "bar_width=" << bar_width << endl;
      for (int i=0; (unsigned)i<bars.size(); ++i)
      {
      	int bar_height = height() * bars[i] / 100;
	if (dtmf_k.count(i) > 0)
	{
      	  p.setPen(red);                        // blue outline
      	  p.setBrush(red);                   // yellow fill
	}
	else
	{
      	  p.setPen(yellow);                        // blue outline
      	  p.setBrush(yellow);                   // yellow fill
	}
      	p.drawRect(i*bar_width, height()-bar_height,
	      	   bar_width, bar_height);
      }
      //p.drawEllipse(10,20, 100,100);        // 100x100 ellipse at 10,20
      p.end();                              // painting done    
    }
};


BarPlot *bar_plot = 0;


class Det : public ToneDetector
{
  public:
    int index;
    Det(int index, int fq) : ToneDetector(fq), index(index) {}
    
};


void detValueChanged(ToneDetector *tdet, double value)
{
  Det *det = dynamic_cast<Det*>(tdet);
  bar_plot->setBar(det->index, (int)(value / 2500000000.0));
  //bar_plot->setBar(det->index, (int)(value / 15000));
}


void repaint(Timer *t)
{
  bar_plot->repaint();
  delete t;
  Timer *repaint_timer = new Timer(500);
  repaint_timer->expired.connect(slot(&repaint));
}

void dtmf_digit_detected(char digit)
{
  printf("DTMF digit detected: %c\n", digit);
} /*  */


int main(int argc, char **argv)
{
  //QApplication app(argc, argv);
  QtApplication app(argc, argv);
  
  const int start_k = 15;
  const int end_k = 50;
  
  dtmf_k.insert(18-start_k);
  dtmf_k.insert(20-start_k);
  dtmf_k.insert(22-start_k);
  dtmf_k.insert(24-start_k);
  dtmf_k.insert(31-start_k);
  dtmf_k.insert(34-start_k);
  dtmf_k.insert(38-start_k);
  dtmf_k.insert(42-start_k);
  
  bar_plot = new BarPlot(end_k-start_k+1);
  //bar_plot->setBar(0, 10);
  //bar_plot->setBar(1, 20);
  //bar_plot->setBar(2, 30);
  
  AudioIO *audio_io = new AudioIO("/dev/dsp");
  audio_io->open(AudioIO::MODE_RD);
  
  ToneDetector *det;
  
  for (int k=start_k; k<end_k+1; ++k)
  {
    det = new Det(k-start_k,k*8000/205+8000/410);
    det->valueChanged.connect(slot(&detValueChanged));
    audio_io->audioRead.connect(slot(det, &ToneDetector::processSamples));
  }
  
  DtmfDecoder dtmf_dec;
  audio_io->audioRead.connect(slot(&dtmf_dec, &DtmfDecoder::processSamples));
  dtmf_dec.digitDetected.connect(slot(&dtmf_digit_detected));
  
  //repaint(0);
    
  app.setMainWidget(bar_plot);
  bar_plot->show();
  app.exec();

}

