#ifndef UDP_SERVER_H
#define UDP_SERVER_H

#include "net.h"

class  Session;

class UDPServer  {
public:
    Session* session;
    RTSPClient* rtspClient;
    int                 APort;
    //Option<UdpSocket>   AConn ;
    int                 AControlPort;
    //Option<UdpSocket>   AControlConn;
    int                 VPort;
    //Option<UdpSocket>   VConn;
    int                 VControlPort;
    //Option<UdpSocket>   VControlConn;
    bool Stoped ;

    void AddInputBytes(int bytes ) const;
    void HandleRTP(RTPPack* pack) const;
    void Stop();
    Result<void> Setup(int _type);
    Result<void> SetupAudio();
    Result<void> SetupVideo();
};







#endif //UDP_SERVER_H