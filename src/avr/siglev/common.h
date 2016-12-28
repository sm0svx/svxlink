#ifndef COMMON_INCLUDED
#define COMMON_INCLUDED


#define F_CPU 	    14745600UL 	/* 14.7456MHz */

#define ASSIGN_INV_BIT(d, dbit, s, sbit)  \
      	  (d) = ((d) | _BV(dbit)) ^ ((((s) >> (sbit)) & 1) << (dbit))

#define ASSIGN_BIT(d, dbit, s, sbit)  \
      	  (d) = ((d) & ~_BV(dbit)) | ((((s) >> (sbit)) & 1) << (dbit))


#endif /* COMMON_INCLUDED */
