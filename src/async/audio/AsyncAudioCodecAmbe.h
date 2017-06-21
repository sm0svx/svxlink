#ifndef ASYNC_AUDIO_CODEC_AMBE_INCLUDED
#define ASYNC_AUDIO_CODEC_AMBE_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <string>
#include <map>

/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncAudioEncoder.h>
#include <AsyncAudioDecoder.h>

/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Forward declarations
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Namespace
 *
 ****************************************************************************/

namespace Async
{


/****************************************************************************
 *
 * Forward declarations of classes inside of the declared namespace
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Defines & typedefs
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Exported Global Variables
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Class definitions
 *
 ****************************************************************************/

/**
@brief
@author Christian Stussak / porst17
@date

Factory for different implementations of the AMBE codec.
*/
class AudioCodecAmbe : public AudioEncoder, public AudioDecoder
{
  public:
    typedef AudioEncoder::Options Options;

    static AudioCodecAmbe *create(const Options &options = Options());

    /**
     * @brief 	Destructor
     */
    virtual ~AudioCodecAmbe(void) {}

    /**
     * @brief   Get the name of the codec
     * @return  Return the name of the codec
     */
    virtual const char *name(void) const { return "AMBE"; }

  protected:
    /**
    * @brief 	Default constuctor
    */
    AudioCodecAmbe(void) {}

  private:
    AudioCodecAmbe(const AudioCodecAmbe&);
    AudioCodecAmbe& operator=(const AudioCodecAmbe&);
};  /* class AudioCodecAmbe */

} /* namespace */

#endif /* ASYNC_AUDIO_CODEC_AMBE_INCLUDED */

/*
 * This file has not been truncated
 */
