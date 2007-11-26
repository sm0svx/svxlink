#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>

#include "rtp.h"
#include "rtpacket.h"

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif


/*************** RTP_MAKE_SDES *************/

int rtp_make_sdes(pkt, ssrc_i, strict, callsign, name)
  char **pkt;
  uint32_t ssrc_i;
  int strict;
  const char *callsign;
  const char *name;
{
    unsigned char zp[1500];
    unsigned char *p = zp;
    rtcp_t *rp;
    unsigned char *ap;
    /* char *sp, *ep; */
    char line[180];
    int l, hl; /* , i; */
    /*
    struct passwd *pw;
    char s[256], ev[1024];
    */

    ssrc_i = htonl(ssrc_i);

    void addSDES(unsigned char item, char *text){
        *ap++ = item;
        *ap++ = l = strlen(text);
        bcopy(text, ap, l);
        ap += l;
    }


    hl = 0;
    if (strict) {
	*p++ = RTP_VERSION << 6;
	*p++ = RTCP_RR;
	*p++ = 0;
	*p++ = 1;
	memcpy(p, &ssrc_i, 4);
	p += 4;
	hl = 8;
    }

    rp = (rtcp_t *) p;

    // #define RationalWorld
#ifdef RationalWorld
    rp->common.version = RTP_VERSION;
    rp->common.p = 0;
    rp->common.count = 1;
    rp->common.pt = RTCP_SDES;
#else
    *((uint16_t *) p) = htons((RTP_VERSION << 14) | RTCP_SDES | (1 << 8));
#endif	
    rp->r.sdes.src = ssrc_i;

    ap = (unsigned char *) rp->r.sdes.item;

    /*****
   At this point ap points to the beginning of the first SDES item
    *******/

    strcpy(line,"CALLSIGN");
    addSDES(RTCP_SDES_CNAME, line);

    sprintf(line,"%-15s%s", callsign, name);
    addSDES(RTCP_SDES_NAME, line);

    strcpy(line,"CALLSIGN");
    addSDES(RTCP_SDES_EMAIL, line);
 
    strcpy(line,"08:30");
    addSDES(RTCP_SDES_PHONE, line);
    
    *ap++ = RTCP_SDES_END;
    *ap++ = 0;

    l = ap - p;

    rp->common.length = htons(((l + 3) / 4) - 1);
    l = hl + ((ntohs(rp->common.length) + 1) * 4);

    /* Okay, if the total length of this packet is not an odd
       multiple of 4 bytes, we're going to put a pad at the
       end of it.  Why?  Because we may encrypt the packet
       later and that requires it be a multiple of 8 bytes,
       and we don't want the encryption code to have to
       know all about our weird composite packet structure.
       Oh yes, there's no reason to do this if strict isn't
       set, since we never encrypt packets sent to a Look
       Who's Listening server.

       Why an odd multiple of 4 bytes, I head you ask?
       Because when we encrypt an RTCP packet, we're required
       to prefix it with four random bytes to deter a known
       plaintext attack, and since the total buffer we
       encrypt, including the random bytes, has to be a
       multiple of 8 bytes, the message needs to be an odd
       multiple of 4. */

    if (strict) {
	int pl = (l & 4) ? l : l + 4;

	if (pl > l) {
	    int pad = pl - l;

	    bzero(zp + l, pad);       /* Clear pad area to zero */
	    zp[pl - 1] = pad;	      /* Put pad byte count at end of packet */
            p[0] |= 0x20;             /* Set the "P" bit in the header of the
					 SDES (last in message) packet */
                                      /* If we've added an additional word to
					 the packet, adjust the length in the
					 SDES message, which must include the
					 pad */
	    rp->common.length = htons(ntohs(rp->common.length) + ((pad) / 4));
	    l = pl;		      /* Include pad in length of packet */
	}
    }

    *pkt = (char *) malloc(l);
    if (*pkt != NULL) {
	bcopy(zp, *pkt, l);
	return l;
    }
    return 0;
}

/************* RTP_MAKE_BYE ***************/

int rtp_make_bye(p, ssrc_i, raison, strict)
  unsigned char *p;
  uint32_t ssrc_i;
  const char *raison;
  int strict;
{
    rtcp_t *rp;
    unsigned char *ap, *zp;
    int l, hl;

    ssrc_i = htonl(ssrc_i);

    /* If requested, prefix the packet with a null receiver
       report.  This is required by the RTP spec, but is not
       required in packets sent only to the Look Who's Listening
       server. */

    zp = p;
    hl = 0;
    if (strict) {
        *p++ = RTP_VERSION << 6;
        *p++ = RTCP_RR;
        *p++ = 0;
        *p++ = 1;
	memcpy(p, &ssrc_i, 4);
	p += 4;
        hl = 8;
    }

    rp = (rtcp_t *) p;
#ifdef RationalWorld
    rp->common.version = RTP_VERSION;
    rp->common.p = 0;
    rp->common.count = 1;
    rp->common.pt = RTCP_BYE;
#else
    *((uint16_t *) p) = htons((RTP_VERSION << 14) | RTCP_BYE | (1 << 8));
#endif  
    rp->r.bye.src[0] = ssrc_i;

    ap = (unsigned char *) rp->r.sdes.item;

    l = 0;
    if (raison != NULL) {
        l = strlen(raison);
        if (l > 0) {
            *ap++ = l;
            bcopy(raison, ap, l);
            ap += l;
        }
    }

    while ((ap - p) & 3) {
        *ap++ = 0;
    }
    l = ap - p;

    rp->common.length = htons((l / 4) - 1);

    l = hl + ((ntohs(rp->common.length) + 1) * 4);

    /* If strict, pad the composite packet to an odd multiple of 4
       bytes so that if we decide to encrypt it we don't have to worry
       about padding at that point. */

    if (strict) {
        int pl = (l & 4) ? l : l + 4;

        if (pl > l) {
            int pad = pl - l;

            bzero(zp + l, pad);       /* Clear pad area to zero */
            zp[pl - 1] = pad;         /* Put pad byte count at end of packet */
            p[0] |= 0x20;             /* Set the "P" bit in the header of the
                                         SDES (last in message) packet */
                                      /* If we've added an additional word to
                                         the packet, adjust the length in the
                                         SDES message, which must include the
                                         pad */
            rp->common.length = htons(ntohs(rp->common.length) + ((pad) / 4));
            l = pl;                   /* Include pad in length of packet */
        }
    }

    return l;
}

/*  PARSESDES  --  Look for an SDES message in a possibly composite
		   RTCP packet and extract pointers to selected items
                   into the caller's structure.  */

/***************************************************/

int parseSDES(packet, r)
  unsigned char *packet;
  struct rtcp_sdes_request *r;
{
    int i, success = FALSE;
    unsigned char *p = packet;

    /* Initialise all the results in the request packet to NULL. */

    for (i = 0; i < r->nitems; i++) {
	r->item[i].r_text = NULL;
    }

    /* Walk through the individual items in a possibly composite
       packet until we locate an SDES. This allows us to accept
       packets that comply with the RTP standard that all RTCP packets
       begin with an SR or RR. */

    while ((p[0] >> 6 & 3) == RTP_VERSION || (p[0] >> 6 & 3) == 1) {
	if ((p[1] == RTCP_SDES) && ((p[0] & 0x1F) > 0)) {
	    unsigned char *cp = p + 8,
			  *lp = cp + (ntohs(*((uint16_t *) (p + 2))) + 1) * 4;
	    bcopy(p + 4, r->ssrc, 4);
	    while (cp < lp) {
		unsigned char itype = *cp;

		if (itype == RTCP_SDES_END) {
		    break;
		}

		/* Search for a match in the request and fill the
		   first unused matching item.	We do it this way to
		   permit retrieval of multiple PRIV items in the same
		   packet. */

		for (i = 0; i < r->nitems; i++) {
		    if (r->item[i].r_item == itype &&
			r->item[i].r_text == NULL) {
                        r->item[i].r_text = (char *) cp; 
			success = TRUE;
			break;
		    }
		}
		cp += cp[1] + 2;
	    }
	    break;
	}
	/* If not of interest to us, skip to next subpacket. */
	p += (ntohs(*((uint16_t *) (p + 2))) + 1) * 4;
    }
    return success;
}

/*************************************/
/*  COPYSDESITEM  --  Copy an SDES item to a zero-terminated user
                      string.  */

void copySDESitem(s, d)
  char *s, *d;
{
    int len = s[1] & 0xFF;

    bcopy(s + 2, d, len);
    d[len] = 0;
}

/************************************/
/*  ISRTCPBYEPACKET  --  Test if this RTCP packet contains a BYE.  */

int isRTCPByepacket(p, len)
  unsigned char *p;
  int len;
{
    unsigned char *end;
    int sawbye = FALSE;
                                                   /* Version incorrect ? */
    if ((((p[0] >> 6) & 3) != RTP_VERSION && ((p[0] >> 6) & 3) != 1) ||
        ((p[0] & 0x20) != 0) ||                    /* Padding in first packet ? */
        ((p[1] != RTCP_SR) && (p[1] != RTCP_RR))) { /* First item not SR or RR ? */
        return FALSE;
    }
    end = p + len;

    do {
        if (p[1] == RTCP_BYE) {
            sawbye = TRUE;
        }
        /* Advance to next subpacket */
        p += (ntohs(*((uint16_t *) (p + 2))) + 1) * 4;
    } while (p < end && (((p[0] >> 6) & 3) == RTP_VERSION));

    return (sawbye);
}

/************************************/
/*  ISRTCPSDESPACKET  --  Test if this RTCP packet contains a BYE.  */

int isRTCPSdespacket(p, len)
  unsigned char *p;
  int len;
{
    unsigned char *end;
    int sawsdes = FALSE;
                                                   /* Version incorrect ? */
    if ((((p[0] >> 6) & 3) != RTP_VERSION && ((p[0] >> 6) & 3) != 1) ||
        ((p[0] & 0x20) != 0) ||                    /* Padding in first packet ? */
        ((p[1] != RTCP_SR) && (p[1] != RTCP_RR))) { /* First item not SR or RR ? */
        return FALSE;
    }
    end = p + len;

    do {
        if (p[1] == RTCP_SDES) {
            sawsdes = TRUE;
        }
        /* Advance to next subpacket */
        p += (ntohs(*((uint16_t *) (p + 2))) + 1) * 4;
    } while (p < end && (((p[0] >> 6) & 3) == RTP_VERSION));

    return (sawsdes);
}

