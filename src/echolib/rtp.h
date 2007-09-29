/*
* rtp.h  --  RTP header file
* RTP draft: November 1994 version
*
* $Id$
*/

#include <stdint.h>

#define RTP_SEQ_MOD (1<<16)
#define RTP_TS_MOD  (0xffffffff)
/*
* Current type value.
*/
#define RTP_VERSION 3

#define RTP_MAX_SDES 256   /* maximum text length for SDES */

typedef enum {
  RTCP_SR   = 200,
  RTCP_RR   = 201,
  RTCP_SDES = 202,
  RTCP_BYE  = 203,
  RTCP_APP  = 204
} rtcp_type_t;

typedef enum {
  RTCP_SDES_END    =  0,
  RTCP_SDES_CNAME  =  1,
  RTCP_SDES_NAME   =  2,
  RTCP_SDES_EMAIL  =  3,
  RTCP_SDES_PHONE  =  4,
  RTCP_SDES_LOC    =  5,
  RTCP_SDES_TOOL   =  6,
  RTCP_SDES_NOTE   =  7,
  RTCP_SDES_PRIV   =  8, 
  RTCP_SDES_IMG    =  9,
  RTCP_SDES_DOOR   = 10,
  RTCP_SDES_SOURCE = 11
} rtcp_sdes_type_t;

#define u_int8 uint8_t
#define u_int16 uint16_t
#define u_int32 uint32_t

typedef struct {
  unsigned int version:2;  /* protocol version */
  unsigned int p:1;        /* padding flag */
  unsigned int x:1;        /* header extension flag */
  unsigned int cc:4;       /* CSRC count */
  unsigned int m:1;        /* marker bit */
  unsigned int pt:7;       /* payload type */
  u_int16 seq;             /* sequence number */
  u_int32 ts;              /* timestamp */
  u_int32 ssrc;            /* synchronization source */
  u_int32 csrc[1];         /* optional CSRC list */
} __attribute__ ((packed)) rtp_hdr_t;

typedef struct {
  unsigned int version:2;  /* protocol version */
  unsigned int p:1;        /* padding flag */
  unsigned int count:5;    /* varies by payload type */
  unsigned int pt:8;       /* payload type */
  u_int16 length;          /* packet length in words, without this word */
} __attribute__ ((packed)) rtcp_common_t;

/* reception report */
typedef struct {
  u_int32 ssrc;            /* data source being reported */
  unsigned int fraction:8; /* fraction lost since last SR/RR */
  int lost:24;             /* cumulative number of packets lost (signed!) */
  u_int32 last_seq;        /* extended last sequence number received */
  u_int32 jitter;          /* interarrival jitter */
  u_int32 lsr;             /* last SR packet from this source */
  u_int32 dlsr;            /* delay since last SR packet */
} __attribute__ ((packed)) rtcp_rr_t;

typedef struct {
  u_int8 type;             /* type of SDES item (rtcp_sdes_type_t) */
  u_int8 length;           /* length of SDES item (in octets) */
  char data[1];            /* text, not zero-terminated */
} __attribute__ ((packed)) rtcp_sdes_item_t;

/* one RTCP packet */
typedef struct {
  rtcp_common_t common;    /* common header */
  union {
    /* sender report (SR) */
    struct {
      u_int32 ssrc;        /* source this RTCP packet refers to */
      u_int32 ntp_sec;     /* NTP timestamp */
      u_int32 ntp_frac;
      u_int32 rtp_ts;      /* RTP timestamp */
      u_int32 psent;       /* packets sent */
      u_int32 osent;       /* octets sent */ 
      /* variable-length list */
      rtcp_rr_t rr[1];
    } sr;

    /* reception report (RR) */
    struct {
      u_int32 ssrc;        /* source this generating this report */
      /* variable-length list */
      rtcp_rr_t rr[1];
    } rr;

    /* BYE */
    struct {
      u_int32 src[1];      /* list of sources */
      /* can't express trailing text */
    } bye;

    /* source description (SDES) */
    struct rtcp_sdes_t {
      u_int32 src;              /* first SSRC/CSRC */
      rtcp_sdes_item_t item[1]; /* list of SDES items */
    } sdes;
  } r;
} __attribute__ ((packed)) rtcp_t;

