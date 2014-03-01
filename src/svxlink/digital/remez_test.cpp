#include <stdio.h>
#include <stdlib.h>

#include "remez.h"


int main(void)
{
  const int numtaps = 16;
  const int numband = 2;
  const double bands[] = {0.0, 0.15, 0.2, 0.5};
  const double des[] = {1.0, 1.0, 0.0, 0.0};
  const double weight[] = {1.0, 1.0};
  const int type = BANDPASS;
  const int griddensity = 16;
  double h[numtaps];
  static const double valid_result[numtaps] = {
    0.0415131831103279,
    0.0581639884202646,
    -0.0281579212691008,
    -0.0535575358002337,
    -0.0617245915143180,
    0.0507753178978075,
    0.2079018331396460,
    0.3327160895375440,
    0.3327160895375440,
    0.2079018331396460,
    0.0507753178978075,
    -0.0617245915143180,
    -0.0535575358002337,
    -0.0281579212691008,
    0.0581639884202646,
    0.0415131831103279
  };
  const double maxerr = 1.0e-14;

  int ret = remez(h, numtaps, numband, bands, des, weight, type, griddensity);
  printf("ret=%d\n", ret);

  for (int i=0; i<numtaps; ++i)
  {
    printf("%f (%s)\n", h[i],
           abs(h[i] - valid_result[i]) < maxerr ? "OK" : "NOT OK");
  }

  return 0;
}

