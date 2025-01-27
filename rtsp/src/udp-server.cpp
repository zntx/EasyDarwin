#include "rtsp-session.h"
#include "rtsp-client.h"
#include "fmt.h"
#include "udp-server.h"
#include "utils.h"

void UDPServer::AddInputBytes(int bytes ) const
{
	if (this->session != nullptr ){
		this->session->InBytes += bytes;
		return ;
	}
	if (this->rtspClient != nullptr) {
            this->rtspClient->InBytes += bytes;
		return ;
	}
	//panic(fmt.Errorf("session and RTSPClient both nullptr"));
}

void UDPServer::HandleRTP(RTPPack* pack) const
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


Result<void> UDPServer::Setup(int _type)
{
    auto socket_wrap = UdpSocket::Bind("0.0.0.0", 0);
    if (socket_wrap.is_err()) {
        return Err(socket_wrap.unwrap_err());
    }
    auto udp_socket = socket_wrap.unwrap();
    auto networkBuffer = utils::Conf().Section("rtsp").Key("network_buffer").MustInt(1048576);

    if (!udp_socket.SetSoRcvbuf(networkBuffer)) {
        logger::Printf("udp server audio conn set read buffer error,");
    }
    if (!udp_socket.SetSoSendbuf(networkBuffer)) {
        logger::Printf("udp server audio conn set write buffer error, ");
    }

    auto port_wrap = udp_socket.local_port();
    if (port_wrap.is_err()) {
        return Err(port_wrap.unwrap_err());
    }

    if( _type == RTP_TYPE_AUDIO) {
        this->APort = port_wrap.unwrap();
    }
    else if (_type == RTP_TYPE_AUDIOCONTROL) {
        this->AControlPort = port_wrap.unwrap();
    }
    else if (_type == RTP_TYPE_VIDEO){
        this->VPort = port_wrap.unwrap();
    }
    else if (_type == RTP_TYPE_VIDEOCONTROL) {
        this->VControlPort = port_wrap.unwrap();
    }
    else {
        logger::Printf("udp server support error, ");
        return Err(string("not support "));
    }

    thread stream([&](UdpSocket&& socket) {
        Space<char> bufUDP = Space<char>::Create(UDP_BUF_SIZE);
        if (bufUDP.no_legal()) {
            return Err(string("no memory"));
        }

        if( _type == RTP_TYPE_AUDIO) {
            logger::Printf("udp server start listen audio port{%d}", this->APort);
        }
        else if (_type == RTP_TYPE_AUDIOCONTROL) {
            logger::Printf("udp server start listen audio port{%d}", this->AControlPort);
        }
        else if (_type == RTP_TYPE_VIDEO){
            logger::Printf("udp server start listen video port{%d}", this->VPort);
        }
        else if (_type == RTP_TYPE_VIDEOCONTROL) {
            logger::Printf("udp server start listen video port{%d}", this->VControlPort);
        }
        //defer logger.Printf("udp server stop listen audio port[%d]", s.APort)
        auto timer = chrono::system_clock::now();
        while (!this->Stoped) {
            int n = 0;
            auto addr_wrap = socket.ReadFromUDP(bufUDP, n);

            if (addr_wrap.is_ok() && n > 0) {

                if (_type == RTP_TYPE_AUDIO || _type == RTP_TYPE_VIDEO) {
                    auto elapsed = chrono::system_clock::now() - timer;
                    if (elapsed >= chrono::seconds(30)) {
                        logger::Printf("Package recv from AConn.len:{:d}\n", n);
                        timer = chrono::system_clock::now();
                    }
                }

                char *rtpBytes = new(std::nothrow) char[n];
                if (rtpBytes == nullptr) {
                    break;
                }
                this->AddInputBytes(n);

                for (std::size_t i = 0; i < n; i++) {
                    *(rtpBytes + i) = bufUDP[i];
                }

                auto *pack = new(std::nothrow) RTPPack{ RTP_TYPE_AUDIO,  Slice<char>(rtpBytes, n)};

                this->HandleRTP(pack);
            }
            else {
                logger::Printf("udp server read  pack error {}", addr_wrap.unwrap_err());
                continue;
            }
        }

        if( _type == RTP_TYPE_AUDIO) {
            logger::Printf("udp server stop listen audio port{%d}", this->APort);
        }
        else if (_type == RTP_TYPE_AUDIOCONTROL) {
            logger::Printf("udp server stop listen audio port{%d}", this->AControlPort);
        }
        else if (_type == RTP_TYPE_VIDEO){
            logger::Printf("udp server stop listen video port{%d}", this->VPort);
        }
        else if (_type == RTP_TYPE_VIDEOCONTROL) {
            logger::Printf("udp server stop listen video port{%d}", this->VControlPort);
        }
    }, std::move(udp_socket) );
    stream.detach();

    return Ok();
}


Result<void> UDPServer::SetupAudio()
{
    auto result = this->Setup(RTP_TYPE_AUDIO);
    if( result.is_err())
        return result;

    auto result2 = this->Setup(RTP_TYPE_AUDIOCONTROL);
    if( result2.is_err())
        return result2;

	return Ok();
}

Result<void> UDPServer::SetupVideo()
{
    auto result = this->Setup(RTP_TYPE_VIDEO);
    if( result.is_err())
        return result;

    auto result2 = this->Setup(RTP_TYPE_VIDEOCONTROL);
    if( result2.is_err())
        return result2;

    return Ok();
}



