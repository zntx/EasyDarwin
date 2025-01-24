//
// Created by zhangyuzhu8 on 2025/1/22.
//

#ifndef EASYDARWIN_PUSHER_H
#define EASYDARWIN_PUSHER_H

#include <map>
#include <mutex>
#include <string>
#include<list>
#include "comon.h"

using namespace std;
class Session;
class RTSPClient;
class UDPServer;
class RTSPClient;
class Player;
class Server;

class Pusher {
    Session *session;
    RTSPClient *rtspClient;
    map<string, Player*> players; //SessionID <-> Player
    mutex playersLock       ;
    bool gopCacheEnable ;
    list<RTPPack*> gopCache  ;
    mutex gopCacheLock    ;
    UDPServer         *UDPServer;
    bool spsppsInSTAPaPack ;
    mutex cond  ;
    list<RTPPack*> queue ;

public:
    string String();
    Server* Server() ;

    string SDPRaw();

    bool Stoped();
    string Path();
    string ID();

    string VCodec();
    string ACodec();

    string AControl();

    string VControl();

    string URL();

    void AddOutputBytes( int);

    int InBytes();

    int OutBytes();

    string TransType();

    time.Time StartAt() ;

    string Source();


    void bindSession(Session* session ) ;

    bool RebindSession(Session* session);

    bool RebindClient(RTSPClient* client);

    Pusher* QueueRTP(RTPPack* pack);
    void Start() ;

    void Stop() ;

    Pusher*  BroadcastRTP(RTPPack* pack ) ;

    map<string, Player*> GetPlayers();

    bool HasPlayer(Player* player);

    Pusher* AddPlayer(Player* player);

    Pusher*  RemovePlayer(Player* player);

    void ClearPlayer() ;

    bool shouldSequenceStart(RTPInfo* rtp);

};

Pusher* NewClientPusher(RTSPClient* client);

Pusher* NewPusher(Session* session);



#endif //EASYDARWIN_PUSHER_H
