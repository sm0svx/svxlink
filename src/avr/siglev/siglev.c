/*****************************************************************************
 *
 * This is a signal level transmitter for use with remote receivers linked
 * in via RF to the SvxLink core. It use ten tones to indicate one of ten
 * signal levels. The signal level is measured using one of the ADC:s in
 * the AVR.
 *
 * The tones are mapped so that the lowest tone (5500Hz) represent the
 * highest signal strength and the highest tone (6400Hz) represent the
 * lowest signal strength. There is also a tone at 6600Hz which indicate
 * no signal but that is not detected by SvxLink. It is used instead of
 * turning the tone off completely because that cause a click in the audio.
 *
 * The program is written for the ATmega8 processor but should be easily
 * ported to other AVR processors.
 *
 ****************************************************************************/

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <inttypes.h>
#include "common.h"
#include <util/delay.h>
#include <string.h>
#include <stdlib.h>


#define PTT_HANGTIME	2	/* 2 seconds PTT hangtime */
#define SQL_TIMEOUT	10	/* 10 minutes squelch timeout */
#define RSSI_ADC_CH     0       /* ADC channel for RSSI measurements */
#define SQL_LVL_ADC_CH  1       /* ADC channel for squelch level */
#define WINDOW_SIZE     2       /* Size of the RSSI schmitt trigger window */


struct siglev_tone_map_item
{
  unsigned char  siglev;
  unsigned short fq;
};

static struct siglev_tone_map_item siglev_tone_map[11];
static uint16_t ptt_hang = 0;
static int32_t sql_timeout = -1;
static uint8_t adc_ch = 0;
static uint16_t rssi_sum = 0;
static uint16_t rssi_cnt = 0;
static uint16_t cur_fq = 0;
static uint16_t icr1 = 0;
static uint8_t  rssi_lo = 0;
static uint8_t  rssi_hi = WINDOW_SIZE;
static uint8_t  sql_thresh = 255;


static inline void set_sin_fq(unsigned short fq);


int main(void)
{
  siglev_tone_map[0].siglev = 0;
  siglev_tone_map[0].fq = 6600;
  siglev_tone_map[1].siglev = sql_thresh;
  siglev_tone_map[1].fq = 6400;
  siglev_tone_map[2].siglev = 30;
  siglev_tone_map[2].fq = 6300;
  siglev_tone_map[3].siglev = 36;
  siglev_tone_map[3].fq = 6200;
  siglev_tone_map[4].siglev = 44;
  siglev_tone_map[4].fq = 6100;
  siglev_tone_map[5].siglev = 49;
  siglev_tone_map[5].fq = 6000;
  siglev_tone_map[6].siglev = 53;
  siglev_tone_map[6].fq = 5900;
  siglev_tone_map[7].siglev = 65;
  siglev_tone_map[7].fq = 5800;
  siglev_tone_map[8].siglev = 75;
  siglev_tone_map[8].fq = 5700;
  siglev_tone_map[9].siglev = 98;
  siglev_tone_map[9].fq = 5600;
  siglev_tone_map[10].siglev = 115;
  siglev_tone_map[10].fq = 5500;

  cli();  // Disable interrupts

    // Setup PB0 as output (PTT)
  DDRB |= _BV(PB0);


    // Clear OC1A/OC1B on Compare Match when up-counting. Set
    // OC1A/OC1B on Compare Match when downcounting.
    // Fast PWM, TOP=ICR1
    // clkI/O (No prescaling)
  TCCR1A = _BV(COM1A1) | _BV(WGM11);
  TCCR1B = _BV(CS10) | _BV(WGM12) | _BV(WGM13);
  
    /* Timer/Counter 1 Overflow Interrupt Enable */
  //TIMSK = _BV(TOV1);
  TIMSK |= _BV(TOIE1);
  
    // Setup OC1A (PB1) pin as output
  DDRB |= _BV(PB1);
  
  set_sin_fq(siglev_tone_map[0].fq);
  ICR1 = 1;
  
    /* ADC Left adjust result.	 */
    /* AVcc as reference voltage */
  ADMUX = _BV(ADLAR) | _BV(REFS0);
  
    /* ADC interrupt enable */
    /* ADC prescaler division factor 128 */
  ADCSRA = _BV(ADIE) | _BV(ADPS2) | _BV(ADPS1) | _BV(ADPS0);
  
    /* Timer 0 clk/8 prescaler */
  TCCR0 = 0x02;
  
    /* Timer/Counter 0 Overflow Interrupt Enable */
  TIMSK |= _BV(TOIE0);
  
  sei();  // Enable interrupts

  while (1)
  {
    sleep_mode();
  }
  
  return 0;
}


static inline void set_sin_fq(unsigned short fq)
{
  if (fq == cur_fq)
  {
    return;
  }
  cur_fq = fq;
  
  if (fq > 0)
  {
    icr1 = ((((F_CPU << 1) / fq) + 1) >> 1) - 1;
  }
  else
  {
    OCR1A = 0xffff;
    ICR1 = 511;
  }
} /* set_sin_fq */


/*
 * Interrupt handling routine for timer 0 overflow. This timer is used for
 * clocking the A/D conversion interval.
 */
SIGNAL(SIG_OVERFLOW0)
{
  static uint8_t divider = 0;
  
    /* Divide clock down to 400Hz (2.5ms) */
  if (++divider < 18)
  {
    return;
  }
  divider = 0;
  
  adc_ch = (adc_ch + 1) & 0x01;
  ADMUX = (ADMUX & 0xf0) | adc_ch;
  ADCSRA |= (_BV(ADEN) | _BV(ADSC));
  
} /* SIGNAL(SIG_OVERFLOW0) */


/*
 * Interrupt handling routine for time 1 overflow. This timer is used in PWM
 * mode to generate a sqare wave. This interrupt routine is responsible for
 * sliding the tone up or down to the correct value. The tone is slided to
 * avoid too much broad band noise when switching tones.
 */
SIGNAL(SIG_OVERFLOW1)
{
  if (ICR1 != icr1)
  {
    if (ICR1 < icr1)
    {
      ICR1 += 1;
      OCR1A = (ICR1 + 1) >> 1;
    }
    else
    {
      ICR1 -= 1;
      OCR1A = (ICR1 + 1) >> 1;
    }
  }
} /* SIGNAL(SIG_OVERFLOW1) */


/*
 * Interrupt service routine for ADC conversion finished
 */
SIGNAL(SIG_ADC)
{
    /* Read the ADC conversion result */
  uint8_t val = ADCH;
  
  if (adc_ch == RSSI_ADC_CH)
  {
    int i;

    rssi_sum += val;
    if (++rssi_cnt == 4)
    {
      val = rssi_sum >> 2;
      rssi_cnt = 0;
      rssi_sum = 0;
      
      if (val > rssi_hi)
      {
        rssi_hi = val;
        rssi_lo = rssi_hi - WINDOW_SIZE;
      }
      else if (val < rssi_lo)
      {
        rssi_lo = val;
        rssi_hi = rssi_lo + WINDOW_SIZE;
      }
      
      val = rssi_lo + WINDOW_SIZE / 2;
      
      for (i=1; i<11; ++i)
      {
        if (val < siglev_tone_map[i].siglev)
        {
          break;
        }
      }
    
      if (val >= sql_thresh)
      {
        ptt_hang = (400 / 2 / 4) * PTT_HANGTIME;
        if (sql_timeout == -1)
        {
          sql_timeout = (60UL * (400 / 2 / 4)) * SQL_TIMEOUT;
        }
      }
      else if (val <= (sql_thresh - WINDOW_SIZE))
      {
        sql_timeout = -1;
        i = 1;
      }
      else if (ptt_hang > 0)
      {
        ptt_hang = (400 / 2 / 4) * PTT_HANGTIME;
      }
      
      set_sin_fq(siglev_tone_map[i-1].fq);
      
      if (ptt_hang > 0)
      {
        if ((--ptt_hang == 0) ||
            ((sql_timeout > 0) && (--sql_timeout == 0)))
        {
            /* Unkey PTT */
          PORTB &= ~_BV(PB0);
        }
        else if (sql_timeout > 0)
        {
            /* Key PTT */
          PORTB |= _BV(PB0);
        }
      }
    }
  }
  else if (adc_ch == SQL_LVL_ADC_CH)
  {
    if (val > 255 - WINDOW_SIZE)
    {
      sql_thresh = 255;
    }
    else
    {
      sql_thresh = val + WINDOW_SIZE;
    }
    siglev_tone_map[1].siglev = sql_thresh;
  }
  
    /* Turn off ADC clock */
  ADCSRA &= ~_BV(ADEN);
}


