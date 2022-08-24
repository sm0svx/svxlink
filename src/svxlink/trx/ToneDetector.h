/**
@file	 ToneDetector.h
@brief   A tone detector that use the Goertzel algorithm
@author  Tobias Blomberg / SM0SVX
@date	 2003-04-15

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2004-2022  Tobias Blomberg / SM0SVX

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


#ifndef TONE_DETECTOR_INCLUDED
#define TONE_DETECTOR_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <sigc++/sigc++.h>
#include <vector>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncAudioSink.h>
#include <CppStdCompat.h>


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
@brief	A tone detector that use the Goertzel algorithm
@author Tobias Blomberg / SM0SVX
@date   2003-04-15

The Goertzel algorithm is just a recursive way to evaluate the DFT at a
single frequency. For maximum detection reliability, the bandwidth will
be adapted to place the tone frequency near the center of the DFT.
As a side effect, the detection bandwidth is slightly narrowed, which
however is acceptable for the current use cases (CTCSS, 1750Hz, etc..).
*/
class ToneDetector : public sigc::trackable, public Async::AudioSink
{
  public:
    /**
     * @brief Constructor
     * @param tone_hz The frequency in Hz of the tone that should be detected
     * @param width_hz The Bandwidth of the detecto in Hz
     * @param det_delay_ms The detection delay in milliseconds
     *
     * Constructs a new tone detector with the given frequency and
     * bandwidth. Note that if windowing is enabled (default), the
     * bandwidth will increase quite a bit. The detection delay say how
     * many audio blocks, measured in milliseconds, that have to
     * give the same detection result before the detector change state.
     */
    ToneDetector(float tone_hz, float width_hz, int det_delay_ms = 0);

    /**
     * @brief   Destructor
     */
    ~ToneDetector(void);

    /**
     * @brief  Return the detection frequency
     * @return Returns the detection frequency in Hz
     */
    float toneFq(void) const { return tone_fq; }

    /**
     * @brief  Return the estimated detection frequency
     * @return Returns the estimated detection frequency in Hz
     *
     * This function will return the estimated tone frequency for the
     * algorithms that use frequency estimation as a detection method.
     */
    float toneFqEstimate() const { return tone_fq_est; }

    /**
     * @brief   Set the tone detection frequency tolerance in percent
     * @param   freq_tol_percent The +/-% frequency offset to accept
     *
     * Use this function to set the tone detection frequency tolerance in
     * percent. The estimated tone frequency must be within
     * (-fc*tol/100, +fc*tol/100) to be detected.
     */
    void setDetectToneFrequencyTolerancePercent(float freq_tol_percent);

    /**
     * @brief   Set the tone undetection frequency tolerance in percent
     * @param   freq_tol_percent The +/-% frequency offset to accept
     *
     * Use this function to set the tone undetection frequency tolerance in
     * percent. The estimated tone frequency must be within
     * (-fc*tol/100, +fc*tol/100) to still be detected.
     */
    void setUndetectToneFrequencyTolerancePercent(float freq_tol_percent);

    /**
     * @brief   Set the detection overlap in percent
     * @param   overlap_percent The overlap in percent
     *
     * Use this function to set how much, in percent, each processing block
     * should overlap. Overlap is used to get a more fine grained resolution in
     * detection time.
     */
    void setDetectOverlapPercent(float overlap_percent);

    /**
     * @brief   Set the undetection overlap in percent
     * @param   overlap_percent The overlap in percent
     *
     * Use this function to set how much, in percent, each processing block
     * should overlap. Overlap is used to get a more fine grained resolution in
     * undetection time.
     */
    void setUndetectOverlapPercent(float overlap_percent);

    /**
     * @brief   Set the detection overlap in in samples
     * @param   overlap The number of samples to overlap
     *
     * Use this function to set how much, in samples, each processing block
     * should overlap. Overlap is used to get a more fine grained resolution in
     * detection time.
     */
    void setDetectOverlapLength(size_t overlap);

    /**
     * @brief   Set the undetection overlap in in samples
     * @param   overlap The number of samples to overlap
     *
     * Use this function to set how much, in samples, each processing block
     * should overlap. Overlap is used to get a more fine grained resolution in
     * detection time.
     */
    void setUndetectOverlapLength(size_t overlap);

    /**
     * @brief  Set the detection delay
     * @param  delay_ms The number of milliseconds to delay a detection
     *
     * Higher values of detection delay give less chance for false detections
     * but of course a slower detector. The tone detection is done in blocks
     * of samples. Setting this parameter affects how many blocks in a
     * row that must give a positive result before the tone is reported
     * as active. The length of one block depends on the chosen detector
     * bandwidth.
     * Setting a delay of 0 will reset the delay to the default defined by
     * DEFAULT_STABLE_COUNT_THRESH. Setting a negative delay is a noop.
     */ 
    void setDetectDelay(int delay_ms);

    /**
     * @brief  Set the detection delay in processing blocks
     * @param  count The number of blocks to delay detection
     *
     * Higher values of this threshold give less chance for false detections
     * but also a slower detector. The tone detection is done in blocks
     * of samples. Setting this parameter affects how many blocks in a
     * row that must give a positive result before the tone is reported
     * as active. The length of one block depends on the chosen detector
     * bandwidth.
     */
    void setDetectStableCountThresh(int count);

    /**
     * @brief  Get the detection delay
     * @return Returns the detection delay in milliseconds
     *
     * The returned detection delay is not an exact value. The actual delay
     * can be both lower and higher. It will give you an idea of the
     * approximate delay though.
     */
    int detectDelay(void) const;

    /**
     * @brief  Set the undetection delay
     * @param  delay_ms The number of milliseconds to delay an undetection
     *
     * Higher values of undetection delay give less chance for false
     * undetections but of course a slower detector. The tone detection
     * is done in blocks of samples. Setting this parameter affects how many
     * blocks in a row that must give a negative result before the tone is
     * reported as inactive. The length of one block depends on the chosen
     * detector bandwidth.
     */ 
    void setUndetectDelay(int delay_ms);

    /**
     * @brief  Set the undetection delay in processing blocks
     * @param  count The number of blocks to delay undetection
     *
     * Higher values of this threshold give less chance for false undetections
     * but also a slower detector. The tone detection is done in blocks
     * of samples. Setting this parameter affects how many blocks in a
     * row that must give a negative result before the tone is reported
     * as inactive. The length of one block depends on the chosen detector
     * bandwidth.
     */
    void setUndetectStableCountThresh(int count);

    /**
     * @brief  Get the undetection delay
     * @return Returns the undetection delay in milliseconds
     *
     * The returned undetection delay is not an exact value. The actual delay
     * can be both lower and higher. It will give you an idea of the
     * approximate delay though.
     */
    int undetectDelay(void) const;

    /**
     * @brief  Set the detector bandwidth to use when the tone is inactive
     * @param  bw_hz The detector bandwidth in Hz to use
     *
     * This function set the detector bandwidth to use when the tone detector
     * is in inactive state, that is when a tone is not being detected.
     * Narrower bandwidth will give a slower detector. The approximate
     * bandwidth can be calculated using (sample_rate / bw_hz).
     * Note: The actual bandwidth will increase quite a bit if windowing
     * is used.
     */
    void setDetectBw(float bw_hz);

    /**
     * @brief  Set the detector bandwidth to use when the tone is active
     * @param  bw_hz The detector bandwidth in Hz to use
     *
     * This function set the detector bandwidth to use when the tone detector
     * is in active state, that is when a tone is being detected.
     * Narrower bandwidth will give a slower detector. The approximate
     * bandwidth can be calculated using (sample_rate / bw_hz).
     * Note: The actual bandwidth will increase quite a bit if windowing
     * is used.
     */
    void setUndetectBw(float bw_hz);

    /**
     * @brief  Set the relative amplitude threshold to use when inactive
     * @param  thresh The threshold in dB
     *
     * This function set the value to use for the peak threshold when
     * the tone detector is in its inactive state, that is when a tone
     * is not being detected.
     * The peak threshold is the value in dB that the tone must be over
     * its adjacent bins to be considered as present.
     */
    void setDetectPeakThresh(float thresh);

    /**
     * @brief  Set the relative amplitude threshold to use when active
     * @param  thresh The threshold in dB
     *
     * This function set the value to use for the peak threshold when
     * the tone detector is in its active state, that is when a tone
     * is being detected.
     * The peak threshold is the value in dB that the tone must be over
     * its adjacent bins to be considered as present.
     */
    void setUndetectPeakThresh(float thresh);

    /**
     * @brief  Set the relative amplitude threshold
     * @param  thresh The threshold in dB
     *
     * This is a convenience function to set the peak threshold for both
     * the active and inactive state in one function call.
     */
    void setPeakThresh(float thresh);

    /**
     * @brief  Set the peak to total passband power threshold when inactive
     * @param  thresh The threshold (0.0 - 1.0)
     *
     * This function is used to set the peak to total passband power
     * threshold when the tone detector is in its inactive state, that is
     * when a tone is not being detected.
     * This will compare the narrow band Goertzel tone detector power with
     * the total power for the whole passband. A value of 1.0 indicate that
     * all power in the passband must be in the tone. A value of 0.0 indicate
     * that none of the passband power is in the tone.
     * You should probably disable the peak threshold by calling
     * setDetectPeakThresh(0.0f), since that test is similar to this one.
     */
    void setDetectPeakToTotPwrThresh(float thresh);

    /**
     * @brief  Set the peak to total passband power threshold when active
     * @param  thresh The threshold (0.0 - 1.0)
     *
     * This function is used to set the peak to total passband power
     * threshold when the tone detector is in its active state, that is
     * when a tone is being detected.
     * This will compare the narrow band Goertzel tone detector power with
     * the total power for the whole passband. A value of 1.0 indicate that
     * all power in the passband is in the tone. A value of 0.0 indicate
     * that none of the passband power is in the tone.
     * You should probably disable the peak threshold by calling
     * setUndetectPeakThresh(0.0f), since that test is similar to this one.
     */
    void setUndetectPeakToTotPwrThresh(float thresh);

    /**
     * @brief  Set the peak to noise floor SNR threshold when inactive
     * @param  thresh_db 	The threshold in dB
     * @param  passband_bw_hz	Passband bandwidth in Hz
     *
     * This function is used to set the signal to noise ratio
     * threshold when the tone detector is in its inactive state, that is
     * when a tone is not being detected.
     * This will compare the narrow band Goertzel tone detector power to
     * an estimated noise floor. The noise floor estimation is deduced from
     * the total passband power where the tone has been removed. This will
     * be a mean value of the noise floor for the whole passband. If the
     * amplitude response for the passband is not flat, the noise floor will
     * have an offset and so will the tone. This will mean that the SNR will
     * also have an offset. So, for example, if the part of the passband where
     * the tone is expected to appear is attenuated by 6dB in comparision to
     * the mean passband gain, the threshold need to also be lowered by 6dB.
     */
    void setDetectSnrThresh(float thresh_db, float passband_bw_hz);

    /**
     * @brief  Set the peak to noise floor SNR threshold when active
     * @param  thresh_db	The threshold in dB
     * @param  passband_bw_hz	Passband bandwidth in Hz
     *
     * This function is used to set the signal to noise ratio
     * threshold when the tone detector is in its active state, that is
     * when a tone is being detected.
     * This will compare the narrow band Goertzel tone detector power to
     * an estimated noise floor. The noise floor estimation is deduced from
     * the total passband power where the tone has been removed. This will
     * be a mean value of the noise floor for the whole passband. If the
     * amplitude response for the passband is not flat, the noise floor will
     * have an offset and so will the tone. This will mean that the SNR will
     * also have an offset. So, for example, if the part of the passband where
     * the tone is expected to appear is attenuated by 6dB in comparision to
     * the mean passband gain, the threshold need to also be lowered by 6dB.
     */
    void setUndetectSnrThresh(float thresh_db, float passband_bw_hz);

    /**
     * @brief  Set the phase detector bandwidth when inactive
     * @param  bw_hz The phase detector bandwidth in Hz
     * @param  stddev_hz The phase detector standard deviation in Hz
     *
     * When the tone detector runs through the samples, the phase value
     * will rotate with a speed that is linear in relation to the difference
     * in frequency between the detector tone frequency and an incoming tone.
     * This can be exploited to increase the speed and acuracy of the
     * tone detector.
     * This function will set the phase detector bandwidth to use while a
     * tone is not being detected. The standard deviation is a measure for
     * how noisy the signal is. Lower values for the standard deviation
     * will require a less noisy signal to trigger the detector.
     * This functionality has only been tested for low frequencies (<300Hz)
     * and for narrow bandwidths (<5Hz).
     * The windowing function interferes with this functionality so it will
     * be automatically turned off when the phase detector is being set up.
     */
    void setDetectPhaseBwThresh(float bw_hz, float stddev_hz);

    /**
     * @brief  Set the phase detector bandwidth when active
     * @param  bw_hz The phase detector bandwidth in Hz
     * @param  stddev_hz The phase detector standard deviation in Hz
     *
     * When the tone detector runs through the samples, the phase value
     * will rotate with a speed that is linear in relation to the difference
     * in frequency between the detector tone frequency and an incoming tone.
     * This can be exploited to increase the speed and acuracy of the
     * tone detector.
     * This function will set the phase detector bandwidth to use when a
     * tone is being detected. The standard deviation is a measure for
     * how noisy the signal is. Lower values for the standard deviation
     * will require a less noisy signal to trigger the detector.
     * This functionality has only been tested for low frequencies (<300Hz)
     * and for narrow bandwidths (<5Hz).
     * The windowing function interferes with this functionality so it will
     * be automatically turned off when the phase detector is being set up.
     */
    void setUndetectPhaseBwThresh(float bw_hz, float stddev_hz);

    /**
     * @brief  Choose if a Hamming window should be applied when inactive
     * @param  enable Set to \em true to enable or \em false to disable
     *
     * A windowing function will help decrease the artificial noise
     * introduced (spectral leakage) when processing a continuous signal
     * in blocks. However, it will also increase the bandwidth of the
     * detector by quite a bit.
     * This function will choose if a Hamming window should be applied when
     * the detector is in its inactive state, that is when a tone is not
     * being detected.
     */
    void setDetectUseWindowing(bool enable);

    /**
     * @brief  Choose if a Hamming window should be applied when active
     * @param  enable Set to \em true to enable or \em false to disable
     *
     * A windowing function will help decrease the artificial noise
     * introduced (spectral leakage) when processing a continuous signal
     * in blocks. However, it will also increase the bandwidth of the
     * detector by quite a bit.
     * This function will choose if a Hamming window should be applied when
     * the detector is in its active state, that is when a tone is
     * being detected.
     */
    void setUndetectUseWindowing(bool enable);
    
    /**
     * @brief  Check if the tone detector is activated or not
     * @return Return \em true if the detector is active or \em false if not
     */
    bool isActivated(void) const { return is_activated; }

    /**
     * @brief   Get the latest calculated SNR
     * @return  Returns the SNR value that was calculated latest
     */
    float lastSnr(void) const { return last_snr; }

    /**
     * @brief  Reset the tone detector
     */
    void reset(void);
    
    /**
     * @brief Write samples into the tone detector
     * @param buf The buffer containing the samples
     * @param len The number of samples in the buffer
     */
    virtual int writeSamples(const float *buf, int len);

    /**
     * @brief   Tell the sink to flush the previously written samples
     *
     * This function is used to tell the sink to flush previously written
     * samples. When done flushing, the sink should call the
     * sourceAllSamplesFlushed function.
     */
    virtual void flushSamples(void) { sourceAllSamplesFlushed(); }

    /**
     * @brief  A signal that is emitted when the tone detector changes state
     * @param  is_active \em true when active and \em false if not
     */
    sigc::signal<void, bool> activated;

    /**
     * @brief  A signal that is emitted when a tone is first detected
     * @param  fq The frequency of the detected tone
     *
     * This signal will be emitted when a tone has been detected. The reported
     * frequency is not a measured frequency but just the frequency given to
     * the detector during construction.
     */
    sigc::signal<void, float> detected;

    /**
     * @brief  A signal that is emitted when the tone SNR has been recalculated
     * @param  snr The current SNR
     *
     * This signal will be emitted as soon as a new SNR value for the tone
     * has been calculated. The signal will only be emitted if the functions
     * setDetectSnrThresh and/or setUndetectSnrThresh have been used to setup
     * a threshold.
     */
    sigc::signal<void, float> snrUpdated;
    
  private:
    struct DetectorParams;

    static CONSTEXPR bool   DEFAULT_USE_WINDOWING           = true;
    static CONSTEXPR float  DEFAULT_TONE_ENERGY_THRESH      = 0.1f;
    static CONSTEXPR float  DEFAULT_PEAK_THRESH             = 10.0;
    static CONSTEXPR float  DEFAULT_PHASE_MEAN_THRESH       = 0.0f;
    static CONSTEXPR float  DEFAULT_PHASE_VAR_THRESH        = 0.0f;
    static CONSTEXPR int    DEFAULT_STABLE_COUNT_THRESH     = 3;
    static CONSTEXPR float  DEFAULT_OVERLAP_PERCENT         = 0.0f;
    static CONSTEXPR float  DEFAULT_FREQ_TOL_HZ             = 0.0f;
    static CONSTEXPR float  DEFAULT_PEAK_TO_TOT_PWR_THRESH  = 0.0f;
    static CONSTEXPR float  DEFAULT_SNR_THRESH              = 0.0f;

    const float         tone_fq;
    size_t              buf_pos;
    bool                is_activated;
    bool                last_active;
    int                 stable_count;
    int		        phase_check_left;
    float	        prev_phase;
    std::vector<float>  phase_diffs;
    DetectorParams	*det_par;
    DetectorParams	*undet_par;
    DetectorParams	*par;
    double		passband_energy;
    float               last_snr;
    float               tone_fq_est;

    std::vector<float>::const_iterator win;

    void phaseCheckReset(void);
    void phaseCheck(void);
    void postProcess(void);
    void setActivated(bool activated);
    void setToneFrequencyTolerancePercent(DetectorParams* par,
                                          float freq_tol_percent);
    void setDelay(DetectorParams* par, int delay_ms);
    int delay(DetectorParams* par) const;
    void setOverlapPercent(DetectorParams* par, float overlap_percent);
    void setOverlapLength(ToneDetector::DetectorParams* par, size_t overlap);
    void setBw(DetectorParams* par, float bw_hz);

};  /* class ToneDetector */


//} /* namespace */

#endif /* TONE_DETECTOR_INCLUDED */



/*
 * This file has not been truncated
 */

