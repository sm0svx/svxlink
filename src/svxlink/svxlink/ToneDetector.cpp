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




/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <stdio.h>
#include <math.h>
#include <assert.h>

#include <stdlib.h>
#include <iostream>

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



/****************************************************************************
 *
 * Namespaces to use
 *
 ****************************************************************************/

//using namespace std;



/****************************************************************************
 *
 * Defines & typedefs
 *
 ****************************************************************************/

//#define SAMPLE	unsigned char

#define SAMPLING_RATE	16000.0	//8kHz
//#define TARGET_FREQUENCY	941.0	//941 Hz
//#define BASE_N	205	//Block size
//#define N	201	//Block size
#define BASE_N	205	//Block size
#define PI  M_PI

//#define THRESHOLD   5000000000000
//#define THRESHOLD   2000000000000
//#define THRESHOLD   500000000000
#define THRESHOLD   250000000000.0



/****************************************************************************
 *
 * Local class definitions
 *
 ****************************************************************************/



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
ToneDetector::ToneDetector(int tone_hz)
  : tone(tone_hz), block_pos(0), is_activated(0)
{
  int 	    k;
  FLOATING  floatN;
  FLOATING  omega;

  floatN = (FLOATING) BASE_N;
  N = BASE_N;
  k = (int) (0.5 + ((2 * floatN * tone_hz) / SAMPLING_RATE));
  //k = (int)(((floatN * tone_hz) / SAMPLING_RATE));
  //N = (int)(k * SAMPLING_RATE / tone_hz + 0.5);
  floatN = N;
  omega = (2.0 * PI * k) / floatN;
  sine = sin(omega);
  cosine = cos(omega);
  coeff = 2.0 * cosine;

  //printf("For SAMPLING_RATE = %f", SAMPLING_RATE);
  //printf(" N = %.2f", floatN);
  //printf(" and FREQUENCY = %d,\n", tone_hz);
  //printf("k = %d and coeff = %f\n\n", k, coeff);

  resetGoertzel();

} /* ToneDetector::ToneDetector */


int ToneDetector::processSamples(short *buf, int len)
{
  //assert(len % 2 == 0);

  //SAMPLE *samples = (short *)buf;
  //len /= 2;
  
  //printf("Processing %d samples\n", len);
  
  for (int i=0; i<len; i++)
  {
    processSample(buf[i]);
    if (++block_pos >= N)
    {
      FLOATING result = getMagnitudeSquared();
      valueChanged(this, result);
      if (result >= THRESHOLD)
      {
	if (!is_activated)
	{
      	  //printf("tone=%4d  result=%14.0f  activated\n", tone, result);
	  //is_activated = true;
	  activated(true);
	}
      	is_activated = 3;
      }
      else if (is_activated && (result < THRESHOLD))
      {
      	--is_activated;
	if (!is_activated)
	{
      	  //printf("tone=%4d  result=%14.0f deactivated\n", tone, result);
	  //is_activated = false;
	  activated(false);
	}
      }
      resetGoertzel();
      block_pos = 0;
    }
  }
  
  return len;
  
} /* ToneDetector::processSamples */



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

void ToneDetector::resetGoertzel(void)
{
  Q2 = 0;
  Q1 = 0;
} /* ToneDetector::resetGoertzel */


void ToneDetector::processSample(SAMPLE sample)
{
  //sample = (short)0x7fff;
  //unsigned short usample = ((int)sample + 0x8000);
  //cout << "sample=" << sample << "  usample=" << usample << endl;
  //exit(0);
  FLOATING Q0;
  Q0 = coeff * Q1 - Q2 + (FLOATING) sample;
  Q2 = Q1;
  Q1 = Q0;
} /* ToneDetector::processSample */


/* Basic Goertzel */
/* Call this routine after every block to get the complex result. */
void ToneDetector::getRealImag(FLOATING *realPart, FLOATING *imagPart)
{
  *realPart = (Q1 - Q2 * cosine);
  *imagPart = (Q2 * sine);
}

/* Optimized Goertzel */
/* Call this after every block to get the RELATIVE magnitude squared. */
FLOATING ToneDetector::getMagnitudeSquared(void)
{
  FLOATING result;

  result = Q1 * Q1 + Q2 * Q2 - Q1 * Q2 * coeff;
  return result;
}


/*
 * This file has not been truncated
 */

