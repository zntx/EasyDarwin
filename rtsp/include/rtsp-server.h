#ifndef RTSP_SERVER_H
#define RTSP_SERVER_H

#include <map>
#include <mutex>
#include "net.h"

class Pusher;
class Session;

using namespace std;

class Server{
	
	TcpListener* tcpListener ;
	int TCPPort;        
	bool Stoped;         
	map<string, Pusher*> pushers;         // Path <-> Pusher
	mutex pushersLock ;
	//chan *Pusher addPusherCh ;
	//chan *Pusher removePusherCh ;

public:
    Server();
    Result<void> Start();

    void Stop();

    bool AddPusher(Pusher* pusher);

    Result<Pusher *> TryAttachToPusher(Session* session);

    void RemovePusher(Pusher* pusher);

    Pusher* GetPusher(const string& path);

    map<string, Pusher*> GetPushers();

    int GetPusherSize();
};

Server* GetServer();

#endif //RTSP_SERVER_H