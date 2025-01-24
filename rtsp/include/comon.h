#ifndef RTSP_COMMON_H
#define RTSP_COMMON_H

#include "net/Slice.h"

enum class SessionType {
    SESSION_TYPE_PUSHER,
    SESSEION_TYPE_PLAYER,
};

enum class TransType {
    TRANS_TYPE_TCP,
    TRANS_TYPE_UDP,
};

enum  RTPType {
    RTP_TYPE_AUDIO,
    RTP_TYPE_VIDEO,
    RTP_TYPE_AUDIOCONTROL,
    RTP_TYPE_VIDEOCONTROL,
};

struct RTPPack  {
    RTPPack() = default;

    int Type;
    //bytes.Buffer* Buffer;
    Slice<char> Buffer;
};

typedef void (*pRTPHandles)( RTPPack*);
typedef void (*pStopHandles)( void);


#endif //RTSP_COMMON_H