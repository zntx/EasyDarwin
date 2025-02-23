#include "player.h"
#include "rtsp-session.h"
#include "pusher.h"
#include "fmt.h"
#include "hdlog.h"
#include "comon.h"
#include "utils.h"

using namespace std;

Player* NewPlayer(Session* session , Pusher* pusher) 
{
	auto queueLimit = utils::Conf().Section("rtsp").Key("player_queue_limit").MustInt(0);
	auto ropPacketWhenPaused = utils::Conf().Section("rtsp").Key("drop_packet_when_paused").MustInt(0);
	auto player = new Player( session , pusher) ;

	session->StopHandles.emplace_back ( [&]() {
        pusher->RemovePlayer(player);
        //player->cond.Broadcast();
    });
	return player;
}

Player::Player(Session* session , Pusher* pusher) :queue(),cond()
{
	auto queue_limit = utils::Conf().Section("rtsp").Key("player_queue_limit").MustInt(0);
	auto ropPacketWhenPaused = utils::Conf().Section("rtsp").Key("drop_packet_when_paused").MustInt(0);
	
    session = session;
    pusher  = pusher;
    //cond    = sync.NewCond(&sync.Mutex{});
    //queue   = make([]*RTPPack, 0);
    queueLimit = queue_limit;
    dropPacketWhenPaused = dropPacketWhenPaused != 0;
    paused  = false;
}

Player* Player::QueueRTP(RTPPack* pack) 
{
	//logger := this->logger
	if (pack == nullptr ){
		logger::info("player queue enter nil pack, drop it");
		return this ;
	}
	if (this->paused && this->dropPacketWhenPaused) {
		return this;
	}
	this->cond.lock();
	this->queue.push_back(pack);

    auto oldLen =  this->queue.size();
	if ( this->queueLimit > 0 && oldLen > this->queueLimit) {
		this->queue.pop_front();
		if (this->session->debugLogEnable ){
			auto len = this->queue.size();
			logger::info("Player {}, QueueRTP, exceeds limit{}, drop %d old packets, current queue.len={}\n", this->String(), this->queueLimit, oldLen - len, len);
		}
	}
	//this->cond.Signal();
	this->cond.unlock();
	return this;
}

void Player::Start() 
{
	//logger := this->logger;
	//auto timer := time.Unix(0, 0);
    auto timer = chrono::system_clock::now();
	while (!this->session->Stoped) {
		RTPPack* pack = nullptr;
		this->cond.lock();
		// if (this->queue.empty()  ) {
		// 	this->cond.Wait();
		// }
		if (!this->queue.empty()) {
//			pack = this->queue.front();
//			this->queue.pop_front();
            auto it = this->queue.begin();
            // ��ȡͷ��Ԫ�ص�ֵ
            pack = *it;
            // �Ƴ�ͷ��Ԫ��
            this->queue.erase(it);
		}
		auto queueLen = this->queue.size();
		this->cond.unlock();

		if (this->paused) {
			continue;
		}
		if (pack == nullptr) {
			if( !this->session->Stoped) {
				logger::info("player not stoped, but queue take out nil pack");
			}
			continue;
		}

        auto err = this->session->SendRTP(pack);
		if (err.is_err()) {
			logger::info("{}\n",err.unwrap_err());
		}
		auto elapsed = chrono::system_clock::now() - timer;
		if (this->session->debugLogEnable && elapsed >= chrono::seconds(30)) {
			logger::info("Player %s, Send a package.type:%d, queue.len=%d\n", this->session->String(), pack->Type, queueLen);
			timer = chrono::system_clock::now();
		}
	}
}

void Player::Pause(bool paused)
{
	if (paused) {
		logger::info("Player {}, Pause\n", this->session->String());
	} 
    else {
		logger::info("Player {}, Play\n", this->session->String());
	}
	this->cond.lock();
	if (paused && this->dropPacketWhenPaused && !this->queue.empty()) {
		//this->queue = make([]*RTPPack, 0);
        this->queue.clear();
	}
	this->paused = paused;
	this->cond.unlock();
}