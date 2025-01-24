//
// Created by zhangyuzhu8 on 2025/1/22.
//

#ifndef EASYDARWIN_RTSP_CLIENT_H
#define EASYDARWIN_RTSP_CLIENT_H

#include "net.h"

using namespace std;

class Server;
class UDPServer;
class Response;

class RTSPClient {
public:
    Server *server;
    //SessionLogger
    bool        	Stoped ;
    string        	Status;
    string        	URL ;
    string        	Path ;
    string          CustomPath  ;//custom path for pusher
    string          ID ;
    TcpStream*      Conn;
    *bufio.ReadWriter	connRW ;
    string          Session ;
    int             Seq  ;
    int        			InBytes ;
    int        			OutBytes ;
    TransType        	TransType ;
    chrono::_V2::system_clock::time_point  StartAt ;
    *sdp.Session     	Sdp ;
    string        	AControl;
    string        	VControl ;
    string        	ACodec ;
    string        	VCodec ;
    int64_t         OptionIntervalMillis ;
    string        	SDPRaw  ;

    bool 			debugLogEnable;
    uint16_t 		lastRtpSN;

    string			Agent  ;
    string			authLine ;

    //tcp channels
    int aRTPChannel        ;
    int aRTPControlChannel ;
    int vRTPChannel        ;
    int vRTPControlChannel ;

    UDPServer* 		     UDPServer ;
    vector<pRTPHandles>  RTPHandles ;
    vector<pStopHandles> StopHandles ;



public:
    RTSPClient( Server *server, string rawUrl ,string path, int64_t sendOptionMillis, string agent);

    string String();

    template <typename Rep, typename Period>
    Result<void>  Start( chrono::duration<Rep, Period>& timeout );
    void Stop();

    Result<Response*> RequestWithPath(string method, string path , map<string,string> headers, bool needResp);

    Result<Response*> Request(string method, map<string,string> headers);

    Result<void> RequestNoResp(string method , map<string,string> headers);

private:
    Result<string> checkAuth(string method, Response* resp);

    template <typename Rep, typename Period>
    Result<void>  requestStream( chrono::duration<Rep, Period>& timeout );

    Result<void>  startStream();

};


#endif //EASYDARWIN_RTSP_CLIENT_H
