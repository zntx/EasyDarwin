#include <thread>

#include "pusher.h"
#include "player.h"
#include "rtsp-session.h"
#include "fmt.h"
#include "hdlog.h"
#include "comon.h"
#include "utils.h"

string Pusher::String()
{
	if (this->session != nullptr ){
		return this->session->String();
	}
	return this->rtspClient->String();
}

Server* Pusher::Server() {
	if (this->session != nullptr) {
		return this->session->Server;
	}
	return (this->rtspClient->Server);
}

string Pusher::SDPRaw()
{
	if (this->session != nullptr) {
		return this->session->SDPRaw;
	}
	return this->rtspClient->SDPRaw;
}

bool Pusher::Stoped()
{
	if( this->session != nullptr) {
		return this->session->Stoped;
	}
	return this->rtspClient->Stoped;
}

string Pusher::Path()
{
	if (this->session != nullptr) {
		return this->session->Path;
	}
	if (this->rtspClient->CustomPath != "") {
		return this->rtspClient->CustomPath;
	}
	return this->rtspClient->Path;
}

string  Pusher::ID()
{
	if( this->session != nullptr) {
		return this->session->ID;
	}
	return this->rtspClient->ID;
}

// log.Logger* Pusher::Logger()
// {
// 	if (this->session != nullptr) {
// 		return this->session.logger
// 	}
// 	return this->rtspClient.logger
// }

string Pusher::VCodec()  {
	if (this->session != nullptr) {
		return this->session->VCodec;
	}
	return this->rtspClient->VCodec;
}

string Pusher::ACodec()
{
	if (this->session != nullptr ){
		return this->session.ACodec;
	}
	return this->rtspClient.ACodec;
}

string Pusher::AControl()
{
	if (this->session != nullptr ){
		return this->session->AControl;
	}
	return this->rtspClient->AControl;
}

string Pusher::VControl()
{
	if (this->session != nullptr ){
		return this->session->VControl;
	}
	return this->rtspClient->VControl;
}

string Pusher::URL()
{
	if (this->session != nullptr) {
		return this->session->URL;
	}
	return this->rtspClient->URL;
}

void Pusher::AddOutputBytes( int size)
{
	if (this->session != nullptr) {
		this->session->OutBytes += size;
		return ;
	}
	this->rtspClient->OutBytes += size;
}

int Pusher::InBytes()
{
	if (this->session != nullptr ){
		return this->session->InBytes;
	}
	return this->rtspClient->InBytes;
}

int Pusher::OutBytes()
{
	if (this->session != nullptr ){
		return this->session->OutBytes;
	}
	return this->rtspClient->OutBytes;
}

string Pusher::TransType()
{
	if (this->session != nullptr ){
		return this->session->TransType.String();
	}
	return this->rtspClient->TransType.String();
}

time.Time Pusher::StartAt()
{
	if( this->session != nullptr) {
		return this->session->StartAt;
	}
	return this->rtspClient->StartAt;
}

string Pusher::Source()
{
	if (this->session != nullptr) {
		return this->session->URL;
	}
	return this->rtspClient->URL;
}

Pusher* NewClientPusher(RTSPClient* client)
{
	pusher = &Pusher{
		RTSPClient:     client,
		Session:        nullptr,
		players:        make(map[string]*Player),
		gopCacheEnable: utils.Conf().Section("rtsp").Key("gop_cache_enable").MustBool(true),
		gopCache:       make([]*RTPPack, 0),

		cond:  sync.NewCond(&sync.Mutex{}),
		queue: make([]*RTPPack, 0),
	}
	client.RTPHandles = append(client.RTPHandles, func(pack *RTPPack) {
		this->QueueRTP(pack);
	})
	client.StopHandles = append(client.StopHandles, func() {
		this->ClearPlayer();
		this->Server().RemovePusher(pusher)
		this->cond.Broadcast();
	})
	return
}

Pusher* NewPusher(Session* session)
{
	pusher = &Pusher{
		Session:        session,
		RTSPClient:     nullptr,
		players:        make(map[string]*Player),
		gopCacheEnable: utils.Conf().Section("rtsp").Key("gop_cache_enable").MustBool(true),
		gopCache:       make([]*RTPPack, 0),

		cond:  sync.NewCond(&sync.Mutex{}),
		queue: make([]*RTPPack, 0),
	}
	this->bindSession(session);
	return
}

void Pusher::bindSession(Session* session )
{
	this->session = session;
	session->RTPHandles.emplace_back( [&](RTPPack *pack) {
		if (session != this->session) {
			logger::Printf("Session recv rtp to this->but pusher got a new session{}.", this->session->ID);
			return ;
		}
		this->QueueRTP(pack);
	});
	session->StopHandles.emplace_back( [&]() {
		if (session != this->session) {
			logger::Printf("Session stop to release this->but pusher got a new session[%v].", this->session->ID);
			return ;
		}
		this->ClearPlayer();
		this->Server()->RemovePusher(this);
		this->cond.Broadcast();
		if (this->UDPServer != nullptr ){
			this->UDPServer->Stop();
			this->UDPServer = nullptr;
		}
	});
}

bool Pusher::RebindSession(Session* session)
{
	if (this->rtspClient != nullptr ){
        logger::Printf("call RebindSession[%s] to a Client-Pusher. got false", session->ID);
		return false;
	}
	auto sess = this->session;
	this->bindSession(session);
	session->Pusher = this;

	this->gopCacheLock.lock();
	this->gopCache.clear();
	this->gopCacheLock.unlock();
	if (sess != nullptr ){
		sess->Stop();
	}
	return true;
}

bool Pusher::RebindClient(RTSPClient* client)
{
	if (this->session != nullptr) {
        logger::Printf("call RebindClient[%s] to a Session-Pusher. got false", client->ID);
		return false;
	}
	auto sess = this->rtspClient;
	this->rtspClient = client;
	if (sess != nullptr ){
		sess->Stop();
	}
	return true;
}

Pusher* Pusher::QueueRTP(RTPPack* pack)
{
	this->cond.lock();
	this->queue.push_back(  pack);
	//this->cond.Signal();
	this->cond.unlock();
	return this;
}

void Pusher::Start() 
{
	//logger := this->Logger();
	while (!this->Stoped() ){
		RTPPack *pack = nullptr;

		this->cond.lock();
		if ( this->queue.empty()  ){
			//this->cond.Wait();
		}
		if ( !this->queue.empty() ){
			auto it = this->queue.begin();
			this->queue.erase(it);
            pack = *it;
		}
		this->cond.unlock();

		if (pack == nullptr) {
			if (!this->Stoped() ){
				logger::Printf("pusher not stoped, but queue take out nullptr pack");
			}
			continue;
		}

		if (this->gopCacheEnable && pack->Type == RTP_TYPE_VIDEO) {
			this->gopCacheLock.lock();
            auto rtp = ParseRTP(pack.Buffer.Bytes());
			if ( rtp != nullptr && this->shouldSequenceStart(rtp)) {
				this->gopCache.clear();
			}
			this->gopCache.push_back( pack);
			this->gopCacheLock.unlock();
		}
		this->BroadcastRTP(pack);
	}
}

void Pusher::Stop() 
{
	if (this->session != nullptr) {
		this->session->Stop();
		return;
	}
	this->rtspClient->Stop();
}

Pusher*  Pusher::BroadcastRTP(RTPPack* pack ) 
{
	for( auto &&player : this->GetPlayers()) {
		player.second->QueueRTP(pack);
		this->AddOutputBytes(pack.Buffer.Len());
	}
	return this;
}

map<string, Player*> Pusher::GetPlayers()
{
    map<string,Player*> players;
	this->playersLock.lock();
//	for( auto &&k : this->players ){
//		players[k.first] = k.second;
//	}
    players = this->players;
	this->playersLock.unlock();
	return players;
}

bool Pusher::HasPlayer(Player* player)  
{
    bool ok = false;
	this->playersLock.lock();
    ok = this->players.find(player->session->ID) != this->players.end();
	this->playersLock.unlock();
	return ok;
}

Pusher* Pusher::AddPlayer(Player* player) 
{
	//logger := this->Logger();
	if (this->gopCacheEnable ){
		this->gopCacheLock.lock();
		for ( auto && pack : this->gopCache) {
			player->QueueRTP(pack);
			this->AddOutputBytes(pack.Buffer.Len());
		}
		this->gopCacheLock.unlock();
	}

	this->playersLock.lock();
	if (this->players.find(player->session->ID) == this->players.end()) {
		this->players[player->session->ID] = player;

        //go player->Start();
        thread ss(&Player::Start, player);
        ss.detach();

		logger::Printf("%v start, now player size[%d]", player, this->players.size());
	}
	this->playersLock.unlock();
	return this;
}

Pusher*  Pusher::RemovePlayer(Player* _player)
{
	//logger := this->Logger();
	this->playersLock.lock();
	if ( this->players.empty() ){
		this->playersLock.unlock();
		return this;
	}
	delete(this->players, _player->session->ID);


	logger::Printf("{} end, now player size[%d]\n", _player, this->players.size());
	this->playersLock.unlock();
	return this;
}

void Pusher::ClearPlayer()
{
	// copy a new map to avoid deadlock
	this->playersLock.lock();
	auto players = this->players;
	this->players.clear();
	this->playersLock.unlock();

//	go func() { // do not block
//		for _, v := range players {
//			v.Stop()
//		}
//	}()
    thread ss([&](){
        for( auto && v: players) {
            v.second->session->Stop();
        }
    });
    ss.detach();

}

bool Pusher::shouldSequenceStart(RTPInfo* rtp)  
{
	if (this->VCodec() == "h264") {
        uint8_t realNALU = 0;
		payloadHeader := rtp.Payload[0] //https://tools.ietf.org/html/rfc6184#section-5.2
		NaluType := uint8(payloadHeader & 0x1F)
		// log.Printf("RTP Type:%d", NaluType)
		switch {
		case NaluType <= 23:
			realNALU = rtp.Payload[0]
			// log.Printf("Single NAL:%d", NaluType)
		case NaluType == 28 || NaluType == 29:
			realNALU = rtp.Payload[1]
			if realNALU&0x40 != 0 {
				// log.Printf("FU NAL End :%02X", realNALU)
			}
			if realNALU&0x80 != 0 {
				// log.Printf("FU NAL Begin :%02X", realNALU)
			} else {
				return false
			}
		case NaluType == 24:
			// log.Printf("STAP-A")
			off := 1
			singleSPSPPS := 0
			for {
				nalSize := ((uint16(rtp.Payload[off])) << 8) | uint16(rtp.Payload[off+1])
				if nalSize < 1 {
					return false
				}
				off += 2
				nalUnit := rtp.Payload[off : off+int(nalSize)]
				off += int(nalSize)
				realNALU = nalUnit[0]
				singleSPSPPS += int(realNALU & 0x1F)
				if off >= len(rtp.Payload) {
					break
				}
			}
			if singleSPSPPS == 0x0F {
				this->spsppsInSTAPaPack = true
				return true
			}
		}
		if realNALU&0x1F == 0x05 {
			if this->spsppsInSTAPaPack {
				return false
			}
			return true
		}
		if realNALU&0x1F == 0x07 { // maybe sps pps header + key frame?
			if len(rtp.Payload) < 200 { // consider sps pps header only.
				return true
			}
			return true
		}
		return false
	}
    else if (this->VCodec() == "h265") {
		if len(rtp.Payload) >= 3 {
			firstByte := rtp.Payload[0]
			headerType := (firstByte >> 1) & 0x3f
			var frameType uint8
			if headerType == 49 { //Fragmentation Units

				FUHeader := rtp.Payload[2]
				/*
				   +---------------+
				   |0|1|2|3|4|5|6|7|
				   +-+-+-+-+-+-+-+-+
				   |S|E|  FuType   |
				   +---------------+
				*/
				rtpStart := (FUHeader & 0x80) != 0
				if !rtpStart {
					if (FUHeader & 0x40) != 0 {
						//log.Printf("FU frame end")
					}
					return false
				} else {
					//log.Printf("FU frame start")
				}
				frameType = FUHeader & 0x3f
			} else if headerType == 48 { //Aggregation Packets

			} else if headerType == 50 { //PACI Packets

			} else { // Single NALU
				/*
					+---------------+---------------+
					|0|1|2|3|4|5|6|7|0|1|2|3|4|5|6|7|
					+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
					|F|   Type    |  LayerId  | TID |
					+-------------+-----------------+
				*/
				frameType = firstByte & 0x7e
			}
			if frameType >= 16 && frameType <= 21 {
				return true
			}
			if frameType == 32 {
				// vps sps pps...
				if len(rtp.Payload) < 200 { // consider sps pps header only.
					return false
				}
				return true
			}
		}
		return false
	}
	return false
}
