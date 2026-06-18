/* OpenAL EFX extension header — vendored for macOS compatibility.
 *
 * Apple's OpenAL.framework does not include EFX headers.
 * These constants and typedefs are from the OpenAL 1.1 EFX specification
 * (originally distributed with OpenAL-Soft).
 *
 * Functions are loaded at runtime via alGetProcAddress() in EFXfuncs.cpp,
 * so only compile-time constants and typedefs are needed here.
 */

#ifndef AL_EFX_H
#define AL_EFX_H

#include <al.h>
#include <alc.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Effect types */
#define AL_EFFECT_TYPE                          0x8001
#define AL_EFFECT_NULL                          0x0000
#define AL_EFFECT_REVERB                        0x0001
#define AL_EFFECT_CHORUS                        0x0002
#define AL_EFFECT_DISTORTION                    0x0003
#define AL_EFFECT_ECHO                          0x0004
#define AL_EFFECT_FLANGER                       0x0005
#define AL_EFFECT_FREQUENCY_SHIFTER             0x0006
#define AL_EFFECT_VOCAL_MORPHER                 0x0007
#define AL_EFFECT_PITCH_SHIFTER                 0x0008
#define AL_EFFECT_RING_MODULATOR                0x0009
#define AL_EFFECT_AUTOWAH                       0x000A
#define AL_EFFECT_COMPRESSOR                    0x000B
#define AL_EFFECT_EQUALIZER                     0x000C
#define AL_EFFECT_EAXREVERB                     0x8000

/* EAX Reverb effect parameters */
#define AL_EAXREVERB_DENSITY                    0x0001
#define AL_EAXREVERB_DIFFUSION                  0x0002
#define AL_EAXREVERB_GAIN                       0x0003
#define AL_EAXREVERB_GAINHF                     0x0004
#define AL_EAXREVERB_GAINLF                     0x0005
#define AL_EAXREVERB_DECAY_TIME                 0x0006
#define AL_EAXREVERB_DECAY_HFRATIO              0x0007
#define AL_EAXREVERB_DECAY_LFRATIO              0x0008
#define AL_EAXREVERB_REFLECTIONS_GAIN           0x0009
#define AL_EAXREVERB_REFLECTIONS_DELAY          0x000A
#define AL_EAXREVERB_REFLECTIONS_PAN            0x000B
#define AL_EAXREVERB_LATE_REVERB_GAIN           0x000C
#define AL_EAXREVERB_LATE_REVERB_DELAY          0x000D
#define AL_EAXREVERB_LATE_REVERB_PAN            0x000E
#define AL_EAXREVERB_ECHO_TIME                  0x000F
#define AL_EAXREVERB_ECHO_DEPTH                 0x0010
#define AL_EAXREVERB_MODULATION_TIME            0x0011
#define AL_EAXREVERB_MODULATION_DEPTH           0x0012
#define AL_EAXREVERB_AIR_ABSORPTION_GAINHF      0x0013
#define AL_EAXREVERB_HFREFERENCE                0x0014
#define AL_EAXREVERB_LFREFERENCE                0x0015
#define AL_EAXREVERB_ROOM_ROLLOFF_FACTOR        0x0016
#define AL_EAXREVERB_DECAY_HFLIMIT              0x0017

/* Filter types */
#define AL_FILTER_TYPE                          0x8001
#define AL_FILTER_NULL                          0x0000
#define AL_FILTER_LOWPASS                       0x0001
#define AL_FILTER_HIGHPASS                      0x0002
#define AL_FILTER_BANDPASS                      0x0003

/* Lowpass filter parameters */
#define AL_LOWPASS_GAIN                         0x0001
#define AL_LOWPASS_GAINHF                       0x0002

/* Highpass filter parameters */
#define AL_HIGHPASS_GAIN                        0x0001
#define AL_HIGHPASS_GAINLF                      0x0002

/* Bandpass filter parameters */
#define AL_BANDPASS_GAIN                        0x0001
#define AL_BANDPASS_GAINLF                      0x0002
#define AL_BANDPASS_GAINHF                      0x0003

/* Auxiliary effect slot properties */
#define AL_EFFECTSLOT_EFFECT                    0x0001
#define AL_EFFECTSLOT_GAIN                      0x0002
#define AL_EFFECTSLOT_AUXILIARY_SEND_AUTO        0x0003
#define AL_EFFECTSLOT_NULL                      0x0000

/* Context attribute for max auxiliary sends */
#define ALC_MAX_AUXILIARY_SENDS                 0x20003

/* Air absorption factor range */
#define AL_MIN_AIR_ABSORPTION_FACTOR            0.0f
#define AL_MAX_AIR_ABSORPTION_FACTOR            10.0f

/* Source properties for auxiliary send filter */
#define AL_DIRECT_FILTER                        0x20005
#define AL_AUXILIARY_SEND_FILTER                 0x20006
#define AL_AIR_ABSORPTION_FACTOR                0x20007
#define AL_DIRECT_FILTER_GAINHF_AUTO            0x2000B
#define AL_AUXILIARY_SEND_FILTER_GAIN_AUTO       0x2000C
#define AL_AUXILIARY_SEND_FILTER_GAINHF_AUTO     0x2000D

/* EFX function pointer types — loaded at runtime via alGetProcAddress() */

/* Effect functions */
typedef void          (AL_APIENTRY *LPALGENEFFECTS)(ALsizei, ALuint*);
typedef void          (AL_APIENTRY *LPALDELETEEFFECTS)(ALsizei, const ALuint*);
typedef ALboolean     (AL_APIENTRY *LPALISEFFECT)(ALuint);
typedef void          (AL_APIENTRY *LPALEFFECTI)(ALuint, ALenum, ALint);
typedef void          (AL_APIENTRY *LPALEFFECTIV)(ALuint, ALenum, const ALint*);
typedef void          (AL_APIENTRY *LPALEFFECTF)(ALuint, ALenum, ALfloat);
typedef void          (AL_APIENTRY *LPALEFFECTFV)(ALuint, ALenum, const ALfloat*);
typedef void          (AL_APIENTRY *LPALGETEFFECTI)(ALuint, ALenum, ALint*);
typedef void          (AL_APIENTRY *LPALGETEFFECTIV)(ALuint, ALenum, ALint*);
typedef void          (AL_APIENTRY *LPALGETEFFECTF)(ALuint, ALenum, ALfloat*);
typedef void          (AL_APIENTRY *LPALGETEFFECTFV)(ALuint, ALenum, ALfloat*);

/* Filter functions */
typedef void          (AL_APIENTRY *LPALGENFILTERS)(ALsizei, ALuint*);
typedef void          (AL_APIENTRY *LPALDELETEFILTERS)(ALsizei, const ALuint*);
typedef ALboolean     (AL_APIENTRY *LPALISFILTER)(ALuint);
typedef void          (AL_APIENTRY *LPALFILTERI)(ALuint, ALenum, ALint);
typedef void          (AL_APIENTRY *LPALFILTERIV)(ALuint, ALenum, const ALint*);
typedef void          (AL_APIENTRY *LPALFILTERF)(ALuint, ALenum, ALfloat);
typedef void          (AL_APIENTRY *LPALFILTERFV)(ALuint, ALenum, const ALfloat*);
typedef void          (AL_APIENTRY *LPALGETFILTERI)(ALuint, ALenum, ALint*);
typedef void          (AL_APIENTRY *LPALGETFILTERIV)(ALuint, ALenum, ALint*);
typedef void          (AL_APIENTRY *LPALGETFILTERF)(ALuint, ALenum, ALfloat*);
typedef void          (AL_APIENTRY *LPALGETFILTERFV)(ALuint, ALenum, ALfloat*);

/* Auxiliary effect slot functions */
typedef void          (AL_APIENTRY *LPALGENAUXILIARYEFFECTSLOTS)(ALsizei, ALuint*);
typedef void          (AL_APIENTRY *LPALDELETEAUXILIARYEFFECTSLOTS)(ALsizei, const ALuint*);
typedef ALboolean     (AL_APIENTRY *LPALISAUXILIARYEFFECTSLOT)(ALuint);
typedef void          (AL_APIENTRY *LPALAUXILIARYEFFECTSLOTI)(ALuint, ALenum, ALint);
typedef void          (AL_APIENTRY *LPALAUXILIARYEFFECTSLOTIV)(ALuint, ALenum, const ALint*);
typedef void          (AL_APIENTRY *LPALAUXILIARYEFFECTSLOTF)(ALuint, ALenum, ALfloat);
typedef void          (AL_APIENTRY *LPALAUXILIARYEFFECTSLOTFV)(ALuint, ALenum, const ALfloat*);
typedef void          (AL_APIENTRY *LPALGETAUXILIARYEFFECTSLOTI)(ALuint, ALenum, ALint*);
typedef void          (AL_APIENTRY *LPALGETAUXILIARYEFFECTSLOTIV)(ALuint, ALenum, ALint*);
typedef void          (AL_APIENTRY *LPALGETAUXILIARYEFFECTSLOTF)(ALuint, ALenum, ALfloat*);
typedef void          (AL_APIENTRY *LPALGETAUXILIARYEFFECTSLOTFV)(ALuint, ALenum, ALfloat*);

#ifdef __cplusplus
}
#endif

#endif /* AL_EFX_H */
