#ifndef RTSP_PLAYER_H
#define RTSP_PLAYER_H


#include <list>
#include <chrono>
#include <mutex>
#include "comon.h"

using namespace std;

class Session;
class Pusher;

class Player {
public:
	Session *session{};
	Pusher *pusher{};
private:
	//*sync.Cond cond ;
    mutex cond;  
	//queue  []*RTPPack;
    list<RTPPack*> queue;
	int queueLimit{} ;
	bool dropPacketWhenPaused; 
	bool paused ;


public:
    Player(Session* session , Pusher* pusher);
    Player* QueueRTP(RTPPack* pack) ;  
    void Start();
    void Pause(bool paused);
};

Player* NewPlayer(Session* session , Pusher* pusher);

#endif