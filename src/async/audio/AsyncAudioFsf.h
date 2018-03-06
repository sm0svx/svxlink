/**
@file	 AsyncAudioFsf.h
@brief   A Frequency Sampling Filter implementation
@author  Tobias Blomberg / SM0SVX
@date	 2018-01-03

\verbatim
Async - A library for programming event driven applications
Copyright (C) 2003-2018 Tobias Blomberg / SM0SVX

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
\endverbatim
*/

/** @example AsyncAudioFsf_demo.cpp
An example of how to use the Async::AudioFsf class
*/

#ifndef ASYNC_AUDIO_FSF_INCLUDED
#define ASYNC_AUDIO_FSF_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <vector>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncAudioProcessor.h>


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

namespace Async
{


/****************************************************************************
 *
 * Forward declarations of classes inside of the declared namespace
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Defines & typedefs
 *
 ****************************************************************************/



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

/**
@brief	A Frequency Sampling Filter implementation
@author Tobias Blomberg / SM0SVX
@date   2018-01-03

This class implements a frequency sampling filter, FSF, as described in
"Understanding Digital Signal Processing" by "Richard G. Lyons" third edition
chapter 7.5. The variant that is implemented is the even 'N' real only variant
with a modified type-IV resonator supporting the case I constellation. A
frequency sampling filter is a FIR filter that consists of a cascade of comb
filters and a number of recursive resonators. Even though this filter contains
both non-recursive and recursive filters it has finate impulse response and
linear phase characteristics so it can replace a standard FIR filter in some
applications.

Even though an optimally designed Parks-McClellan FIR filter often have better
performace in terms of stop band attenuation vs. transition bandwidth, the
frequency sampling filter may be much more computationally efficient. As a rule
of thumb, the frequency sampling filter is more computationally efficient than
a PM-designed FIR filter when the passband is less than fs/5 and the transition
bandwidth is less than fs/8, where fs is the sampling frequency.

The base of the FSF (frequency sampling filter) is formed by two cascaded comb
filters. To define the frequency response of the filter, one or more resonators
are added in parallel after the comb filters. Each resonator will pass
frequencies falling within that "bin". The number of available bins are defined
using the 'N' design parameter. The binwidth is fs/N. In the real-only case the
bins 0 to N/2 can be used. The 'coeff' array is used to specify the stopband(s)
and passbands(s). Simply put, a 0 designates the bin as a stopband and a 1
defines a passband. In reality values in between zero and one is often used to
improve filter performance with transition band coefficients. These are not
easily calculated so design tables are often used, like appendix H in the book
mentioned initially.

Since the numeric precision in computers are finite a little tweak of the
theory is required. A dampening factor is added to make the filter stable even
though rounding errors are present. Whithout this dampening factor the filter
poles may end up outside of the unit circle which would make the filter
unstable. The drawback is that the filter will not have exactly linear phase
but the approximation should be enough for most applications since it's very
close to linear.

As a design example, let us assume we want a bandpass filter centered around
5500Hz with a 3dB bandwidth of about 400Hz. The sampling frequency is 16000Hz.
The higher 'N' we choose, the more exact the passband will be in frequency and
the narrowser the transition bandwidth will be. However, when choosing a high
'N' we also increase the number of resonators needed to cover the passband
which increase the computational load. We thus like to keep 'N' as low as
possible. In the example above we need a quite high 'N' to get a narrow
passband that still have a flat frequency response so we choose N=128. To get
the index of the center frequency we calculate 5500/(fs/N)=44. We thus set
coeff[44]=1 and also do the same for coeff[43] and coeff[45] to get
approximately the desired bandwidth. To improve the stopband attenutation (from
like 15 to 40dB) we also need to add a couple of coefficients for the
transition regions of the filter so we set coeff[42] and coeff[46]=0.39811024.
That value has been looked up in the table mentioned above. All other positions
must be set to 0 to form the stop band. The dampening factor 'r' should be left
at its default unless there is a good reason to change it.

\image html AsyncAudioFsfExample.png "Example filter frequency response (blue) and phase response (red)"

\include AsyncAudioFsf_demo.cpp
*/
class AudioFsf : public Async::AudioProcessor
{
  public:
    /**
     * @brief 	Default constructor
     * @param   N     The number of filter "bins"
     * @param   coeff The N/2+1 coefficients defining the filter
     * @param   r     The dampening factor
     *
     * See class description for how to choose the argument values. The coeff
     * array must have the length N/2+1 where coeff[0] is the filter section
     * representing DC and the coeff[N/2] filter section represents fs/2.
     * coeff[0] must be set to implement a lowpass filter and coeff[N/2] must
     * be set to implement a highpass filter.
     */
    AudioFsf(size_t N, const float *coeff, float r=0.99999);

    /**
     * @brief 	Destructor
     */
    ~AudioFsf(void);

  protected:
    /**
     * @brief Process incoming samples and put them into the output buffer
     * @param dest  Destination buffer
     * @param src   Source buffer
     * @param count Number of samples in the source buffer
     *
     * This function do the actual processing of the incoming samples.
     */
    virtual void processSamples(float *dest, const float *src, int count);

  private:
    class CombFilter;
    class Resonator;

    CombFilter *            m_combN;
    CombFilter *            m_comb2;
    std::vector<Resonator*> m_resonators;

    AudioFsf(const AudioFsf&);
    AudioFsf& operator=(const AudioFsf&);

};  /* class AudioFsf */


} /* namespace */

#endif /* ASYNC_AUDIO_FSF_INCLUDED */

/*
 * This file has not been truncated
 */
