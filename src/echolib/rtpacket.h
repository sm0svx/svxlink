/*

	   Definitions for RTP packet manipulation routines

*/

#ifndef RTPACKET_H
#define RTPACKET_H

#include <stdint.h>

int rtp_make_sdes(unsigned char *, const char *, const char *, const char *);
int rtp_make_bye(unsigned char *);
bool parseSDES(char *, unsigned char *, unsigned char);
bool isRTCPByepacket(unsigned char *, int);
bool isRTCPSdespacket(unsigned char *, int);

#endif
