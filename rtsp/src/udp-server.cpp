#include "rtsp-session.h"
#include "rtsp-client.h"
#include "fmt.h"
#include "udp-server.h"
#include "utils.h"

void UDPServer::AddInputBytes(int bytes )
{
	if (this->session != nullptr ){
		this->session->InBytes += bytes;
		return ;
	}
	if (this->rtspClient != nullptr) {
            this->rtspClient->InBytes += bytes)
		return ;
	}
	//panic(fmt.Errorf("session and RTSPClient both nullptr"));
}

void UDPServer::HandleRTP(RTPPack* pack)
{
	if (this->session != nullptr) {
		for( const auto& v : this->session->RTPHandles ){
			v(pack);
		}
		return ;
	}

	if (this->rtspClient != nullptr ){
        for( const auto& v : this->rtspClient->RTPHandles ){
            v(pack);
        }
		return ;
	}
	//panic(fmt.Errorf("session and RTSPClient both nullptr"))
}

//func (s *UDPServer) Logger() *log.Logger {
//	if s.Session != nullptr {
//		return s.Session.logger
//	}
//	if s.RTSPClient != nullptr {
//		return s.RTSPClient.logger
//	}
//	panic(fmt.Errorf("session and RTSPClient both nullptr"))
//}

void UDPServer::Stop()
{
	if (this->Stoped) {
		return ;
	}
	this->Stoped = true;
//	if (this->AConn != nullptr) {
//        this->AConn.Close();
//        this->AConn = nullptr;
//	}
//	if (this->AControlConn != nullptr) {
//        this->AControlConn.Close()
//        this->AControlConn = nullptr
//	}
//	if (this->VConn != nullptr) {
//        this->VConn.Close()
//        this->VConn = nullptr
//	}
//	if (this->VControlConn != nullptr) {
//        this->VControlConn.Close()
//        this->VControlConn = nullptr
//	}
}

Result<void> UDPServer::SetupAudio()
{
    {
        auto usocket_wrap = UdpSocket::Bind("0.0.0.0", 0);
        if (usocket_wrap.is_err()) {
            return Err(usocket_wrap.unwrap_err());
        }
        auto usocket = usocket_wrap.unwrap();
        auto networkBuffer = utils::Conf().Section("rtsp").Key("network_buffer").MustInt(1048576);

        if (!usocket.SetSoRcvbuf(networkBuffer)) {
            logger::Printf("udp server audio conn set read buffer error,");
        }
        if (!usocket.SetSoSendbuf(networkBuffer)) {
            logger::Printf("udp server audio conn set write buffer error, ");
        }

        auto port_wrap = usocket.local_port();
        if (port_wrap.is_err()) {
            return Err(port_wrap.unwrap_err());
        }

        this->APort = port_wrap.unwrap();
        this->AConn = Some(usocket);

        thread([&]() {
            //bufUDP := make([]byte, UDP_BUF_SIZE)
            Space<char> bufUDP = Space<char>::Create(UDP_BUF_SIZE);
            if (bufUDP.no_legal()) {
                return;
            }

            logger::Printf("udp server start listen audio port{%d}", this->APort);
            //defer logger.Printf("udp server stop listen audio port[%d]", s.APort)
            auto timer = chrono::system_clock::now();
            while (!this->Stoped) {
                int n = 0;
                auto addr_wrap = this->AConn.ReadFromUDP(bufUDP, n);

                if (addr_wrap.is_ok() && n > 0) {
                    auto elapsed = chrono::system_clock::now() - timer;
                    if (elapsed >= chrono::seconds(30)) {
                        logger::Printf("Package recv from AConn.len:{%d}\n", n);
                        timer = chrono::system_clock::now();
                    }
                    char *rtpBytes = new(std::nothrow) char[n];
                    if (rtpBytes == nullptr) {
                        break;
                    }
                    Slice<char> rtp_bytes(rtpBytes, n);
                    this->AddInputBytes(n);

                    for (std::size_t i = 0; i < n; i++) {
                        rtp_bytes.set(i, bufUDP[i]);
                    }

                    RTPPack *pack = new(std::nothrow) RTPPack;
                    pack->Type = RTP_TYPE_AUDIO;
                    pack->Buffer = rtp_bytes;

                    this->HandleRTP(pack);
                } else {
                    logger::Printf("udp server read audio pack error {}", addr_wrap.unwrap_err());
                    continue;
                }
            }

            logger::Printf("udp server stop listen audio port{%d}", this->APort);
        }).detach();
    }

    {
        auto usocket_wrap = UdpSocket::Bind("0.0.0.0", 0);
        if (usocket_wrap.is_err()) {
            return Err(usocket_wrap.unwrap_err());
        }
        auto usocket = usocket_wrap.unwrap();
        auto networkBuffer = utils::Conf().Section("rtsp").Key("network_buffer").MustInt(1048576);

        if (!usocket.SetSoRcvbuf(networkBuffer)) {
            logger::Printf("udp server audio conn set read buffer error,");
        }
        if (!usocket.SetSoSendbuf(networkBuffer)) {
            logger::Printf("udp server audio conn set write buffer error, ");
        }

        auto port_wrap = usocket.local_port();
        if (port_wrap.is_err()) {
            return Err(port_wrap.unwrap_err());
        }
        this->AControlPort = port_wrap.unwrap();

        thread([&]() {
            Space<char> bufUDP = Space<char>::Create(UDP_BUF_SIZE);
            if (bufUDP.no_legal()) {
                return;
            }
            logger::Printf("udp server start listen audio control port{%d}", this->AControlPort);
            //defer logger::Printf("udp server stop listen audio control port[%d]", s.AControlPort);
            while(!this->Stoped) {
                int n = 0;
                auto addr_wrap = this->AConn.ReadFromUDP(bufUDP, n);

                if (addr_wrap.is_ok() && n > 0) {
                    //logger.Printf("Package recv from AControlConn.len:%d\n", n)
                    char *rtpBytes = new(std::nothrow) char[n];
                    if (rtpBytes == nullptr) {
                        break;
                    }
                    Slice<char> rtp_bytes(rtpBytes, n);
                    this->AddInputBytes(n);

                    for (std::size_t i = 0; i < n; i++) {
                        rtp_bytes.set( i, bufUDP[i]);
                    }
                    RTPPack *pack = new(std::nothrow) RTPPack;
                    pack->Type = RTP_TYPE_AUDIO;
                    pack->Buffer = rtp_bytes;

                    this->HandleRTP(pack);
                }
                else {
                    logger::Printf("udp server read audio control pack error", addr_wrap.unwrap_err());
                    continue;
                }
            }
        }).detach();
    }
	return Ok();
}

Result<void> UDPServer::SetupVideo()
{
    auto usocket_wrap = UdpSocket::Bind("0.0.0.0", 0);
    if (usocket_wrap.is_err()) {
        return Err(usocket_wrap.unwrap_err());
    }
    auto usocket = usocket_wrap.unwrap();
    auto networkBuffer = utils::Conf().Section("rtsp").Key("network_buffer").MustInt(1048576);

    if (!usocket.SetSoRcvbuf(networkBuffer)) {
        logger::Printf("udp server video conn set read buffer error,");
    }
    if (!usocket.SetSoSendbuf(networkBuffer)) {
        logger::Printf("udp server video conn set write buffer error, ");
    }

    auto port_wrap = usocket.local_port();
    if (port_wrap.is_err()) {
        return Err(port_wrap.unwrap_err());
    }
    this->VPort = port_wrap.unwrap();

    thread([&]() {
        Space<char> bufUDP = Space<char>::Create(UDP_BUF_SIZE);
        if (bufUDP.no_legal()) {
            return;
        }
        logger::Printf("udp server start listen audio  port{%d}", this->AControlPort);
        //defer logger::Printf("udp server stop listen audio control port[%d]", s.AControlPort);
        auto timer = chrono::system_clock::now();

        while(!this->Stoped) {
            int n = 0;
            auto addr_wrap = this->AConn.ReadFromUDP(bufUDP, n);
            if (addr_wrap.is_ok() && n > 0) {
				auto elapsed =  chrono::system_clock::now() - timer;
				if (elapsed >= chrono::seconds(30) ){
					logger::Printf("Package recv from VConn.len:{:d}\n", n);
					timer = chrono::system_clock::now();
				}
                char *rtpBytes = new(std::nothrow) char[n];
                if (rtpBytes == nullptr) {
                    break;
                }
                Slice<char> rtp_bytes(rtpBytes, n);
				this->AddInputBytes(n);

                for (std::size_t i = 0; i < n; i++) {
                    rtp_bytes.set( i, bufUDP[i]);
                }
                RTPPack *pack = new(std::nothrow) RTPPack;
                pack->Type =   RTP_TYPE_VIDEO;
                pack->Buffer = rtp_bytes;

				this->HandleRTP(pack);
			}
            else {
				logger::Printf("udp server read video pack error", addr_wrap.unwrap_err());
				continue;
			}
		}
	}).detach();

    {
        auto usocket_wrap = UdpSocket::Bind("0.0.0.0", 0);
        if (usocket_wrap.is_err()) {
            return Err(usocket_wrap.unwrap_err());
        }
        auto usocket = usocket_wrap.unwrap();
        auto networkBuffer = utils::Conf().Section("rtsp").Key("network_buffer").MustInt(1048576);

        if (!usocket.SetSoRcvbuf(networkBuffer)) {
            logger::Printf("udp server video control conn set read buffer error, ");
        }
        if (!usocket.SetSoSendbuf(networkBuffer)) {
            logger::Printf("udp server video control conn set write buffer error, ");
        }

        auto port_wrap = usocket.local_port();
        if (port_wrap.is_err()) {
            return Err(port_wrap.unwrap_err());
        }
        this->VControlPort = port_wrap.unwrap();

        thread( [&](){
            Space<char> bufUDP = Space<char>::Create(UDP_BUF_SIZE);
            if (bufUDP.no_legal()) {
                return;
            }
            logger::Printf("udp server start listen video control port{:d}", this->VControlPort);

            while(!this->Stoped) {
                int n = 0;
                auto addr_wrap = this->AConn.ReadFromUDP(bufUDP, n);
                if (addr_wrap.is_ok() && n > 0) {
                    //logger.Printf("Package recv from VControlConn.len:%d\n", n)
                    char *rtpBytes = new(std::nothrow) char[n];
                    if (rtpBytes == nullptr) {
                        break;
                    }
                    Slice<char> rtp_bytes(rtpBytes, n);

                    this->AddInputBytes(n);

                    for (std::size_t i = 0; i < n; i++) {
                        rtp_bytes.set( i, bufUDP[i]);
                    }
                    RTPPack *pack = new(std::nothrow) RTPPack;
                    pack->Type =   RTP_TYPE_VIDEOCONTROL;
                    pack->Buffer = rtpBytes;

                    this->HandleRTP(pack);
                }
                else {
                    logger::Printf("udp server read video control pack error", addr_wrap.unwrap_err());
                    continue;
                }
            }


            logger::Printf("udp server stop listen video control port{:d}", this->VControlPort);
        }).detach();
    }
	return Ok();
}



