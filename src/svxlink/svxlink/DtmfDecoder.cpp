/*
 *----------------------------------------------------------------------------
 * Filename:  	DtmfDecoder.cpp
 * Module:    	
 * Author:    	Tobias Blomberg
 * Created:   	2003-04-16
 * License:   	GPL
 * Description: A DTMF (Dual Tone Multi Frequency) detector using the
 *    	      	ToneDetector class.
 *----------------------------------------------------------------------------
 * Signatures:
 * Sign Name  	      	  E-mail
 * TBg	Tobias Blomberg   blomman@ludd.luth.se
 *
 *----------------------------------------------------------------------------
 */




/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <stdio.h>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "ToneDetector.h"
#include "DtmfDecoder.h"



/****************************************************************************
 *
 * Namespaces to use
 *
 ****************************************************************************/

using namespace SigC;



/****************************************************************************
 *
 * Defines & typedefs
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Local class definitions
 *
 ****************************************************************************/

class DtmfToneDetector : public ToneDetector
{
  public:
    DtmfToneDetector(bool is_row, int pos, int fq)
      : ToneDetector(fq, 205), is_row(is_row), pos(pos)
    {
      ToneDetector::activated.connect(
      	  slot(this, &DtmfToneDetector::onToneActivated));
    }
    
    SigC::Signal3<void, bool, int, bool>  activated;
    
  private:
    bool  is_row;
    int   pos;
    
    void onToneActivated(bool is_activated)
    {
      activated(is_row, pos, is_activated);
    }
    
}; /* class DtmfToneDetector */



/****************************************************************************
 *
 * Prototypes
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Exported Global Variables
 *
 ****************************************************************************/




/****************************************************************************
 *
 * Local Global Variables
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Public member functions
 *
 ****************************************************************************/


/*
 *------------------------------------------------------------------------
 * Method:    
 * Purpose:   
 * Input:     
 * Output:    
 * Author:    
 * Created:   
 * Remarks:   
 * Bugs:      
 *------------------------------------------------------------------------
 */
DtmfDecoder::DtmfDecoder(void)
  : active_row(-1), active_col(-1)
{
  row[0] = new DtmfToneDetector(true, 0, 697);
  row[0]->activated.connect(slot(this, &DtmfDecoder::toneActivated));
  row[1] = new DtmfToneDetector(true, 1, 770);
  row[1]->activated.connect(slot(this, &DtmfDecoder::toneActivated));
  row[2] = new DtmfToneDetector(true, 2, 852);
  row[2]->activated.connect(slot(this, &DtmfDecoder::toneActivated));
  row[3] = new DtmfToneDetector(true, 3, 941);
  row[3]->activated.connect(slot(this, &DtmfDecoder::toneActivated));
  
  col[0] = new DtmfToneDetector(false, 0, 1209);
  col[0]->activated.connect(slot(this, &DtmfDecoder::toneActivated));
  col[1] = new DtmfToneDetector(false, 1, 1336);
  col[1]->activated.connect(slot(this, &DtmfDecoder::toneActivated));
  col[2] = new DtmfToneDetector(false, 2, 1477);
  col[2]->activated.connect(slot(this, &DtmfDecoder::toneActivated));
  col[3] = new DtmfToneDetector(false, 3, 1633);
  col[3]->activated.connect(slot(this, &DtmfDecoder::toneActivated));


} /* DtmfDecoder::DtmfDecoder */


DtmfDecoder::~DtmfDecoder(void)
{

} /* DtmfDecoder::~DtmfDecoder */


int DtmfDecoder::processSamples(short *buf, int len)
{
  //printf("len=%d\n", len);
  
  for (int i=0; i<len/160; i++)
  {
    for (int n=0; n<4; n++)
    {
      row[n]->processSamples(buf+i*160, 160);
      col[n]->processSamples(buf+i*160, 160);
    }
  }
  
  return len;
  
} /* DtmfDecoder::processSamples */



/****************************************************************************
 *
 * Protected member functions
 *
 ****************************************************************************/


/*
 *------------------------------------------------------------------------
 * Method:    
 * Purpose:   
 * Input:     
 * Output:    
 * Author:    
 * Created:   
 * Remarks:   
 * Bugs:      
 *------------------------------------------------------------------------
 */






/****************************************************************************
 *
 * Private member functions
 *
 ****************************************************************************/


/*
 *----------------------------------------------------------------------------
 * Method:    
 * Purpose:   
 * Input:     
 * Output:    
 * Author:    
 * Created:   
 * Remarks:   
 * Bugs:      
 *----------------------------------------------------------------------------
 */
void DtmfDecoder::toneActivated(bool is_row, int pos, bool is_activated)
{
  const char digit_map[4][4] =
  {
    { '1', '2', '3', 'A' },
    { '4', '5', '6', 'B' },
    { '7', '8', '9', 'C' },
    { '*', '0', '#', 'D' },
  };
  
  if (is_row)
  {
    if (is_activated)
    {
      active_row = pos;
    }
    else if (pos == active_row)
    {
      active_row = -1;
    }
  }
  else
  {
    if (is_activated)
    {
      active_col = pos;
    }
    else if (pos == active_col)
    {
      active_col = -1;
    }
  }
  
  if ((active_row != -1) && (active_col != -1))
  {
    //printf("Digit=%c\n", digit_map[active_row][active_col]);
    digitDetected(digit_map[active_row][active_col]);
  }
  
} /* DtmfDecoder::toneActivated */






/*
 * This file has not been truncated
 */

