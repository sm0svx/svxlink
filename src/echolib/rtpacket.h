/*

	   Definitions for RTP packet manipulation routines

*/

#include <stdint.h>


struct rtcp_sdes_request_item {
    unsigned char r_item;
    char *r_text;
} __attribute__ ((packed));

struct rtcp_sdes_request {
    int nitems; 		      /* Number of items requested */
    unsigned char ssrc[4];	      /* Source identifier */
    struct rtcp_sdes_request_item item[10]; /* Request items */
} __attribute__ ((packed));

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif


extern int rtp_make_sdes(char **, uint32_t, int, const char *,
    const char *);
extern int rtp_make_bye(unsigned char *, uint32_t, const char *, int);
extern int parseSDES(unsigned char *, struct rtcp_sdes_request *);
extern void copySDESitem(char *, char *);
extern int isRTCPByepacket(unsigned char *, int);
extern int isRTCPSdespacket(unsigned char *, int);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

