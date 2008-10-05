/*

	   Definitions for RTP packet manipulation routines

*/

#ifndef RTPACKET_H
#define RTPACKET_H

#include <stdint.h>

int rtp_make_sdes(unsigned char *, uint32_t, const char *, const char *);
int rtp_make_bye(unsigned char *, uint32_t, const char *);
bool parseSDES(char *, unsigned char *, unsigned char);
bool isRTCPByepacket(unsigned char *, int);
bool isRTCPSdespacket(unsigned char *, int);

#endif
