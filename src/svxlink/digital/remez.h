#ifndef REMEZ_INCLUDED
#define REMEZ_INCLUDED


#define BANDPASS       1
#define DIFFERENTIATOR 2
#define HILBERT        3


/********************
 * remez
 *=======
 * Calculates the optimal (in the Chebyshev/minimax sense)
 * FIR filter impulse response given a set of band edges,
 * the desired reponse on those bands, and the weight given to
 * the error in those bands.
 *
 * INPUT:
 * ------
 * int     numtaps     - Number of filter coefficients
 * int     numband     - Number of bands in filter specification
 * double  bands[]     - User-specified band edges [2 * numband]
 * double  des[]       - User-specified band responses [numband]
 * double  weight[]    - User-specified error weights [numband]
 * int     type        - Type of filter
 * int     griddensity - Density of the frequency grid (default 16)
 *
 * OUTPUT:
 * -------
 * double h[]      - Impulse response of final filter [numtaps]
 * returns         - true on success, false on failure to converge
 ********************/
int remez(double h[], int numtaps,
          int numband, const double bands[],
          const double des[], const double weight[],
          int type, int griddensity);


#endif /* REMEZ_INCLUDED */
