//
// Created by zhangyuzhu8 on 2025/1/18.
//

#ifndef EASYDARWIN_RTSP_SESSION_H
#define EASYDARWIN_RTSP_SESSION_H

#include <vector>
#include "net/net.h"
#include "hdlog.h"

using namespace std;
class Server;
class Pusher;
class Player;
class UDPClient;
class Request;


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
    int Type;
    //bytes.Buffer* Buffer;
    Slice<char> Buffer;
};

typedef void (*pRTPHandles)( RTPPack*);
typedef void (*pStopHandles)( );



class Session  {
public:
    Session( TcpStream& conn);
    void Stop();
    void Start();
    void handleRequest(Request *req );
    RtspErr SendRTP(RTPPack* pack);

public:
    //SessionLogger
    string       ID;
    Server*      Server;
    //            Conn      *RichConn
    TcpStream Conn;
    //            connRW    *bufio.ReadWriter
    //            connWLock sync.RWMutex
    SessionType  Type;
    TransType    TransType;
    string       Path;
    string       URL;
    string       SDPRaw;
    //map[string]*SDPInfo       SDPMap;

    bool         authorizationEnable;
    string       nonce;
    bool         closeOld;
    bool         debugLogEnable;

    string       AControl;
    string       VControl;
    string       ACodec;
    string       VCodec;

    // stats info
    size_t         InBytes;
    size_t         OutBytes;
    //time.Time    StartAt;
    int         Timeout;

    bool        Stoped;

    //tcp channels
    int         aRTPChannel;
    int         aRTPControlChannel;
    int         vRTPChannel;
    int         vRTPControlChannel;

    Pusher*      Pusher;
    Player*      Player;
    UDPClient*   UDPClient;
    vector<pRTPHandles>   RTPHandles;
    vector<pStopHandles > StopHandles;


};



#endif //EASYDARWIN_RTSP_SESSION_H
