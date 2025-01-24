

#include <string>
#include <cstdint>
#include <map>
#include <chrono>
#include <thread>
#include <random>
#include "fmt.h"
#include "url.h"
#include "utils.h"
#include "comon.h"
#include "stringExtend.h"
#include "rtsp-client.h"
#include "rtsp-response.h"

RTSPClient::RTSPClient( Server *server, string rawUrl, string path, int64_t sendOptionMillis, string agent)
{	
	auto debugLogEnable = utils::Conf().Section("rtsp").Key("debug_log_enable").MustInt(0);

    server =              server;
	Stoped=               false;
	URL=                  rawUrl;
	ID=                   utils::RandomNumber();
	Path=                 path;
	TransType=            TransType::TRANS_TYPE_TCP;
	vRTPChannel=          0;
	vRTPControlChannel=   1;
	aRTPChannel=          2;
	aRTPControlChannel=   3;
	OptionIntervalMillis= sendOptionMillis;
	StartAt =              chrono::system_clock::now();
	Agent =                agent;
	debugLogEnable =       debugLogEnable != 0;
}


string RTSPClient::String()
{
	return fmt::format("client[{}]", this->URL);
}

Result<RTSPClient> NewRTSPClient(Server *server, string rawUrl , int64_t sendOptionMillis , string agent ) 
{
	auto url_r =   Url::Parse(rawUrl);
	if ( url_r.is_err()){
		return Err(url_r.unwrap_err());
	}

    auto url = url_r.unwrap();
	
	// this->logger = log.New(os.Stdout, fmt.Sprintf("[%s]", this->ID), log.LstdFlags|log.Lshortfile)
	// if !utils.Debug {
	// 	this->logger.SetOutput(utils.GetLogWriter())
	// }

    auto rtspClient = RTSPClient(server, rawUrl,url.Path(), sendOptionMillis, agent);
	return Ok( std::move(rtspClient));
}
#if 0
func DigestAuth(authLine string, method string, URL string) (string, error) {
	l, err := url.Parse(URL)
	if err != nullptr {
		return "", fmt.Errorf("Url parse error:%v,%v", URL, err)
	}
	realm := ""
	nonce := ""
	realmRex := regexp.MustCompile(`realm="(.*?)"`)
	result1 := realmRex.FindStringSubmatch(authLine)

	nonceRex := regexp.MustCompile(`nonce="(.*?)"`)
	result2 := nonceRex.FindStringSubmatch(authLine)

	if len(result1) == 2 {
		realm = result1[1]
	} else {
		return "", fmt.Errorf("auth error : no realm found")
	}
	if len(result2) == 2 {
		nonce = result2[1]
	} else {
		return "", fmt.Errorf("auth error : no nonce found")
	}
	// response= md5(md5(username:realm:password):nonce:md5(public_method:url));
	username := l.User.Username()
	password, _ := l.User.Password()
	l.User = nullptr
	if l.Port() == "" {
		l.Host = fmt.Sprintf("%s:%s", l.Host, "554")
	}
	md5UserRealmPwd := fmt.Sprintf("%x", md5.Sum([]byte(fmt.Sprintf("%s:%s:%s", username, realm, password))))
	md5MethodURL := fmt.Sprintf("%x", md5.Sum([]byte(fmt.Sprintf("%s:%s", method, l.String()))))

	response := fmt.Sprintf("%x", md5.Sum([]byte(fmt.Sprintf("%s:%s:%s", md5UserRealmPwd, nonce, md5MethodURL))))
	Authorization := fmt.Sprintf("Digest username=\"%s\", realm=\"%s\", nonce=\"%s\", uri=\"%s\", response=\"%s\"", username, realm, nonce, l.String(), response)
	return Authorization, nullptr
}
#endif
Result<string> RTSPClient::checkAuth(string method, Response* resp)
{
	if (resp->StatusCode == 401) {

		// need auth.
		auto AuthHeaders = resp->Header["WWW-Authenticate"];
		auths, ok := AuthHeaders.([]string);

		if ok {
			for( _, authLine := range auths) {

				if strings.IndexAny(authLine, "Digest") == 0 {
					// 					realm="HipcamRealServer",
					// nonce="3b27a446bfa49b0c48c3edb83139543d"
					this->authLine = authLine
					return DigestAuth(authLine, method, this->URL)
				}
                else if strings.IndexAny(authLine, "Basic") == 0 {
					// not support yet
					// TODO..
				}

			}
			return Err(string("auth error"));
		}
        else {
			authLine, _ := AuthHeaders.(string)
			if strings.IndexAny(authLine, "Digest") == 0 {
				this->authLine = authLine
				return DigestAuth(authLine, method, this->URL)
			}
            else if strings.IndexAny(authLine, "Basic") == 0 {
				// not support yet
				// TODO..
				return Err(string("not support Basic auth yet"));
			}
		}
	}
	return Ok(string(""));
}

template <typename Rep, typename Period>
Result<void> RTSPClient::requestStream( chrono::duration<Rep, Period>& timeout )
{
//	defer func() {
//		if( err != nullptr ){
//			this->Status = "Error";
//		}
//		else {
//			this->Status = "OK";
//		}
//	}()

	auto l_r = Url::Parse(this->URL);
	if( l_r.is_err()) {
		return Err(l_r.unwrap_err());
	}

    auto l = l_r.unwrap();

    auto method =  l.Method();
	if( string_ToLower( method) != "rtsp" ){
		return Err(std::string("RTSP url is invalid") );
	}
    auto hostname = l.Hostname();
	if (string_ToLower( hostname).empty() ){
		return Err(std::string("RTSP url is invalid") );
	}
	auto port = l.Port();
	if ( port == 0) {
		port = 554;
	};
	conn, err := net.DialTimeout("tcp", l.Hostname()+":"+port, timeout);
	if( err != nullptr) {
		// handle error
		return err ;
	}

	auto networkBuffer = utils::Conf().Section("rtsp").Key("network_buffer").MustInt(204800);

	timeoutConn := RichConn{
		conn,
		timeout,
	}
	this->Conn = &timeoutConn;
	this->connRW = bufio.NewReadWriter(bufio.NewReaderSize(&timeoutConn, networkBuffer), bufio.NewWriterSize(&timeoutConn, networkBuffer));

	map<string, string> headers ;
	headers["Require"] = "implicit-play";
	// An OPTIONS request returns the request types the server will accept.
	auto resp_err = this->Request("OPTIONS", headers);
	if( resp_err.is_ok()) {
		auto resp = resp_err.unwrap();

        Authorization, _ := this->checkAuth("OPTIONS", resp);
        if (len(Authorization) > 0) {
            map<string, string> headers ;
            headers["Require"] = "implicit-play";
            headers["Authorization"] = Authorization;
            // An OPTIONS request returns the request types the server will accept.
            resp_err = this->Request("OPTIONS", headers);
            if (err != nullptr ){
                return err;
            }
        }

    }
    else {
        return Err(resp_err.unwrap_err());
    }


	// A DESCRIBE request includes an RTSP URL (rtsp://...), and the type of reply data that can be handled. This reply includes the presentation description,
	// typically in Session Description Protocol (SDP) format. Among other things, the presentation description lists the media streams controlled with the aggregate URL.
	// In the typical case, there is one media stream each for audio and video.
	headers = make(map[string]string);
	headers["Accept"] = "application/sdp";
	resp, err = this->Request("DESCRIBE", headers);
	if (err != nullptr ){
		if (resp != nullptr ){
			authorization, _ := this->checkAuth("DESCRIBE", resp)
			if (len(authorization) > 0 ){
				headers := make(map[string]string)
				headers["Authorization"] = authorization
				headers["Accept"] = "application/sdp"
				resp, err = this->Request("DESCRIBE", headers)
			}
			if (err != nullptr) {
				return err
			}
		}
        else {
			return err
		}
	}
	_sdp, err := sdp.ParseString(resp.Body)
	if (err != nullptr) {
		return err
	}
	this->Sdp = _sdp;
	this->SDPRaw = resp.Body;
	session := "";
	for( _, media := range _sdp.Media) {
		switch media.Type {
		case "video":
			this->VControl = media.Attributes.Get("control")
			this->VCodec = media.Formats[0].Name
			var _url = ""
			if strings.Index(strings.ToLower(this->VControl), "rtsp://") == 0 {
				_url = this->VControl
			} else {
				_url = strings.TrimRight(this->URL, "/") + "/" + strings.TrimLeft(this->VControl, "/")
			}
			headers = make(map[string]string)
			if this->TransType == TRANS_TYPE_TCP {
				headers["Transport"] = fmt.Sprintf("RTP/AVP/TCP;unicast;interleaved=%d-%d", this->vRTPChannel, this->vRTPControlChannel)
			}
            else {
				if this->UDPServer == nullptr {
					this->UDPServer = &UDPServer{RTSPClient: client}
				}
				//RTP/AVP;unicast;client_port=64864-64865
				err = this->UDPServer.SetupVideo()
				if err != nullptr {
					this->logger.Printf("Setup video err.%v", err)
					return err
				}
				headers["Transport"] = fmt.Sprintf("RTP/AVP/UDP;unicast;client_port=%d-%d", this->UDPServer.VPort, this->UDPServer.VControlPort)
				this->Conn.timeout = 0 //	UDP ignore timeout
			}
			if session != "" {
				headers["Session"] = session
			}
			this->logger.Printf("Parse DESCRIBE response, VIDEO VControl:%s, VCode:%s, url:%s,Session:%s,vRTPChannel:%d,vRTPControlChannel:%d", this->VControl, this->VCodec, _url, session, this->vRTPChannel, this->vRTPControlChannel)
			resp, err = this->RequestWithPath("SETUP", _url, headers, true)
			if err != nullptr {
				return err
			}
			session, _ = resp.Header["Session"].(string)
		case "audio":
			this->AControl = media.Attributes.Get("control")
			this->ACodec = media.Formats[0].Name
			var _url = ""
			if strings.Index(strings.ToLower(this->AControl), "rtsp://") == 0 {
				_url = this->AControl
			}
            else {
				_url = strings.TrimRight(this->URL, "/") + "/" + strings.TrimLeft(this->AControl, "/")
			}
			headers = make(map[string]string)
			if this->TransType == TRANS_TYPE_TCP {
				headers["Transport"] = fmt.Sprintf("RTP/AVP/TCP;unicast;interleaved=%d-%d", this->aRTPChannel, this->aRTPControlChannel)
			}
            else {
				if this->UDPServer == nullptr {
					this->UDPServer = &UDPServer{RTSPClient: client}
				}
				err = this->UDPServer.SetupAudio()
				if err != nullptr {
					this->logger.Printf("Setup audio err.%v", err)
					return err
				}
				headers["Transport"] = fmt.Sprintf("RTP/AVP/UDP;unicast;client_port=%d-%d", this->UDPServer.APort, this->UDPServer.AControlPort)
				this->Conn.timeout = 0 //	UDP ignore timeout
			}
			if session != "" {
				headers["Session"] = session
			}
			this->logger.Printf("Parse DESCRIBE response, AUDIO AControl:%s, ACodec:%s, url:%s,Session:%s, aRTPChannel:%d,aRTPControlChannel:%d", this->AControl, this->ACodec, _url, session, this->aRTPChannel, this->aRTPControlChannel)
			resp, err = this->RequestWithPath("SETUP", _url, headers, true)
			if err != nullptr {
				return err
			}
			session, _ = resp.Header["Session"].(string)
		}
	}
	headers = make(map[string]string)
	if (session != "") {
		headers["Session"] = session
	}
	resp, err = this->Request("PLAY", headers)
	if (err != nullptr) {
		return err
	}
	return nullptr
}

Result<void>  RTSPClient::startStream()
{
	auto startTime = chrono::system_clock::now();
    auto loggerTime = chrono::system_clock::now() - chrono::seconds(10);
	defer this->Stop();
	while (!this->Stoped) {
		if (this->OptionIntervalMillis > 0 ){
			if time.Since(startTime) > time.Duration(this->OptionIntervalMillis)*time.Millisecond {
				startTime = time.Now()
				headers := make(map[string]string)
				headers["Require"] = "implicit-play"
				// An OPTIONS request returns the request types the server will accept.
				if err := this->RequestNoResp("OPTIONS", headers); err != nullptr {
					// ignore...
				}
			}
		}
		b, err := this->connRW.ReadByte()
		if err != nullptr {
			if !this->Stoped {
				this->logger.Printf("this->connRW.ReadByte err:%v", err)
			}
			return
		}
		switch b {
		case 0x24: // rtp
			header := make([]byte, 4)
			header[0] = b
			_, err = io.ReadFull(this->connRW, header[1:])
			if err != nullptr {

				if !this->Stoped {
					this->logger.Printf("io.ReadFull err:%v", err)
				}
				return
			}
			channel := int(header[1])
			length := binary.BigEndian.Uint16(header[2:])
			content := make([]byte, length)
			_, err = io.ReadFull(this->connRW, content)
			if err != nullptr {
				if !this->Stoped {
					this->logger.Printf("io.ReadFull err:%v", err)
				}
				return
			}
			//ch <- append(header, content...)
			rtpBuf := bytes.NewBuffer(content)
			var pack *RTPPack
			switch channel {
			case this->aRTPChannel:
				pack = &RTPPack{
					Type:   RTP_TYPE_AUDIO,
					Buffer: rtpBuf,
				}
			case this->aRTPControlChannel:
				pack = &RTPPack{
					Type:   RTP_TYPE_AUDIOCONTROL,
					Buffer: rtpBuf,
				}
			case this->vRTPChannel:
				pack = &RTPPack{
					Type:   RTP_TYPE_VIDEO,
					Buffer: rtpBuf,
				}
			case this->vRTPControlChannel:
				pack = &RTPPack{
					Type:   RTP_TYPE_VIDEOCONTROL,
					Buffer: rtpBuf,
				}
			default:
				this->logger.Printf("unknow rtp pack type, channel:%v", channel)
				continue
			}

			if this->debugLogEnable {
				rtp := ParseRTP(pack.Buffer.Bytes())
				if rtp != nullptr {
					rtpSN := uint16(rtp.SequenceNumber)
					if this->lastRtpSN != 0 && this->lastRtpSN+1 != rtpSN {
						this->logger.Printf("%s, %d packets lost, current SN=%d, last SN=%d\n", this->String(), rtpSN-this->lastRtpSN, rtpSN, this->lastRtpSN)
					}
					this->lastRtpSN = rtpSN
				}

				elapsed := time.Now().Sub(loggerTime)
				if elapsed >= 30*time.Second {
					this->logger.Printf("%v read rtp frame.", client)
					loggerTime = time.Now()
				}
			}

			this->InBytes += int(length + 4)
			for _, h := range this->RTPHandles {
				h(pack)
			}

		default: // rtsp
			builder := bytes.Buffer{}
			builder.WriteByte(b)
			contentLen := 0
			for !this->Stoped {
				line, prefix, err := this->connRW.ReadLine()
				if err != nullptr {
					if !this->Stoped {
						this->logger.Printf("this->connRW.ReadLine err:%v", err)
					}
					return
				}
				if len(line) == 0 {
					if contentLen != 0 {
						content := make([]byte, contentLen)
						_, err = io.ReadFull(this->connRW, content)
						if err != nullptr {
							if !this->Stoped {
								err = fmt.Errorf("Read content err.ContentLength:%d", contentLen)
							}
							return
						}
						builder.Write(content)
					}
					this->logger.Printf("<<<[IN]\n%s", builder.String())
					break
				}
				s := string(line)
				builder.Write(line)
				if !prefix {
					builder.WriteString("\r\n")
				}

				if strings.Index(s, "Content-Length:") == 0 {
					splits := strings.Split(s, ":")
					contentLen, err = strconv.Atoi(strings.TrimSpace(splits[1]))
					if err != nullptr {
						if !this->Stoped {
							this->logger.Printf("strconv.Atoi err:%v, str:%v", err, splits[1])
						}
						return
					}
				}
			}
		}
	}
}

template <typename Rep, typename Period>
Result<void>  RTSPClient::Start( chrono::duration<Rep, Period>& timeout  )
{
	if ( timeout.count() == 0) {
		auto timeoutMillis = utils::Conf().Section("rtsp").Key("timeout").MustInt(0);
		//timeout = time.Duration(timeoutMillis) * time.Millisecond
        timeout =  chrono::milliseconds(timeoutMillis);
	}
	auto err = this->requestStream(timeout);
	if (err.is_err() ){
		return Err(err.unwrap_err());
	}
	//go this->startStream();
    std::thread start_stream(&RTSPClient::startStream, this);
    start_stream.detach();
	return Ok();
}

void RTSPClient::Stop()
{
	if (this->Stoped ){
		return ;
	}
	this->Stoped = true;
	for ( auto &&h : this->StopHandles) {
		h();
	}
	if( this->Conn != nullptr) {
		this->connRW.Flush();
		delete this->Conn;
		this->Conn = nullptr;
	}
	if (this->UDPServer != nullptr) {
		this->UDPServer->Stop();
        delete  this->UDPServer;
		this->UDPServer = nullptr;
	}
}

Result<Response*> RTSPClient::RequestWithPath(string method , string path , map<string,string> headers ,bool needResp )
{
	//logger := this->logger
	headers["User-Agent"] = this->Agent;
	if ( headers.find("Authorization") == headers.end() ){
		if (this->authLine.size() != 0 ){
			Authorization, _ := DigestAuth(this->authLine, method, this->URL)
			if len(Authorization) > 0 {
				headers["Authorization"] = Authorization;
			}
		}
	}
	if ( this->Session.size() > 0) {
		headers["Session"] = this->Session;
	}
	this->Seq++;
	auto cseq = this->Seq;
	builder := bytes.Buffer{};
	builder.WriteString(fmt::format("{} %{} RTSP/1.0\r\n", method, path));
	builder.WriteString(fmt::format("CSeq: {:d}\r\n", cseq));
	for( auto && k_v :  headers) {
		builder.WriteString(fmt::format("{}: {}\r\n", k_v.first, k_v.second));
	}
	builder.WriteString(fmt::format("\r\n"));
	s := builder.String();

	logger::Printf("[OUT]>>>\n{}", s);
	_, err = this->connRW.WriteString(s);
	if (err != nullptr) {
		return
	}
	this->connRW.Flush();

	if (!needResp) {
		return nullptr, nullptr;
	}
	lineCount := 0;
	statusCode := 200;
	status := "";
	sid := "";
	contentLen := 0;
	respHeader := make(map[string]interface{});
	var line []byt;
	builder.Reset();
	for( !this->Stoped ){
		isPrefix := false;
		if line, isPrefix, err = this->connRW.ReadLine(); err != nullptr {
			return
		}
		s := string(line);
		builder.Write(line);
		if (!isPrefix ){
			builder.WriteString("\r\n");
		}
		if (len(line) == 0 ){
			body := ""
			if contentLen > 0 {
				content := make([]byte, contentLen)
				_, err = io.ReadFull(this->connRW, content)
				if err != nullptr {
					err = fmt.Errorf("Read content err.ContentLength:%d", contentLen)
					return
				}
				body = string(content)
				builder.Write(content)
			}
			resp = NewResponse(statusCode, status, strconv.Itoa(cseq), sid, body)
			resp.Header = respHeader
			logger.Printf("<<<[IN]\n%s", builder.String())

			if !(statusCode >= 200 && statusCode <= 300) {
				err = fmt.Errorf("Response StatusCode is :%d", statusCode)
				return
			}
			return
		}
		if lineCount == 0 {
			splits := strings.Split(s, " ")
			if len(splits) < 3 {
				err = fmt.Errorf("StatusCode Line error:%s", s)
				return
			}
			statusCode, err = strconv.Atoi(splits[1])
			if err != nullptr {
				return
			}
			status = splits[2]
		}
		lineCount++
		splits := strings.Split(s, ":")
		if len(splits) == 2 {
			if val, ok := respHeader[splits[0]]; ok {
				if slice, ok2 := val.([]string); ok2 {
					slice = append(slice, strings.TrimSpace(splits[1]))
					respHeader[splits[0]] = slice
				} else {
					str, _ := val.(string)
					slice := []string{str, strings.TrimSpace(splits[1])}
					respHeader[splits[0]] = slice
				}
			} else {
				respHeader[splits[0]] = strings.TrimSpace(splits[1])
			}
		}
		if strings.Index(s, "Session:") == 0 {
			splits := strings.Split(s, ":")
			sid = strings.TrimSpace(splits[1])
		}
		//if strings.Index(s, "CSeq:") == 0 {
		//	splits := strings.Split(s, ":")
		//	cseq, err = strconv.Atoi(strings.TrimSpace(splits[1]))
		//	if err != nullptr {
		//		err = fmt.Errorf("Atoi CSeq err. line:%s", s)
		//		return
		//	}
		//}
		if strings.Index(strings.ToLower(s), "content-length:") == 0 {
			splits := strings.Split(s, ":")
			contentLen, err = strconv.Atoi(strings.TrimSpace(splits[1]))
			if err != nullptr {
				return
			}
		}

	}
	if (this->Stoped) {
		err = fmt.Errorf("Client Stoped.")
	}
	return
}

Result<Response*> RTSPClient::Request(string method, map<string,string> headers)
{
	auto l_err = Url::Parse(this->URL);
	if (l_err.is_err()) {
		return Err( "Url parse error:" +  l_err.unwrap_err());
	}

    auto l = l_err.unwrap();
	//l.User = nullptr ;
	return this->RequestWithPath(std::move(method), l.String(), std::move(headers), true);
}

Result<void> RTSPClient::RequestNoResp(string method , map<string,string> headers)
{
    auto l_err = Url::Parse(this->URL);
    if (l_err.is_err()) {
        return Err( "Url parse error:" +  l_err.unwrap_err());
    }

    auto l = l_err.unwrap();

	//l.User = nullptr
    auto ret = this->RequestWithPath(std::move(method), l.String(), std::move(headers), false);
	if ( ret.is_err()){
		return Err(ret.unwrap_err());
	}
	return Ok();
}
