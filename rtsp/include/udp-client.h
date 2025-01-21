//
// Created by zhangyuzhu8 on 2025/1/18.
//

#ifndef EASYDARWIN_UDP_CLIENT_H
#define EASYDARWIN_UDP_CLIENT_H

#include <iostream>
#include "net.h"

typedef Result<void> RtspErr;

class Session;
struct RTPPack;

class UDPClient {
private:
    Session*          session;
    uint64_t          APort ;
    Option<UdpSocket> AConn;
    uint64_t          AControlPort;
    Option<UdpSocket> AControlConn;
    uint64_t          VPort;
    Option<UdpSocket> VConn;
    uint64_t          VControlPort;
    Option<UdpSocket> VControlConn;
    bool              Stoped;

public:
    void Stop();
    RtspErr SetupAudio();
    RtspErr SetupVideo();
    RtspErr SendRTP( RTPPack* pack);
};


#endif //EASYDARWIN_UDP_CLIENT_H
