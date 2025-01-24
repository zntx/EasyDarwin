//
// Created by zhangyuzhu8 on 2025/1/20.
//
#include "rtsp-session.h"
#include "rtsp-server.h"
#include "rtsp-request.h"
#include "rtsp-response.h"
#include "udp-client.h"
#include "url.h"
#include "net.h"
#include "player.h"
#include "pusher.h"
#include "ini.hpp"
#include "fmt.h"


Session* NewSession(Server* server, TcpStream conn )
{
    inicpp::IniManager ini("easydarwin");

    auto networkBuffer = ini.Section("rtsp").Key("network_buffer").MustInt(204800);
    auto timeoutMillis = ini.Section("rtsp").Key("timeout").MustInt(0);
    //auto timeoutTCPConn = &RichConn{conn, time.Duration(timeoutMillis) * time.Millisecond}
    auto authorizationEnable = ini.Section("rtsp").Key("authorization_enable").MustInt(0);
    auto close_old = ini.Section("rtsp").Key("close_old").MustInt(0);
    auto debugLogEnable = ini.Section("rtsp").Key("debug_log_enable").MustInt(0);

    auto session = new(std::nothrow) Session(conn);
    if( !session) {
        return nullptr;
    }

//    session->ID =                 shortid.MustGenerate();
    session->Server=              server;
//    session->Conn=                timeoutTCPConn,
//    session->connRW=              bufio.NewReadWriter(bufio.NewReaderSize(timeoutTCPConn, networkBuffer), bufio.NewWriterSize(timeoutTCPConn, networkBuffer)),
//    session->StartAt=             time.Now();
    session->Timeout=             ini.Section("rtsp").Key("timeout").MustInt(0);
    session->authorizationEnable= authorizationEnable != 0;
    session->debugLogEnable=      debugLogEnable != 0;
//    session->RTPHandles=          make([]func(*RTPPack), 0),
//    session->StopHandles=         make([]func(), 0),
    session->vRTPChannel=         -1;
    session->vRTPControlChannel=  -1;
    session->aRTPChannel=         -1;
    session->aRTPControlChannel=  -1;
    session->closeOld=            close_old != 0;


    //session.logger = log.New(os.Stdout, fmt.Sprintf("[%s]", session.ID), log.LstdFlags|log.Lshortfile)
//    if (!utils.Debug) {
//        logger::SetOutput(utils.GetLogWriter());
//    }
    return session ;
}

Session::Session( TcpStream& conn) :Conn(std::move(conn) )
{

}

string Session::String()
{
	return fmt::format("session{}{}[{}][{}][{}]", this->Type, this->TransType, this->Path, this->ID,
                       this->Conn.peer().unwrap().to_string());
}

void Session::Stop()
{
    if (this->Stoped ){
        return ;
    }
    this->Stoped = true;

    //for _, h := range session.StopHandles {
    for(  auto &&h : this->StopHandles) {
        h() ;
    }
//    if (this->Conn != nullptr) {
//        this->connRW.Flush();
//        this->Conn.Close();
//        this->Conn = nullptr;
//    }
    if (this->UDPClient != nullptr) {
        this->UDPClient->Stop();
        this->UDPClient = nullptr;
    }
}

void Session::Start()
{
    //defer session.Stop()
//    buf1 := make([]byte, 1)
//    buf2 := make([]byte, 2)
//    logger := session.logger
//    timer := time.Unix(0, 0)

    char buf[4] = {0};
    Slice<char> buf1(&buf[0], 1);
    Slice<char> buf2(&buf[1], 2);

    while( !this->Stoped) {
        auto res = this->Conn.read(buf1);
        if( res == 0) {
//            if _, err := io.ReadFull(session.connRW, buf1); err != nullptr {
//                    logger.Println(session, err)
//                    return
//            }
            continue;
        }

        if (buf1[0] == 0x24) { //rtp data
            res = this->Conn.read(buf1);
            if( res == 0) {
                continue;
            }
            res = this->Conn.read(buf2);
            if( res == 0) {
                continue;
            }
            auto channel = int(buf1[0]);
            size_t rtpLen = u16::from_be_bytes( buf2[0], buf2[1]).value;
            auto _rtpBytes = new(std::nothrow) char[rtpLen];
            Slice<char> rtpBytes(_rtpBytes, rtpLen);

            res = this->Conn.read(rtpBytes);
            if( res == 0)
            {
                logger::info(err);
                return;
            }
            //rtpBuf := bytes.NewBuffer(rtpBytes)
            RTPPack pack = {0, Slice<char>(nullptr, 0)};
            pack.Buffer = rtpBytes;

            if( this->aRTPChannel == channel) {
                pack.Type = RTP_TYPE_AUDIO;

//                elapsed := time.Now().Sub(timer)
//                if elapsed >= 30*time.Second {
//                    logger.Println("Recv an audio RTP package")
//                    timer = time.Now()
//                }
                break;
            }
            else if (this->aRTPControlChannel == channel) {
                pack.Type = RTP_TYPE_AUDIOCONTROL;
            }
            else if ( this->vRTPChannel == channel) {
                pack.Type = RTP_TYPE_VIDEO;
                //elapsed := time.Now().Sub(timer)
//                if elapsed >= 30*time.Second {
//                logger.Println("Recv an video RTP package")
//                timer = time.Now()
//                }
            }
            else if ( this->vRTPControlChannel == channel) {
                pack.Type = RTP_TYPE_VIDEOCONTROL;
            }
            else {
                logger::warn("unknow rtp pack type, {}", channel);
                continue;
            }
            this->InBytes += rtpLen + 4;
            for (auto &&h :this->RTPHandles ){
                h(&pack);
            }
        }
        else { // rtsp cmd
            reqBuf := bytes.NewBuffer(nullptr)
            reqBuf.Write(buf1)
            while( !this->Stoped) {
                if line, isPrefix, err := this->connRW.ReadLine(); err != nullptr {
                    logger::Println(err);
                    return
                }
                else {
                    reqBuf.Write(line)
                    if !isPrefix {
                        reqBuf.WriteString("\r\n")
                    }
                    if len(line) == 0 {
                        req := NewRequest(reqBuf.String())
                        if req == nullptr {
                            break
                        }
                        this->InBytes += reqBuf.Len()
                        contentLen := req.GetContentLength()
                        this->InBytes += contentLen
                        if contentLen > 0 {
                            bodyBuf := make([]byte, contentLen)
                            if n, err := io.ReadFull(this->connRW, bodyBuf); err != nullptr {
                                    logger::Println(err)
                                    return
                            } else if n != contentLen {
                                        logger::Printf("read rtsp request body failed, expect size[%d], got size[%d]", contentLen, n)
                                        return
                                }
                            req.Body = string(bodyBuf)
                        }
                        this->handleRequest(req)
                        break
                    }
                }
            }
        }
    }
}


#if 0
RtspErr CheckAuth(string authLine , string method , string sessionNonce )
{
	realmRex := regexp.MustCompile(`realm="(.*?)"`)
	nonceRex := regexp.MustCompile(`nonce="(.*?)"`)
	usernameRex := regexp.MustCompile(`username="(.*?)"`)
	responseRex := regexp.MustCompile(`response="(.*?)"`)
	uriRex := regexp.MustCompile(`uri="(.*?)"`)

	realm := ""
	nonce := ""
	username := ""
	response := ""
	uri := ""
	result1 := realmRex.FindStringSubmatch(authLine)
	if len(result1) == 2 {
		realm = result1[1]
	} else {
		return fmt.Errorf("CheckAuth error : no realm found")
	}
	result1 = nonceRex.FindStringSubmatch(authLine)
	if len(result1) == 2 {
		nonce = result1[1]
	} else {
		return fmt.Errorf("CheckAuth error : no nonce found")
	}
	if sessionNonce != nonce {
		return fmt.Errorf("CheckAuth error : sessionNonce not same as nonce")
	}

	result1 = usernameRex.FindStringSubmatch(authLine)
	if len(result1) == 2 {
		username = result1[1]
	} else {
		return fmt.Errorf("CheckAuth error : username not found")
	}

	result1 = responseRex.FindStringSubmatch(authLine)
	if len(result1) == 2 {
		response = result1[1]
	} else {
		return fmt.Errorf("CheckAuth error : response not found")
	}

	result1 = uriRex.FindStringSubmatch(authLine)
	if len(result1) == 2 {
		uri = result1[1]
	} else {
		return fmt.Errorf("CheckAuth error : uri not found")
	}
	var user models.User
	err := db.SQLite.Where("Username = ?", username).First(&user).Error
	if err != nullptr {
		return fmt.Errorf("CheckAuth error : user not exists")
	}
	md5UserRealmPwd := fmt.Sprintf("%x", md5.Sum([]byte(fmt.Sprintf("%s:%s:%s", username, realm, user.Password))))
	md5MethodURL := fmt.Sprintf("%x", md5.Sum([]byte(fmt.Sprintf("%s:%s", method, uri))))
	myResponse := fmt.Sprintf("%x", md5.Sum([]byte(fmt.Sprintf("%s:%s:%s", md5UserRealmPwd, nonce, md5MethodURL))))
	if myResponse != response {
		return fmt.Errorf("CheckAuth error : response not equal")
	}
	return nullptr
}
#endif
void Session::handleRequest(Request *req ) 
{
	//if this->Timeout > 0 {
	//	this->Conn.SetDeadline(time.Now().Add(time.Duration(this->Timeout) * time.Second))
	//}
	//logger := this->logger
	logger::debug("<<<\n {}", req);

	auto res = NewResponse(200, "OK", req->Header["CSeq"], this->ID, "");
#if 0    
	defer func() {
		if p := recover(); p != nullptr {
			logger.Printf("handleRequest err ocurs:%v", p)
			res.StatusCode = 500
			res.Status = fmt.Sprintf("Inner Server Error, %v", p)
		}
		logger.Printf(">>>\n%s", res)
		outBytes := []byte(res.String())
		this->connWLock.Lock()
		this->connRW.Write(outBytes)
		this->connRW.Flush()
		this->connWLock.Unlock()
		this->OutBytes += len(outBytes)
		switch req.Method {
		case "PLAY", "RECORD":
			switch this->Type {
			case SESSEION_TYPE_PLAYER:
				if this->Pusher.HasPlayer(this->Player) {
					this->Player.Pause(false)
				} else {
					this->Pusher.AddPlayer(this->Player)
				}
				// case SESSION_TYPE_PUSHER:
				// 	this->Server.AddPusher(this->Pusher)
			}
		case "TEARDOWN":
			{
				this->Stop()
				return
			}
		}
		if res.StatusCode != 200 && res.StatusCode != 401 {
			logger.Printf("Response request error[%d]. stop session.", res.StatusCode)
			this->Stop()
		}
	}()
#endif
	if (req->Method != "OPTIONS") {
		if (this->authorizationEnable) {
			auto authLine = req->Header["Authorization"];
			auto authFailed = true;
			if (authLine != "" ){
				err := CheckAuth(authLine, req->Method, this->nonce)
				if (err == nullptr) {
					authFailed = false;
				}
                else {
					logger::info("%v", err);
				}
			}
			if (authFailed ){
				res->StatusCode = 401;
				res->Status = "Unauthorized";
				auto nonce = fmt::format("{:x}", md5.Sum([]byte(shortid.MustGenerate())));
				this->nonce = nonce;
				res->Header["WWW-Authenticate"] = fmt::format(`Digest realm="EasyDarwin", nonce="%s", algorithm="MD5"`, nonce);
				return ;
			}
		}
	}
 
	if( req->Method == "OPTIONS" ) {
		res->Header["Public"] = "DESCRIBE, SETUP, TEARDOWN, PLAY, PAUSE, OPTIONS, ANNOUNCE, RECORD";
    }
	else if( req->Method == "ANNOUNCE" ) {
		this->Type = SessionType::SESSION_TYPE_PUSHER;
		this->URL = req->URL;

		url, err := url.Parse(req.URL)
		if err != nullptr {
			res->StatusCode = 500;
			res->Status = "Invalid URL";
			return ;
		}
		this->Path = url.Path ;

		this->SDPRaw = req->Body ;
		this->SDPMap = ParseSDP(req->Body);
		sdp, ok :=this->SDPMap["audio"];
		if (ok ){
			this->AControl = sdp.Control;
			this->ACodec = sdp.Codec;
			logger.Printf("audio codec[%s]\n", this->ACodec);
		}
		sdp, ok = this->SDPMap["video"];
		if (ok ){
			this->VControl = sdp.Control;
			this->VCodec = sdp.Codec;
			logger.Printf("video codec[%s]\n", this->VCodec);
		}
		auto addPusher = false;
		if (this->closeOld) {
			r, _ := this->Server.TryAttachToPusher(session);
			if( r < -1 ){
				logger.Printf("reject pusher.");
				res.StatusCode = 406;
				res.Status = "Not Acceptable";
			}
            else if( r == 0 ){
				addPusher = true;
			}
            else {
				logger.Printf("Attached to old pusher");
				// 尝试发给客户端ANNOUCE
				// players := pusher.GetPlayers()
				// for _, v := range players {
				// 	sess := v.Session

				// 	hearers := make(map[string]string)
				// 	hearers["Content-Type"] = "application/sdp"
				// 	hearers["Session"] = sess.ID
				// 	hearers["Content-Length"] = strconv.Itoa(len(v.SDPRaw))
				// 	var req = Request{Method: ANNOUNCE, URL: v.URL, Version: "1.0", Header: hearers, Body: pusher.SDPRaw()}
				// 	sess.connWLock.Lock()
				// 	logger.Println(req.String())
				// 	outBytes := []byte(req.String())
				// 	sess.connRW.Write(outBytes)
				// 	sess.connRW.Flush()
				// 	sess.connWLock.Unlock()
				// }
			}
		}
        else {
			addPusher = true;
		}
		if( addPusher) {
			this->Pusher = NewPusher(session);
			addedToServer := this->Server.AddPusher(this->Pusher);
			if (!addedToServer) {
				logger.Printf("reject pusher.");
				res->StatusCode = 406;
				res->Status = "Not Acceptable";
			}
		}
    }
	else if( req->Method == "DESCRIBE"  ) {
		this->Type = SessionType::SESSEION_TYPE_PLAYER;
		this->URL = req->URL;

		auto url_err = Url::Parse(req->URL);
		if ( url_err.is_err()) {
			res->StatusCode = 500;
			res ->Status = "Invalid URL";
			return ;
		}
		this->Path = url_err.unwrap().Path();
		auto pusher = this->Server->GetPusher(this->ath);
		if (pusher == nullptr) {
			res->StatusCode = 404;
			res->Status = "NOT FOUND";
			return ;
		}
		this->Player = NewPlayer(this, pusher);
		this->Pusher = pusher;
		this->AControl = pusher.AControl();
		this->VControl = pusher.VControl();
		this->ACodec = pusher.ACodec();
		this->VCodec = pusher.VCodec();
		this->Conn.timeout = 0;
		res->SetBody(this->Pusher->SDPRaw());
    }
	else if( req->Method == "SETUP"  ) {
		ts := req->Header["Transport"];
		// control字段可能是`stream=1`字样，也可能是rtsp://...字样。即control可能是url的path，也可能是整个url
		// 例1：
		// a=control:streamid=1
		// 例2：
		// a=control:rtsp://192.168.1.64/trackID=1
		// 例3：
		// a=control:?ctype=video
		auto setupUrl_err = Url::Parse(req->URL);
		if ( setupUrl_err.is_err()){
			res->StatusCode = 500;
			res->Status = "Invalid URL";
			return ;
		}
        auto setupUrl = setupUrl_err.unwrap();
		if (setupUrl.Port() == 0) {
			setupUrl.setHost(fmt::format("{}:554", setupUrl.Hostname()));
		}
		auto setupPath = setupUrl.String();

		// error status. SETUP without ANNOUNCE or DESCRIBE.
		if (this->Pusher == nullptr) {
			res->StatusCode = 500;
			res->Status = "Error Status";
			return;
		}
		//setupPath = setupPath[strings.LastIndex(setupPath, "/")+1:]
		string vPath  ;
		if (strings.Index(strings.ToLower(this->VControl), "rtsp://") == 0 ){
			auto vControlUrl_err = Url::Parse(this->VControl);
			if (vControlUrl_err.is_err()){
				res->StatusCode = 500;
				res->Status = "Invalid VControl";
				return;
			}
            auto vControlUrl =vControlUrl_err.unwrap();
			if (vControlUrl.Port() == 0 ){
				vControlUrl.setHost(fmt::format("{}:554", vControlUrl.Hostname()));
			}
			vPath = vControlUrl.String();
		}
        else {
			vPath = this->VControl;
		}

        string aPath;
		if (strings.Index(strings.ToLower(this->AControl), "rtsp://") == 0 ){
			auto aControlUrl_err = Url::Parse(this->AControl);
			if( aControlUrl_err.is_err()){
				res->StatusCode = 500;
				res->Status = "Invalid AControl";
				return ;
			}
            auto aControlUrl = aControlUrl_err.unwrap();
			if (aControlUrl.Port() ==  0) {
				aControlUrl.setHost( fmt::format("{}:554", aControlUrl.Hostname()));
			}
			aPath = aControlUrl.String();
		}
        else {
			aPath = this->AControl;
		}

		mtcp := regexp.MustCompile("interleaved=(\\d+)(-(\\d+))?");
		mudp := regexp.MustCompile("client_port=(\\d+)(-(\\d+))?");

		if (tcpMatchs := mtcp.FindStringSubmatch(ts); tcpMatchs != nullptr) {
			this->ransType = TRANS_TYPE_TCP;
			if (setupPath == aPath || aPath != "" && strings.LastIndex(setupPath, aPath) == len(setupPath)-len(aPath)) {
				this->aRTPChannel, _ = strconv.Atoi(tcpMatchs[1]);
				this->aRTPControlChannel, _ = strconv.Atoi(tcpMatchs[3]);
			}
            else (if setupPath == vPath || vPath != "" && strings.LastIndex(setupPath, vPath) == len(setupPath)-len(vPath)) {
				this->vRTPChannel, _ = strconv.Atoi(tcpMatchs[1]);
				this->vRTPControlChannel, _ = strconv.Atoi(tcpMatchs[3]);
			}
            else {
				res->StatusCode = 500;
				res->Status = fmt::format("SETUP [TCP] got UnKown control:{}", setupPath);
				logger::Printf("SETUP [TCP] got UnKown control:{}", setupPath);
			}
			logger.Printf("Parse SETUP req.TRANSPORT:TCP.Session.Type:{:d},control:{}, AControl:{},VControl:{}", this->Type, setupPath, aPath, vPath);
		}
        else (if udpMatchs := mudp.FindStringSubmatch(ts); udpMatchs != nullptr ){
			this->TransType = TRANS_TYPE_UDP;
			// no need for tcp timeout.
			this->Conn.timeout = 0;
			if (this->Type == SESSEION_TYPE_PLAYER && this->UDPClient == nullptr ){
				this->UDPClient = &UDPClient{
					Session: session,
				}
			}
			if (this->Type == SESSION_TYPE_PUSHER && this->Pusher.UDPServer == nullptr ){
				this->Pusher.UDPServer = &UDPServer{
					Session: session,
				}
			}
			logger.Printf("Parse SETUP req.TRANSPORT:UDP.Session.Type:%d,control:%s, AControl:%s,VControl:%s", this->Type, setupPath, aPath, vPath);
			if( setupPath == aPath || aPath != "" && strings.LastIndex(setupPath, aPath) == len(setupPath)-len(aPath)) {
				if (this->Type == SESSEION_TYPE_PLAYER) {
					this->UDPClient.APort, _ = strconv.Atoi(udpMatchs[1]);
					this->UDPClient.AControlPort, _ = strconv.Atoi(udpMatchs[3]);
					if (err := this->UDPClient.SetupAudio(); err != nullptr ){
						res.StatusCode = 500;
						res.Status = fmt.Sprintf("udp client setup audio error, %v", err);
						return;
					}
				}
				if (this->Type == SESSION_TYPE_PUSHER ){
					if (err := this->Pusher.UDPServer.SetupAudio(); err != nullptr) {
						res.StatusCode = 500;
						res.Status = fmt.Sprintf("udp server setup audio error, %v", err);
						return;
					}
					tss := strings.Split(ts, ";");
					idx := -1;
					for( i, val := range tss ){
						if val == udpMatchs[0] {
							idx = i;
						}
					}
					tail := append([]string{}, tss[idx+1:]...);
					tss = append(tss[:idx+1], fmt.Sprintf("server_port=%d-%d", this->Pusher.UDPServer.APort, this->Pusher.UDPServer.AControlPort));
					tss = append(tss, tail...);
					ts = strings.Join(tss, ";");
				}
			}
            else if (setupPath == vPath || vPath != "" && strings.LastIndex(setupPath, vPath) == len(setupPath)-len(vPath)) {
				if (this->Type == SESSEION_TYPE_PLAYER) {
					this->UDPClient.VPort, _ = strconv.Atoi(udpMatchs[1]);
					this->UDPClient.VControlPort, _ = strconv.Atoi(udpMatchs[3]);
					if (err := this->UDPClient.SetupVideo(); err != nullptr) {
						res.StatusCode = 500;
						res.Status = fmt.Sprintf("udp client setup video error, %v", err);
						return;
					}
				}

				if (this->Type == SESSION_TYPE_PUSHER ){
					if( err := this->Pusher.UDPServer.SetupVideo(); err != nullptr) {
						res.StatusCode = 500;
						res.Status = fmt.Sprintf("udp server setup video error, %v", err);
						return;
					}
					tss := strings.Split(ts, ";")
					idx := -1
					for( i, val := range tss) {
						if val == udpMatchs[0] {
							idx = i;
						}
					}
					tail := append([]string{}, tss[idx+1:]...);
					tss = append(tss[:idx+1], fmt.Sprintf("server_port=%d-%d", this->Pusher.UDPServer.VPort, this->Pusher.UDPServer.VControlPort));
					tss = append(tss, tail...);
					ts = strings.Join(tss, ";");
				}
			}
            else {
				logger.Printf("SETUP [UDP] got UnKown control:%s", setupPath);
			}
		}
		res.Header["Transport"] = ts;;
    }
	else if( req->Method == "PLAY"  ) {
		// error status. PLAY without ANNOUNCE or DESCRIBE.
		if (this->Pusher == nullptr) {
			res->StatusCode = 500;
			res->Status = "Error Status";
			return ;
		}
		res->Header["Range"] = req->Header["Range"];
    }
	else if( req->Method == "RECORD"  ) {
		// error status. RECORD without ANNOUNCE or DESCRIBE.
		if (this->Pusher == nullptr ){
			res->StatusCode = 500;
			res->Status = "Error Status";
			return ;
		}

    }
	else if( req->Method == "PAUSE"  ) {
		if (this->Player == nullptr) {
			res->StatusCode = 500;
			res->Status = "Error Status";
			return ;
		}
		this->Player->Pause(true) ;
	}
}

Result<void> Session::SendRTP(RTPPack* pack)
{
	if( pack == nullptr) {
		return Err(fmt::format("player send rtp got nullptr pack"));
	}
	if (this->TransType == TransType::TRANS_TYPE_UDP) {
		if (this->UDPClient == nullptr) {
			return Err(fmt::format("player use udp transport but udp client not found"));
		}
		return this->UDPClient->SendRTP(pack);
	}
	switch (pack->Type) {
	case RTP_TYPE_AUDIO:
		bufChannel := make([]byte, 2)
		bufChannel[0] = 0x24
		bufChannel[1] = byte(this->aRTPChannel)
		this->connWLock.Lock()
		this->connRW.Write(bufChannel)
		bufLen := make([]byte, 2)
		binary.BigEndian.PutUint16(bufLen, uint16(pack.Buffer.Len()))
		this->connRW.Write(bufLen)
		this->connRW.Write(pack.Buffer.Bytes())
		this->connRW.Flush()
		this->connWLock.Unlock()
		this->OutBytes += pack.Buffer.Len() + 4;
        break;

	case RTP_TYPE_AUDIOCONTROL:
		bufChannel := make([]byte, 2)
		bufChannel[0] = 0x24
		bufChannel[1] = byte(this->aRTPControlChannel)
		this->connWLock.Lock()
		this->connRW.Write(bufChannel)
		bufLen := make([]byte, 2)
		binary.BigEndian.PutUint16(bufLen, uint16(pack.Buffer.Len()))
		this->connRW.Write(bufLen)
		this->connRW.Write(pack.Buffer.Bytes())
		this->connRW.Flush()
		this->connWLock.Unlock()
		this->OutBytes += pack.Buffer.Len() + 4;
        break;

	case RTP_TYPE_VIDEO:
		bufChannel := make([]byte, 2)
		bufChannel[0] = 0x24
		bufChannel[1] = byte(this->vRTPChannel)
		this->connWLock.Lock()
		this->connRW.Write(bufChannel)
		bufLen := make([]byte, 2)
		binary.BigEndian.PutUint16(bufLen, uint16(pack.Buffer.Len()))
		this->connRW.Write(bufLen)
		this->connRW.Write(pack.Buffer.Bytes())
		this->connRW.Flush()
		this->connWLock.Unlock()
		this->OutBytes += pack.Buffer.Len() + 4;
        break;

	case RTP_TYPE_VIDEOCONTROL:
		bufChannel := make([]byte, 2)
		bufChannel[0] = 0x24
		bufChannel[1] = byte(this->vRTPControlChannel)
		this->connWLock.Lock()
		this->connRW.Write(bufChannel)
		bufLen := make([]byte, 2)
		binary.BigEndian.PutUint16(bufLen, uint16(pack.Buffer.Len()))
		this->connRW.Write(bufLen)
		this->connRW.Write(pack.Buffer.Bytes())
		this->connRW.Flush()
		this->connWLock.Unlock()
		this->OutBytes += pack.Buffer.Len() + 4;
        break;

	default:
		return Err(fmt::format("session tcp send rtp got unkown pack type {:d}", pack->Type));
	}
	return Ok();
}
