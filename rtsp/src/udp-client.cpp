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
    if (!this->AConn.is_empty() ){
        this->AConn.unwrap();
        //this->AConn = nil;
    }
    if (!this->AControlConn.is_empty()) {
        this->AControlConn.unwrap();
        //this->AControlConn = nil;
    }
    if (!this->VConn.is_empty() ){
        this->VConn.unwrap();
        //this->VConn = nil;
    }
    if (!this->VControlConn.is_empty()) {
        this->VControlConn.unwrap();
        //this->VControlConn = nil;
    }
}

RtspErr UDPClient::SetupAudio()
{
#if 0
    var (
            logger = c.logger
            addr   *net.UDPAddr
    )
    defer func() {
        if err != nil {
                    logger.Println(err)
                    c.Stop()
            }
    }()
#endif

    //auto host = this->session->Conn.get().peer_addr();
    auto host_wrap = this->session->Conn.peer();
    if( host_wrap.is_err())
    {
        return Err(host_wrap.unwrap_err());
    }

    auto host = host_wrap.unwrap();
    //host = host[:strings.LastIndex(host, ":")]
#if 0
    if addr, err = net.ResolveUDPAddr("udp", fmt.Sprintf("%s:%d", host, this->APort)); err != nil {
            return
    }
    this->AConn, err = net.DialUDP("udp", nil, addr)
    if err != nil {
        return
    }
    networkBuffer := utils.Conf().Section("rtsp").Key("network_buffer").MustInt(1048576)
    if err = this->AConn.SetReadBuffer(networkBuffer); err != nil {
            logger.Printf("udp client audio conn set read buffer error, %v", err)
    }
    if err = c.AConn.SetWriteBuffer(networkBuffer); err != nil {
            logger.Printf("udp client audio conn set write buffer error, %v", err)
    }
#endif
    host.set_port(this->APort);
    Result<UdpSocket> _con = UdpSocket::Connect( host , 0);
    if( _con.is_err()){
        return Err(_con.unwrap_err());
    }
    this->AConn = Some(_con.unwrap());

#if 0
    addr, err = net.ResolveUDPAddr("udp", fmt.Sprintf("%s:%d", host, c.AControlPort))
    if err != nil {
        return
    }
    c.AControlConn, err = net.DialUDP("udp", nil, addr)
    if err != nil {
        return
    }
    if err = c.AControlConn.SetReadBuffer(networkBuffer); err != nil {
        logger.Printf("udp client audio control conn set read buffer error, %v", err)
    }
    if err = c.AControlConn.SetWriteBuffer(networkBuffer); err != nil {
        logger.Printf("udp client audio control conn set write buffer error, %v", err)
    }
#endif
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
#if 0
    var (
            logger = c.logger
    addr   *net.UDPAddr
    )
    defer func() {
        if err != nil {
                    logger.Println(err)
                    c.Stop()
            }
    }()
#endif
//    host := c.Conn.RemoteAddr().String()
//    host = host[:strings.LastIndex(host, ":")]
    auto host_wrap = this->session->Conn.peer();
    if( host_wrap.is_err())
    {
        return Err(host_wrap.unwrap_err());
    }

    auto host = host_wrap.unwrap();
#if 0
    if addr, err = net.ResolveUDPAddr("udp", fmt.Sprintf("%s:%d", host, c.VPort)); err != nil {
            return
    }
    if c.VConn, err = net.DialUDP("udp", nil, addr); err != nil {
            return
    }
    networkBuffer := utils.Conf().Section("rtsp").Key("network_buffer").MustInt(1048576)
    if err = c.VConn.SetReadBuffer(networkBuffer); err != nil {
            logger.Printf("udp client video conn set read buffer error, %v", err)
    }
    if err = c.VConn.SetWriteBuffer(networkBuffer); err != nil {
            logger.Printf("udp client video conn set write buffer error, %v", err)
    }
#endif
    host.set_port(this->VPort);
    Result<UdpSocket> _con = UdpSocket::Connect( host , 0);
    if( _con.is_err()){
        return Err(_con.unwrap_err());
    }
    this->VConn = Some(_con.unwrap());
#if 0
    addr, err = net.ResolveUDPAddr("udp", fmt.Sprintf("%s:%d", host, c.VControlPort))
    if err != nil {
                return
        }
    c.VControlConn, err = net.DialUDP("udp", nil, addr)
    if err != nil {
                return
        }
    if err = c.VControlConn.SetReadBuffer(networkBuffer); err != nil {
            logger.Printf("udp client video control conn set read buffer error, %v", err)
    }
    if err = c.VControlConn.SetWriteBuffer(networkBuffer); err != nil {
            logger.Printf("udp client video control conn set write buffer error, %v", err)
    }
#endif

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
