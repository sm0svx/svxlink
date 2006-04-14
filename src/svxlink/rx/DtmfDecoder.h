/*
 *----------------------------------------------------------------------------
 * Filename:  	DtmfDecoder.h
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


#ifndef DTMF_DECODER_INCLUDED
#define DTMF_DECODER_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <sigc++/signal_system.h>


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



/****************************************************************************
 *
 * Forward declarations
 *
 ****************************************************************************/

class DtmfToneDetector;


/****************************************************************************
 *
 * Namespace
 *
 ****************************************************************************/

//namespace MyNameSpace
//{

/****************************************************************************
 *
 * Defines & typedefs
 *
 ****************************************************************************/

/*
 *----------------------------------------------------------------------------
 * Macro:   
 * Purpose: 
 * Input:   
 * Output:  
 * Author:  
 * Created: 
 * Remarks: 
 * Bugs:    
 *----------------------------------------------------------------------------
 */


/*
 *----------------------------------------------------------------------------
 * Type:    
 * Purpose: 
 * Members: 
 * Input:   
 * Output:  
 * Author:  
 * Created: 
 * Remarks: 
 *----------------------------------------------------------------------------
 */


/****************************************************************************
 *
 * Exported Global Variables
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Class definitions
 *
 ****************************************************************************/

/*
 *----------------------------------------------------------------------------
 * Class:     DtmfDecoder
 * Purpose:   DtmfDecoder class
 * Inherits:  
 * Input:     
 * Output:    
 * Author:    
 * Created:   
 * Remarks:   
 * Bugs:      
 *----------------------------------------------------------------------------
 */   
class DtmfDecoder : public SigC::Object
{
  public:
    /*
     *------------------------------------------------------------------------
     * Method:	DtmfDecoder
     * Purpose: Constructor
     * Input: 	
     * Output:	None
     * Author:	
     * Created: 
     * Remarks: 
     * Bugs:  	
     *------------------------------------------------------------------------
     */
    DtmfDecoder(void);
    ~DtmfDecoder(void);
    
    int processSamples(short *buf, int len);
    
    SigC::Signal1<void, char> digitDetected;
    
    
  protected:
    
  private:
    DtmfToneDetector  *row[4];
    DtmfToneDetector  *col[4];
    int       	      active_row;
    int       	      active_col;
    short     	      sample_buf[512];
    int       	      buffered_samples;
    char      	      last_detected_digit;
    bool      	      digit_activated;

    void checkTones(void);
    
};  /* class DtmfDecoder */


//} /* namespace */

#endif /* DTMF_DECODER_INCLUDED */



/*
 * This file has not been truncated
 */

