
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>

#include "rtp.h"
#include "rtpacket.h"

#define addText(block, text) {	int sl = strlen(text); *block++ = sl; \
                                memcpy(block, text, sl); block += sl;  }

/*************** RTP_MAKE_SDES *************/

int rtp_make_sdes(unsigned char *p, const char *callsign,
  const char *name, const char *priv)
{
    unsigned char *ap;
    unsigned short ver;
    char tmp[256];
    int len;

    /* Prefix the packet with a null receiver report.  This is
       required by the RTP spec, but is not required in packets
       sent only to the Look Who's Listening server. */

    *p++ = RTP_VERSION << 6;
    *p++ = RTCP_RR;
    *p++ = 0;
    *p++ = 1;
    *p++ = 0;
    *p++ = 0;
    *p++ = 0;
    *p++ = 0;

    /* Set SDES version/misc data. */
    ver = (RTP_VERSION << 14) | RTCP_SDES | (1 << 8);
    p[0] = ver >> 8;
    p[1] = ver;

    /* Set SDES source. */
    p[4] = 0;
    p[5] = 0;
    p[6] = 0;
    p[7] = 0;

    /* At this point ap points to the beginning of the first SDES item. */
    ap = p + 8;

    *ap++ = RTCP_SDES_CNAME;
    addText(ap, "CALLSIGN");

    *ap++ = RTCP_SDES_NAME;
    sprintf(tmp, "%-15s%s", callsign, name);
    addText(ap, tmp);

    *ap++ = RTCP_SDES_EMAIL;
    addText(ap, "CALLSIGN");
    
    *ap++ = RTCP_SDES_PHONE;
    addText(ap, "08:30");

    if (priv)
    {
      *ap++ = RTCP_SDES_PRIV;
      addText(ap, priv);
    }
    
    *ap++ = RTCP_SDES_END;
    *ap++ = 0;

    /* Some data padding for alignment. */
    while ((ap - p) & 3)
    {
        *ap++ = 0;
    }

    /* Length of all items. */
    len = ((ap - p) / 4) - 1;
    p[2] = len >> 8;
    p[3] = len;

    /* Size of the entire packet. */
    return (ap - p) + 8;
}

/************* RTP_MAKE_BYE ***************/

int rtp_make_bye(unsigned char *p)
{
    unsigned char *ap;
    unsigned short ver;
    int len;

    /* Prefix the packet with a null receiver report.  This is
       required by the RTP spec, but is not required in packets
       sent only to the Look Who's Listening server. */

    *p++ = RTP_VERSION << 6;
    *p++ = RTCP_RR;
    *p++ = 0;
    *p++ = 1;
    *p++ = 0;
    *p++ = 0;
    *p++ = 0;
    *p++ = 0;

    /* Set BYE version/misc data. */
    ver = (RTP_VERSION << 14) | RTCP_BYE | (1 << 8);
    p[0] = ver >> 8;
    p[1] = ver;

    /* Set BYE source. */
    p[4] = 0;
    p[5] = 0;
    p[6] = 0;
    p[7] = 0;

    /* At this point ap points to the beginning of the trailing text. */
    ap = p + 8;

    /* Add trailing text. */
    addText(ap, "jan2002");

    /* Some data padding for alignment. */
    while ((ap - p) & 3)
    {
        *ap++ = 0;
    }
    
    /* Length of trailing text. */
    len = ((ap - p) / 4) - 1;
    p[2] = len >> 8;
    p[3] = len;
    
    /* Size of the entire packet. */
    return (ap - p) + 8;
}

/*  PARSESDES  --  Look for an SDES message in a possibly composite
		   RTCP packet. */

/***************************************************/

bool parseSDES(char *r_text, unsigned char *packet, unsigned char r_item)
{
    bool success = false;
    unsigned char *p = packet;

    /* Initialise the result in the request packet to NULL. */
    *r_text = 0;

    /* Walk through the individual items in a possibly composite
       packet until we locate an SDES. This allows us to accept
       packets that comply with the RTP standard that all RTCP packets
       begin with an SR or RR. */

    while ((p[0] >> 6 & 3) == RTP_VERSION || (p[0] >> 6 & 3) == 1)
    {
        int len = (ntohs(*((uint16_t *) (p + 2))) + 1) * 4;
        
	if ((p[1] == RTCP_SDES) && ((p[0] & 0x1F) > 0))
	{
	    unsigned char *cp = p + 8;
	    unsigned char *lp = cp + len;
	    
	    while (cp < lp)
	    {
		unsigned char itype = cp[0];
		unsigned char ilen = cp[1];

		if (itype == RTCP_SDES_END)
		{
		    break;
		}

		/* Search for a match in the request and fill the
		   matching item. */
                if (r_item == itype && *r_text == 0)
                {
                    memcpy(r_text, cp + 2, ilen);
                    r_text[ilen] = 0;
                    success = true;
                    break;
		}
		cp += ilen + 2;
	    }
	    break;
	}
	/* If not of interest to us, skip to next subpacket. */
	p += len;
    }
    return success;
}


/************************************/
/*  ISRTCPBYEPACKET  --  Test if this RTCP packet contains a BYE.  */

bool isRTCPByepacket(unsigned char *p, int len)
{
    unsigned char *end = p + len;
    bool sawbye = false;
                                                   /* Version incorrect ? */
    if ((((p[0] >> 6) & 3) != RTP_VERSION && ((p[0] >> 6) & 3) != 1) ||
        ((p[0] & 0x20) != 0) ||                    /* Padding in first packet ? */
        ((p[1] != RTCP_SR) && (p[1] != RTCP_RR)))
    { /* First item not SR or RR ? */
        return false;
    }

    do {
        if (p[1] == RTCP_BYE)
        {
            sawbye = true;
        }
        /* Advance to next subpacket */
        p += (ntohs(*((uint16_t *) (p + 2))) + 1) * 4;
    } while (p < end && (((p[0] >> 6) & 3) == RTP_VERSION));

    return sawbye;
}

/************************************/
/*  ISRTCPSDESPACKET  --  Test if this RTCP packet contains a SDES.  */

bool isRTCPSdespacket(unsigned char *p, int len)
{
    unsigned char *end = p + len;
    bool sawsdes = 0;
                                                   /* Version incorrect ? */
    if ((((p[0] >> 6) & 3) != RTP_VERSION && ((p[0] >> 6) & 3) != 1) ||
        ((p[0] & 0x20) != 0) ||                    /* Padding in first packet ? */
        ((p[1] != RTCP_SR) && (p[1] != RTCP_RR)))
    { /* First item not SR or RR ? */
        return false;
    }

    do {
        if (p[1] == RTCP_SDES)
        {
            sawsdes = true;
        }
        /* Advance to next subpacket */
        p += (ntohs(*((uint16_t *) (p + 2))) + 1) * 4;
    } while (p < end && (((p[0] >> 6) & 3) == RTP_VERSION));

    return sawsdes;
}

