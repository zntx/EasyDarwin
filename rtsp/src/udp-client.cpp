//
// Created by zhangyuzhu8 on 2025/1/18.
//

#include "udp-client.h"
#include "rtsp-session.h"
#include "fmt/include/fmt/core.h"

void UDPClient::Stop()
{
    if (this->Stoped) {
        return ;
    }
    this->Stoped = true;
 //   if (!this->AConn.is_empty() ){
//        this->AConn.unwrap();
//        //this->AConn = nil;
//    }
//    if (!this->AControlConn.is_empty()) {
//        this->AControlConn.unwrap();
//        //this->AControlConn = nil;
//    }
//    if (!this->VConn.is_empty() ){
//        this->VConn.unwrap();
//        //this->VConn = nil;
//    }
//    if (!this->VControlConn.is_empty()) {
//        this->VControlConn.unwrap();
//        //this->VControlConn = nil;
//    }
}

RtspErr UDPClient::SetupAudio()
{
    //auto host = this->session->Conn.get().peer_addr();
    auto host_wrap = this->session->Conn.peer();
    if( host_wrap.is_err())
    {
        return Err(host_wrap.unwrap_err());
    }

    auto host = host_wrap.unwrap();

    host.set_port(this->APort);
    Result<UdpSocket> _con = UdpSocket::Connect( host , 0);
    if( _con.is_err()){
        return Err(_con.unwrap_err());
    }
    this->AConn = Some(_con.unwrap());

    host.set_port(this->AControlPort);
    Result<UdpSocket> __con = UdpSocket::Connect( host , 0);
    if( __con.is_err()){
        return Err(__con.unwrap_err());
    }
    this->AControlConn = Some(__con.unwrap());

    return Ok();
}

RtspErr UDPClient::SetupVideo()
{
    auto host_wrap = this->session->Conn.peer();
    if( host_wrap.is_err())
    {
        return Err(host_wrap.unwrap_err());
    }

    auto host = host_wrap.unwrap();

    host.set_port(this->VPort);
    Result<UdpSocket> _con = UdpSocket::Connect( host , 0);
    if( _con.is_err()){
        return Err(_con.unwrap_err());
    }
    this->VConn = Some(_con.unwrap());

    host.set_port(this->VControlPort);
    Result<UdpSocket> __con = UdpSocket::Connect( host , 0);
    if( __con.is_err()){
        return Err(__con.unwrap_err());
    }
    this->VControlConn = Some(__con.unwrap());

    return Ok();
}

RtspErr UDPClient::SendRTP( RTPPack* pack)
{
    if (pack == nullptr ){
        logger::error("udp client send rtp got nil pack");
        return Err(std::string("udp client send rtp got nil pack"));
    }
    Option<UdpSocket>* conn = nullptr ;
    switch (pack->Type) {
    case RTP_TYPE_AUDIO:
        conn = &this->AConn;
        break;
    case RTP_TYPE_AUDIOCONTROL:
        conn = &this->AControlConn;
        break;
    case RTP_TYPE_VIDEO:
        conn = &this->VConn;
        break;
    case RTP_TYPE_VIDEOCONTROL:
        conn = &this->VControlConn;
        break;
    default:
        logger::error("udp client send rtp got unkown pack type{}", pack->Type);
        auto message = fmt::format("udp client send rtp got unkown pack type{:d}", (int)pack->Type);
        return Err(message);
    }
    if (conn == nullptr || conn->is_empty()) {
        logger::error("udp client send rtp pack type{} failed, conn not found", pack->Type);
        auto message = fmt::format("\"udp client send rtp pack type{:d} failed, conn not found", (int)pack->Type);
        return Err(message);
    }
//    var n int
//    if n, err = conn.Write(pack.Buffer.Bytes()); err != nil {
//        logger::error("udp client write bytes error, %v", err);
//        return
//    }

    auto conn_ = conn->get();
    size_t n = conn_.write(pack->Buffer);


    // logger.Printf("udp client write [%d/%d]", n, pack.Buffer.Len())
    this->session->OutBytes += n;

    return Ok();
}
