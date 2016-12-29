#ifndef MULTIRATE_FILTER_COEFF_INCLUDED
#define MULTIRATE_FILTER_COEFF_INCLUDED

/**********************************************************************
 * The filters in this file have been designed using the filter
 * designer applet at:
 *
 *   http://www.dsptutor.freeuk.com/remez/RemezFIRFilterDesign.html
 **********************************************************************/

/*
First stage 48kHz <-> 16kHz (3.5kHz cut-off)
This is an intermediate filter meant to be used to downsample to 8kHz.

Parks-McClellan FIR Filter Design

Filter type: Low pass
Passband: 0 - 0.07291666666666666667 (0 - 3500Hz)
Order: 29
Passband ripple: 0.1 dB
Transition band: 0.09375 (4500Hz)
Stopband attenuation: 60.0 dB
*/
static const int coeff_48_16_int_taps = 30;
static const float coeff_48_16_int[coeff_48_16_int_taps] =
{
  -0.001104533022845565,
  1.4483111628894497E-4,
  0.0030143616079341333,
  0.007290576776838937,
  0.010111003515779919,
  0.007406824406566465,
  -0.0033299650331323396,
  -0.019837606041858764,
  -0.03369491630668587,
  -0.03261321520115128,
 -0.006227597046237875,
  0.0472474773894006,
  0.11741132225100549,
  0.18394793387595304,
  0.22449383849677723,
  0.22449383849677723,
  0.18394793387595304,
  0.11741132225100549,
  0.0472474773894006,
  -0.006227597046237875,
  -0.03261321520115128,
  -0.03369491630668587,
  -0.019837606041858764,
  -0.0033299650331323396,
  0.007406824406566465,
  0.010111003515779919,
  0.007290576776838937,
  0.0030143616079341333,
  1.4483111628894497E-4,
  -0.001104533022845565
};

/*
48kHz <-> 16kHz (5.5kHz cut-off)

Parks-McClellan FIR Filter Design

Filter type: Low pass
Passband: 0 - 0.1145833333333333333 (0 - 5500Hz)
Order: 49
Passband ripple: 0.1 dB
Transition band: 0.05208333333333333333 (2500Hz)
Stopband attenuation: 60.0 dB
*/
static const int coeff_48_16_taps = 50;
static const float coeff_48_16[coeff_48_16_taps] =
{
  -0.0006552324784575,
  -0.0023665474931056,
  -0.0046009521986267,
  -0.0065673940075750,
  -0.0063452223170932,
  -0.0030442928485507,
  0.0027216740916904,
  0.0079365191173948,
  0.0088820372171036,
  0.0034577679862077,
  -0.0063356171066514,
  -0.0145569576678951,
  -0.0143873806232840,
  -0.0031353455170217,
  0.0143500967202013,
  0.0267723137455069,
  0.0227432656734411,
  -0.0007785303731755,
  -0.0333072891420923,
  -0.0533991698157678,
  -0.0390764894652067,
  0.0189267202445683,
  0.1088868590088443,
  0.2005613197280159,
  0.2583048205906900,
  0.2583048205906900,
  0.2005613197280159,
  0.1088868590088443,
  0.0189267202445683,
  -0.0390764894652067,
  -0.0533991698157678,
  -0.0333072891420923,
  -0.0007785303731755,
  0.0227432656734411,
  0.0267723137455069,
  0.0143500967202013,
  -0.0031353455170217,
  -0.0143873806232840,
  -0.0145569576678951,
  -0.0063356171066514,
  0.0034577679862077,
  0.0088820372171036,
  0.0079365191173948,
  0.0027216740916904,
  -0.0030442928485507,
  -0.0063452223170932,
  -0.0065673940075750,
  -0.0046009521986267,
  -0.0023665474931056,
  -0.0006552324784575
};

/*
48kHz <-> 16kHz (6.5kHz cut-off)

Parks-McClellan FIR Filter Design

Filter type: Low pass
Passband: 0 - 0.135416666667 (0 - 6500Hz)
Order: 53
Passband ripple: 0.1 dB
Transition band: 0.052083332 (2500Hz)
Stopband attenuation: 60.0 dB

The cut-off frequency is chosen so that tones used in the SigLevDetTone class
(5.5-6.4kHz) are let through.

The transition band (6.5 - 9kHz) for this filter is deliberately chosen to be
a bit too wide for downsampling to 16kHz. The (attenuated) frequencies from
8-9kHz will be folded down between 7-8kHz but that does not matter since that
frequency range is not used anyway.
What is gained by using a wider transition band is that the filter will have
a lower order which reduce required CPU power and filter delay.
*/
static const int coeff_48_16_wide_taps = 54;
static const float coeff_48_16_wide[coeff_48_16_wide_taps] =
{
  5.11059239270262E-4,
  -8.255590813253409E-4,
  -0.0022883650051252883,
  -0.00291284164121095,
  -0.0012268298491091916,
  0.0022762075309263855,
  0.004665122182146708,
  0.0028373838432406684,
  -0.0029213363716820875,
  -0.007788031828919018,
  -0.006016833804341717,
  0.002968009107977126,
  0.01198761593254768,
  0.011232706838970668,
  -0.0019206055143741107,
  -0.017561483250559024,
  -0.019661897398973553,
  -0.0011813015957021255,
  0.025346590995928835,
  0.034210485687661864,
  0.008664040822720114,
  -0.03840386432673845,
  -0.0655288086799168,
  -0.030167800561122577,
  0.07566615695450109,
  0.21042482376878066,
  0.3043049697785759,
  0.3043049697785759,
  0.21042482376878066,
  0.07566615695450109,
  -0.030167800561122577,
  -0.0655288086799168,
  -0.03840386432673845,
  0.008664040822720114,
  0.034210485687661864,
  0.025346590995928835,
  -0.0011813015957021255,
  -0.019661897398973553,
  -0.017561483250559024,
  -0.0019206055143741107,
  0.011232706838970668,
  0.01198761593254768,
  0.002968009107977126,
  -0.006016833804341717,
  -0.007788031828919018,
  -0.0029213363716820875,
  0.0028373838432406684,
  0.004665122182146708,
  0.0022762075309263855,
  -0.0012268298491091916,
  -0.00291284164121095,
  -0.0022883650051252883,
  -8.255590813253409E-4,
  5.11059239270262E-4
};

/*
8kHz <-> 16kHz

Parks-McClellan FIR Filter Design

Filter type: Low pass
Passband: 0 - 0.21875 (0 - 3500Hz)
Order: 89
Passband ripple: 0.1 dB
Transition band: 0.03125 (500Hz)
Stopband attenuation: 62.0 dB
*/
static const int coeff_16_8_taps = 90;
static const float coeff_16_8[coeff_16_8_taps] =
{
  4.4954770039301524E-4,
  -8.268172996066966E-4,
  -0.002123078315145856,
  -0.0015479438021244402,
  7.273225897575334E-4,
  0.0013974534015721682,
  -7.334976988828609E-4,
  -0.0019468497129111343,
  4.1355600739715313E-4,
  0.002536269673526767,
  1.5022005765340837E-4,
  -0.003101672879509627,
  -9.95458834752388E-4,
  0.00354467345212626,
  0.0021278523715996304,
  -0.0037661500010028543,
  -0.00353539274926452,
  0.0036538076631845626,
  0.005173997894832533,
  -0.003092155201519595,
  -0.006964869006639621,
  0.001972228534636602,
  0.008799395727660558,
  -1.908879053321082E-4,
  -0.01053574038718076,
  -0.0023470042371114453,
  0.011994344679012392,
  0.005724529332766167,
  -0.012958939230749365,
  -0.010021252057195512,
  0.013170597031930194,
  0.015338845914920506,
  -0.012300860896401845,
  -0.021850249720503187,
  0.009887401534293974,
  0.029911674274011077,
  -0.0051694230705885726,
  -0.04035692286061595,
  -0.0034027067537959477,
  0.05542257393205645,
  0.01998932901259646,
  -0.08281607098012608,
  -0.0619525333134873,
  0.17225790685629527,
  0.42471952920395545,
  0.42471952920395545,
  0.17225790685629527,
  -0.0619525333134873,
  -0.08281607098012608,
  0.01998932901259646,
  0.05542257393205645,
  -0.0034027067537959477,
  -0.04035692286061595,
  -0.0051694230705885726,
  0.029911674274011077,
  0.009887401534293974,
  -0.021850249720503187,
  -0.012300860896401845,
  0.015338845914920506,
  0.013170597031930194,
  -0.010021252057195512,
  -0.012958939230749365,
  0.005724529332766167,
  0.011994344679012392,
  -0.0023470042371114453,
  -0.01053574038718076,
  -1.908879053321082E-4,
  0.008799395727660558,
  0.001972228534636602,
  -0.006964869006639621,
  -0.003092155201519595,
  0.005173997894832533,
  0.0036538076631845626,
  -0.00353539274926452,
  -0.0037661500010028543,
  0.0021278523715996304,
  0.00354467345212626,
  -9.95458834752388E-4,
  -0.003101672879509627,
  1.5022005765340837E-4,
  0.002536269673526767,
  4.1355600739715313E-4,
  -0.0019468497129111343,
  -7.334976988828609E-4,
  0.0013974534015721682,
  7.273225897575334E-4,
  -0.0015479438021244402,
  -0.002123078315145856,
  -8.268172996066966E-4,
  4.4954770039301524E-4
};

#endif /* MULTIRATE_FILTER_COEFF_INCLUDED */
