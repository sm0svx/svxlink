/*
 *----------------------------------------------------------------------------
 * Filename:  	ToneDetector.h
 * Module:    	
 * Author:    	Tobias Blomberg
 * Created:   	2003-04-15
 * License:   	GPL
 * Description: A tone decoder that uses the Goertzel algorithm to detect
 *    	      	a tone.
 *----------------------------------------------------------------------------
 * Signatures:
 * Sign Name  	      	  E-mail
 * TBg	Tobias Blomberg   blomman@ludd.luth.se
 *
 *----------------------------------------------------------------------------
 */


#ifndef TONE_DETECTOR_INCLUDED
#define TONE_DETECTOR_INCLUDED


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
#define FLOATING	float
#define SAMPLE	      	short


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
 * Class:     ToneDetector
 * Purpose:   ToneDetector class
 * Inherits:  
 * Input:     
 * Output:    
 * Author:    
 * Created:   
 * Remarks:   
 * Bugs:      
 *----------------------------------------------------------------------------
 */   
class ToneDetector : public SigC::Object
{
  public:
    /*
     *------------------------------------------------------------------------
     * Method:	ToneDetector
     * Purpose: Constructor
     * Input: 	
     * Output:	None
     * Author:	
     * Created: 
     * Remarks: 
     * Bugs:  	
     *------------------------------------------------------------------------
     */
    ToneDetector(int tone_hz);
    ~ToneDetector(void) {}
    
    int processSamples(short *buf, int len);
    
    SigC::Signal1<void, bool> activated;
    SigC::Signal2<void, ToneDetector*, double>	valueChanged;
    
  protected:
    
  private:
    int   tone;
    int   block_pos;
    int  is_activated;
    
    FLOATING  coeff;
    FLOATING  Q1;
    FLOATING  Q2;
    FLOATING  sine;
    FLOATING  cosine;
    int       N;
    
    void resetGoertzel(void);
    void processSample(SAMPLE sample);
    void getRealImag(FLOATING *realPart, FLOATING *imagPart);
    FLOATING getMagnitudeSquared(void);

};  /* class ToneDetector */


//} /* namespace */

#endif /* TONE_DETECTOR_INCLUDED */



/*
 * This file has not been truncated
 */

