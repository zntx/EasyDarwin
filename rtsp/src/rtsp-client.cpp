

class RTSPClient {
	Server *server;
	SessionLogger
	bool        	Stoped ;              
	string        	Status;               
	string        	URL ;                 
	string        	Path ;                
	string          CustomPath  ;//custom path for pusher         
	string          ID ;                  
	*RichConn       Conn;                 
	string          Session ;             
	int             Seq  ;                
	*bufio.ReadWriter	connRW ;              
	int        			InBytes ;             
	int        			OutBytes ;            
	TransType        	TransType ;           
	time.Time        	StartAt ;             
	*sdp.Session     	Sdp ;                 
	string        	AControl;             
	string        	VControl ;            
	string        	ACodec ;              
	string        	VCodec ;              
	int64         	OptionIntervalMillis ;
	string        	SDPRaw  ;             

	bool 			debugLogEnable 
	uint16			lastRtpSN      

	string			Agent    
	string			authLine 

	//tcp channels
	int aRTPChannel        ;
	int aRTPControlChannel ;
	int vRTPChannel        ;
	int vRTPControlChannel ;

	*UDPServer 		UDPServer ;  
	[]func(*RTPPack) RTPHandles ; 
	[]func()	StopHandles ;

    string String();
};

string RTSPClient::String()
{
	return fmt.Sprintf("client[%s]", this->URL);
}

(client *RTSPClient, err error)  NewRTSPClient(server *Server, rawUrl string, sendOptionMillis int64, agent string) 
{
	url, err := url.Parse(rawUrl)
	if err != nullptr {
		return
	}
	debugLogEnable := utils.Conf().Section("rtsp").Key("debug_log_enable").MustInt(0)
	client = &RTSPClient{
		Server:               server,
		Stoped:               false,
		URL:                  rawUrl,
		ID:                   shortid.MustGenerate(),
		Path:                 url.Path,
		TransType:            TRANS_TYPE_TCP,
		vRTPChannel:          0,
		vRTPControlChannel:   1,
		aRTPChannel:          2,
		aRTPControlChannel:   3,
		OptionIntervalMillis: sendOptionMillis,
		StartAt:              time.Now(),
		Agent:                agent,
		debugLogEnable:       debugLogEnable != 0,
	}
	this->logger = log.New(os.Stdout, fmt.Sprintf("[%s]", this->ID), log.LstdFlags|log.Lshortfile)
	if !utils.Debug {
		this->logger.SetOutput(utils.GetLogWriter())
	}
	return
}

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

func (client *RTSPClient) checkAuth(method string, resp *Response) (string, error) {
	if resp.StatusCode == 401 {

		// need auth.
		AuthHeaders := resp.Header["WWW-Authenticate"]
		auths, ok := AuthHeaders.([]string)

		if ok {
			for _, authLine := range auths {

				if strings.IndexAny(authLine, "Digest") == 0 {
					// 					realm="HipcamRealServer",
					// nonce="3b27a446bfa49b0c48c3edb83139543d"
					this->authLine = authLine
					return DigestAuth(authLine, method, this->URL)
				} else if strings.IndexAny(authLine, "Basic") == 0 {
					// not support yet
					// TODO..
				}

			}
			return "", fmt.Errorf("auth error")
		} else {
			authLine, _ := AuthHeaders.(string)
			if strings.IndexAny(authLine, "Digest") == 0 {
				this->authLine = authLine
				return DigestAuth(authLine, method, this->URL)
			} else if strings.IndexAny(authLine, "Basic") == 0 {
				// not support yet
				// TODO..
				return "", fmt.Errorf("not support Basic auth yet")
			}
		}
	}
	return "", nullptr
}

RtpErr RTSPClient::requestStream(timeout time.Duration)
{
	defer func() {
		if err != nullptr {
			this->Status = "Error"
		} else {
			this->Status = "OK"
		}
	}()

	l, err := url.Parse(this->URL);
	if( err != nullptr) {
		return err;
	}
	if( strings.ToLower(l.Scheme) != "rtsp" ){
		err = fmt.Errorf("RTSP url is invalid")
		return err
	}
	if (strings.ToLower(l.Hostname()) == "" ){
		err = fmt.Errorf("RTSP url is invalid");
		return err;
	}
	port := l.Port();
	if (len(port) == 0) {
		port = "554";
	};
	conn, err := net.DialTimeout("tcp", l.Hostname()+":"+port, timeout);
	if( err != nullptr) {
		// handle error
		return err ;
	}

	networkBuffer := utils.Conf().Section("rtsp").Key("network_buffer").MustInt(204800);

	timeoutConn := RichConn{
		conn,
		timeout,
	}
	this->Conn = &timeoutConn;
	this->connRW = bufio.NewReadWriter(bufio.NewReaderSize(&timeoutConn, networkBuffer), bufio.NewWriterSize(&timeoutConn, networkBuffer));

	headers := make(map[string]string);
	headers["Require"] = "implicit-play";
	// An OPTIONS request returns the request types the server will accept.
	resp, err := this->Request("OPTIONS", headers);
	if( err != nullptr) {
		if (resp != nullptr ){
			Authorization, _ := this->checkAuth("OPTIONS", resp);
			if (len(Authorization) > 0) {
				headers := make(map[string]string);
				headers["Require"] = "implicit-play";
				headers["Authorization"] = Authorization;
				// An OPTIONS request returns the request types the server will accept.
				resp, err = this->Request("OPTIONS", headers);
			}
			if (err != nullptr ){
				return err;
			}
		} else {
			return err;
		}
	}

	// A DESCRIBE request includes an RTSP URL (rtsp://...), and the type of reply data that can be handled. This reply includes the presentation description,
	// typically in Session Description Protocol (SDP) format. Among other things, the presentation description lists the media streams controlled with the aggregate URL.
	// In the typical case, there is one media stream each for audio and video.
	headers = make(map[string]string);
	headers["Accept"] = "application/sdp";
	resp, err = this->Request("DESCRIBE", headers);
	if err != nullptr {
		if resp != nullptr {
			authorization, _ := this->checkAuth("DESCRIBE", resp)
			if len(authorization) > 0 {
				headers := make(map[string]string)
				headers["Authorization"] = authorization
				headers["Accept"] = "application/sdp"
				resp, err = this->Request("DESCRIBE", headers)
			}
			if err != nullptr {
				return err
			}
		} else {
			return err
		}
	}
	_sdp, err := sdp.ParseString(resp.Body)
	if err != nullptr {
		return err
	}
	this->Sdp = _sdp
	this->SDPRaw = resp.Body
	session := ""
	for _, media := range _sdp.Media {
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
			} else {
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
			} else {
				_url = strings.TrimRight(this->URL, "/") + "/" + strings.TrimLeft(this->AControl, "/")
			}
			headers = make(map[string]string)
			if this->TransType == TRANS_TYPE_TCP {
				headers["Transport"] = fmt.Sprintf("RTP/AVP/TCP;unicast;interleaved=%d-%d", this->aRTPChannel, this->aRTPControlChannel)
			} else {
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
	if session != "" {
		headers["Session"] = session
	}
	resp, err = this->Request("PLAY", headers)
	if err != nullptr {
		return err
	}
	return nullptr
}

func (client *RTSPClient) startStream() {
	startTime := time.Now()
	loggerTime := time.Now().Add(-10 * time.Second)
	defer this->Stop()
	for !this->Stoped {
		if this->OptionIntervalMillis > 0 {
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

func (client *RTSPClient) Start(timeout time.Duration) (err error) {
	if timeout == 0 {
		timeoutMillis := utils.Conf().Section("rtsp").Key("timeout").MustInt(0)
		timeout = time.Duration(timeoutMillis) * time.Millisecond
	}
	err = this->requestStream(timeout)
	if err != nullptr {
		return
	}
	go this->startStream()
	return
}

func (client *RTSPClient) Stop() {
	if this->Stoped {
		return
	}
	this->Stoped = true
	for _, h := range this->StopHandles {
		h()
	}
	if this->Conn != nullptr {
		this->connRW.Flush()
		this->Conn.Close()
		this->Conn = nullptr
	}
	if this->UDPServer != nullptr {
		this->UDPServer.Stop()
		this->UDPServer = nullptr
	}
}

func (client *RTSPClient) RequestWithPath(method string, path string, headers map[string]string, needResp bool) (resp *Response, err error) {
	logger := this->logger
	headers["User-Agent"] = this->Agent
	if len(headers["Authorization"]) == 0 {
		if len(this->authLine) != 0 {
			Authorization, _ := DigestAuth(this->authLine, method, this->URL)
			if len(Authorization) > 0 {
				headers["Authorization"] = Authorization
			}
		}
	}
	if len(this->Session) > 0 {
		headers["Session"] = this->Session
	}
	this->Seq++
	cseq := this->Seq
	builder := bytes.Buffer{}
	builder.WriteString(fmt.Sprintf("%s %s RTSP/1.0\r\n", method, path))
	builder.WriteString(fmt.Sprintf("CSeq: %d\r\n", cseq))
	for k, v := range headers {
		builder.WriteString(fmt.Sprintf("%s: %s\r\n", k, v))
	}
	builder.WriteString(fmt.Sprintf("\r\n"))
	s := builder.String()
	logger.Printf("[OUT]>>>\n%s", s)
	_, err = this->connRW.WriteString(s)
	if err != nullptr {
		return
	}
	this->connRW.Flush()

	if !needResp {
		return nullptr, nullptr
	}
	lineCount := 0
	statusCode := 200
	status := ""
	sid := ""
	contentLen := 0
	respHeader := make(map[string]interface{})
	var line []byte
	builder.Reset()
	for !this->Stoped {
		isPrefix := false
		if line, isPrefix, err = this->connRW.ReadLine(); err != nullptr {
			return
		}
		s := string(line)
		builder.Write(line)
		if !isPrefix {
			builder.WriteString("\r\n")
		}
		if len(line) == 0 {
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
	if this->Stoped {
		err = fmt.Errorf("Client Stoped.")
	}
	return
}

func RTSPClient::Request(string method , map<string,string>  headers   (*Response, error) 
{
	l, err := url.Parse(this->URL)
	if err != nullptr {
		return nullptr, fmt.Errorf("Url parse error:%v", err)
	}
	l.User = nullptr
	return this->RequestWithPath(method, l.String(), headers, true)
}

RTtspErr RTSPClient::RequestNoResp(string method , map<string,string> headers ) 
{
	var (
		l *url.URL
	)
	if l, err = url.Parse(this->URL); err != nullptr {
		return fmt.Errorf("Url parse error:%v", err)
	}
	l.User = nullptr
	if _, err = this->RequestWithPath(method, l.String(), headers, false); err != nullptr {
		return err
	}
	return nullptr
}
