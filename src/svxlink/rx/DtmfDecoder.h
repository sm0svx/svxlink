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

#include <AudioSink.h>


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
class DtmfDecoder : public SigC::Object, public Async::AudioSink
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
    
    //int processSamples(float *buf, int len);
    
    /**
     * @brief 	Write samples into this audio sink
     * @param 	samples The buffer containing the samples
     * @param 	count The number of samples in the buffer
     * @return	Returns the number of samples that has been taken care of
     */
    int writeSamples(const float *samples, int count);
    
    /**
     * @brief 	Tell the sink to flush the previously written samples
     *
     * This function is used to tell the sink to flush previously written
     * samples. When done flushing, the sink should call the
     * sourceAllSamplesFlushed function.
     */
    void flushSamples(void)
    {
      sourceAllSamplesFlushed();
    }
    
    SigC::Signal1<void, char> digitDetected;
    
    
  protected:
    
  private:
    DtmfToneDetector  *row[4];
    DtmfToneDetector  *col[4];
    int       	      active_row;
    int       	      active_col;
    float     	      sample_buf[512];
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

