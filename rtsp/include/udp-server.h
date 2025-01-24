#ifndef UDP_SERVER_H
#define UDP_SERVER_H

#include "net.h"

class  Session;

class UDPServer  {
public:
    Session* session;
    RTSPClient* rtspClient;
    int                 APort;
    Option<UdpSocket>   AConn ;
    int                 AControlPort;
    Option<UdpSocket>   AControlConn;
    int                 VPort;
    Option<UdpSocket>   VConn;
    int                 VControlPort;
    Option<UdpSocket>   VControlConn;
    bool Stoped ;

    void AddInputBytes(int bytes );
    void HandleRTP(RTPPack* pack);
    void Stop();
    Result<void> SetupAudio();
    Result<void> SetupVideo();
};







#endif //UDP_SERVER_H