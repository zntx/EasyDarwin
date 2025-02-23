//
// Created by zhangyuzhu8 on 2025/1/18.
//

#ifndef EASYDARWIN_RTSP_SESSION_H
#define EASYDARWIN_RTSP_SESSION_H

#include <vector>
#include "net.h"
#include "hdlog.h"
#include "comon.h"

using namespace std;
class Server;
class Pusher;
class Player;
class UDPClient;
class Request;


#define UDP_BUF_SIZE 1048576



class Session  {


public:
    //SessionLogger
    string       ID;
    Server*      Server;
    //            Conn      *RichConn
    TcpStream Conn;
    time_t timeout;
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
    vector<function<void(RTPPack*)>>  RTPHandles;
    vector<function<void(void)>>  StopHandles;

public:
    string String();
    Session( TcpStream& conn);
    void Stop();
    void Start();
    void handleRequest(Request *req );
    Result<void> SendRTP(RTPPack* pack);

};


Session* NewSession(Server* server, TcpStream conn );


#endif //EASYDARWIN_RTSP_SESSION_H
