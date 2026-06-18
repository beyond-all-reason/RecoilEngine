/* OpenAL extensions header — vendored for macOS compatibility.
 *
 * Apple's OpenAL.framework does not include alext.h.
 * These constants and typedefs are from OpenAL-Soft's alext.h
 * for the ALC_SOFT_loopback extension used by the sound system.
 *
 * Functions are loaded at runtime via alcGetProcAddress().
 */

#ifndef AL_ALEXT_H
#define AL_ALEXT_H

#include <al.h>
#include <alc.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ALC_SOFT_loopback */
#define ALC_SOFT_loopback                       1
#define ALC_FORMAT_CHANNELS_SOFT                0x1990
#define ALC_FORMAT_TYPE_SOFT                    0x1991

/* Sample types */
#define ALC_BYTE_SOFT                           0x1400
#define ALC_UNSIGNED_BYTE_SOFT                  0x1401
#define ALC_SHORT_SOFT                          0x1402
#define ALC_UNSIGNED_SHORT_SOFT                 0x1403
#define ALC_INT_SOFT                            0x1404
#define ALC_UNSIGNED_INT_SOFT                   0x1405
#define ALC_FLOAT_SOFT                          0x1406

/* Channel configurations */
#define ALC_MONO_SOFT                           0x1500
#define ALC_STEREO_SOFT                         0x1501
#define ALC_QUAD_SOFT                           0x1503
#define ALC_5POINT1_SOFT                        0x1504
#define ALC_6POINT1_SOFT                        0x1505
#define ALC_7POINT1_SOFT                        0x1506

/* Loopback function pointer types */
typedef ALCdevice*  (ALC_APIENTRY *LPALCLOOPBACKOPENDEVICESOFT)(const ALCchar*);
typedef ALCboolean  (ALC_APIENTRY *LPALCISRENDERFORMATSUPPORTEDSOFT)(ALCdevice*, ALCsizei, ALCenum, ALCenum);
typedef void        (ALC_APIENTRY *LPALCRENDERSAMPLESSOFT)(ALCdevice*, ALCvoid*, ALCsizei);

#ifdef __cplusplus
}
#endif

#endif /* AL_ALEXT_H */
