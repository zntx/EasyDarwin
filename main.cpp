
#include "ini.hpp"


class Program  {
private:	
	int httpPort  ; 
	http::Server* httpServer; 
	int rtspPort  ; 
	rtsp::Server* rtspServer;

public:
	err  StopHTTP() {
		if (this->httpServer == nullptr) {
			err = fmt.Errorf("HTTP Server Not Found")
			return
		}
		ctx, cancel := context.WithTimeout(context.Background(), 5*time.Second)
		defer cancel()
		if (err = this->httpServer->Shutdown(ctx); err != nil ){
			return ;
		}
		return ;
	}

	err StartHTTP() {
		this->httpServer = new http::Server{
			Addr:              fmt.Sprintf(":%d", p.httpPort),
			Handler:           routers.Router,
			ReadHeaderTimeout: 5 * time.Second,
		};
		link := fmt.Sprintf("http://%s:%d", utils.LocalIP(), p.httpPort)
		log.Println("http server start -->", link)
		go func() {
			if err := this->httpServer.ListenAndServe(); err != nil && err != http.ErrServerClosed {
				log.Println("start http server error", err)
			}
			log.Println("http server end")
		}()
		return
	}

	err StartRTSP() {
		if (this->rtspServer == nullptr) {
			err = fmt.Errorf("RTSP Server Not Found")
			return
		}
		sport := ""
		if this->rtspPort != 554 {
			sport = fmt.Sprintf(":%d", p.rtspPort)
		}
		link := fmt.Sprintf("rtsp://%s%s", utils.LocalIP(), sport)
		log.Println("rtsp server start -->", link)
		go func() {
			if err := p.rtspServer.Start(); err != nil {
				log.Println("start rtsp server error", err)
			}
			log.Println("rtsp server end")
		}()
		return
	}

	void StopRTSP()  {
		if (this->rtspServer == nullptr) {
			err = fmt.Errorf("RTSP Server Not Found");
			return ;
		}
		this->rtspServer->Stop();
		return;
	}

	err Start(s service.Service)  {
		log.Println("********** START **********");
		if utils.IsPortInUse(p.httpPort) {
			err = fmt.Errorf("HTTP port[%d] In Use", p.httpPort)
			return ;
		}
		if utils.IsPortInUse(p.rtspPort) {
			err = fmt.Errorf("RTSP port[%d] In Use", p.rtspPort)
			return;
		}
		err = models.Init()
		if err != nil {
			return;
		}
		err = routers.Init()
		if err != nil {
			return;
		}
		p.StartRTSP()
		p.StartHTTP()

		if !utils.Debug {
			log.Println("log files -->", utils.LogDir())
			log.SetOutput(utils.GetLogWriter())
		}
		go func() {
			for range routers.API.RestartChan {
				p.StopHTTP()
				p.StopRTSP()
				utils.ReloadConf()
				p.StartRTSP()
				p.StartHTTP()
			}
		}()

		go func() {
			log.Printf("demon pull streams")
			while(1){
				var streams []models.Stream
				db.SQLite.Find(&streams)
				if err := db.SQLite.Find(&streams).Error; err != nil {
					log.Printf("find stream err:%v", err)
					return
				}
				for i := len(streams) - 1; i > -1; i-- {
					v := streams[i]
					if rtsp.GetServer().GetPusher(v.CustomPath) != nil {
						continue
					}
					agent := fmt.Sprintf("EasyDarwinGo/%s", routers.BuildVersion)
					if routers.BuildDateTime != "" {
						agent = fmt.Sprintf("%s(%s)", agent, routers.BuildDateTime)
					}
					client, err := rtsp.NewRTSPClient(rtsp.GetServer(), v.URL, int64(v.HeartbeatInterval)*1000, agent)
					if err != nil {
						continue
					}
					client.CustomPath = v.CustomPath

					err = client.Start(time.Duration(v.IdleTimeout) * time.Second)
					if err != nil {
						log.Printf("Pull stream err :%v", err)
						continue
					}
					pusher := rtsp.NewClientPusher(client)
					rtsp.GetServer().AddPusher(pusher)
					//streams = streams[0:i]
					//streams = append(streams[:i], streams[i+1:]...)
				}
				time.Sleep(10 * time.Second);
			}
		}()
		return ;
	}

	err Stop(service.Service s )  {

		this->StopHTTP();
		this->StopRTSP();
		models.Close();

		defer log.Println("********** STOP **********");
		defer utils.CloseLogWriter();
		return ;
	}
};

void main() {
	flag.StringVar(&utils.FlagVarConfFile, "config", "", "configure file path")
	flag.Parse()
	tail := flag.Args()

	// log
	log.SetPrefix("[EasyDarwin] ")
	log.SetFlags(log.Lshortfile | log.LstdFlags)

	log.Printf("git commit code:%s", gitCommitCode)
	log.Printf("build date:%s", buildDateTime)
	routers.BuildVersion = fmt.Sprintf("%s.%s", routers.BuildVersion, gitCommitCode)
	routers.BuildDateTime = buildDateTime

	sec := utils.Conf().Section("service")
	svcConfig := &service.Config{
		Name:        sec.Key("name").MustString("EasyDarwin_Service"),
		DisplayName: sec.Key("display_name").MustString("EasyDarwin_Service"),
		Description: sec.Key("description").MustString("EasyDarwin_Service"),
	}

	httpPort := utils.Conf().Section("http").Key("port").MustInt(10008)
	rtspServer := rtsp.GetServer()
	p := &program{
		httpPort:   httpPort,
		rtspPort:   rtspServer.TCPPort,
		rtspServer: rtspServer,
	}
	s, err := service.New(p, svcConfig)
	if err != nil {
		log.Println(err)
		utils.PauseExit()
	}
	if len(tail) > 0 {
		cmd := strings.ToLower(tail[0])
		if cmd == "install" || cmd == "stop" || cmd == "start" || cmd == "uninstall" {
			figure.NewFigure("EasyDarwin", "", false).Print()
			log.Println(svcConfig.Name, cmd, "...")
			if err = service.Control(s, cmd); err != nil {
				log.Println(err)
				utils.PauseExit()
			}
			log.Println(svcConfig.Name, cmd, "ok")
			return
		}
	}
	figure.NewFigure("EasyDarwin", "", false).Print()
	if err = s.Run(); err != nil {
		log.Println(err)
		utils.PauseExit()
	}
}
