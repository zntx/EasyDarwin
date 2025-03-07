
#include <thread>
#include "pusher.h"
#include "rtsp-server.h"
#include "rtsp-session.h"
#include "utils.h"
#include "hdlog.h"


Server* GetServer()
{
	//once.Do(func() {
    static Server instance;
	//})
	return &instance;
}

Server::Server()
{
    Stoped =         true;
    TCPPort =        utils::Conf().Section("rtsp").Key("port").MustInt(554);
    tcpListener =    nullptr;
    //pushers =       make(map[string]*Pusher),
    //addPusherCh =    make(chan *Pusher),
    //removePusherCh = make(chan *Pusher),
}

Result<void> Server::Start()
{
    auto lisetener_wrap = TcpListener::Bind("0.0.0.0", this->TCPPort);
	if ( lisetener_wrap.is_err()) {
		return Err(lisetener_wrap.unwrap_err());
	}

    auto lisetener = lisetener_wrap.unwrap();

	int localRecord = utils::Conf().Section("rtsp").Key("save_stream_to_local").MustInt(0);
	auto ffmpeg = utils::Conf().Section("rtsp").Key("ffmpeg_path").MustString("");
	auto m3u8_dir_path = utils::Conf().Section("rtsp").Key("m3u8_dir_path").MustString("");
    auto ts_duration_second = utils::Conf().Section("rtsp").Key("ts_duration_second").MustInt(6);
	auto SaveStreamToLocal = false;
#if 0
	if (!ffmpeg.empty() && localRecord > 0 && !m3u8_dir_path.empty() ){
		err = utils.EnsureDir(m3u8_dir_path)
		if err != nil {
			logger.Printf("Create m3u8_dir_path[%s] err:%v.", m3u8_dir_path, err)
		}
        else {
			SaveStreamToLocal = true
		}
	}

	go func() { // save to local.
		pusher2ffmpegMap := make(map[*Pusher]*exec.Cmd)
		if SaveStreamToLocal {
			logger.Printf("Prepare to save stream to local....")
			defer logger.Printf("End save stream to local....")
		}
		var pusher *Pusher
		addChnOk := true
		removeChnOk := true
		for addChnOk || removeChnOk {
			select {
			case pusher, addChnOk = <-this->addPusherCh:
				if SaveStreamToLocal {
					if addChnOk {
						dir := path.Join(m3u8_dir_path, pusher.Path(), time.Now().Format("20060102"))
						err := utils.EnsureDir(dir)
						if err != nil {
							logger.Printf("EnsureDir:[%s] err:%v.", dir, err)
							continue
						}
						m3u8path := path.Join(dir, fmt.Sprintf("out.m3u8"))
						port := pusher.Server().TCPPort
						rtsp := fmt.Sprintf("rtsp://localhost:%d%s", port, pusher.Path())
						paramStr := utils.Conf().Section("rtsp").Key(pusher.Path()).MustString("-c:v copy -c:a aac")
						params := []string{"-fflags", "genpts", "-rtsp_transport", "tcp", "-i", rtsp, "-hls_time", strconv.Itoa(ts_duration_second), "-hls_list_size", "0", m3u8path}
						if paramStr != "default" {
							paramsOfThisPath := strings.Split(paramStr, " ")
							params = append(params[:6], append(paramsOfThisPath, params[6:]...)...)
						}
						// ffmpeg -i ~/Downloads/720p.mp4 -s 640x360 -g 15 -c:a aac -hls_time 5 -hls_list_size 0 record.m3u8
						cmd := exec.Command(ffmpeg, params...)
						f, err := os.OpenFile(path.Join(dir, fmt.Sprintf("log.txt")), os.O_RDWR|os.O_CREATE, 0755)
						if err == nil {
							cmd.Stdout = f
							cmd.Stderr = f
						}
						err = cmd.Start()
						if err != nil {
							logger.Printf("Start ffmpeg err:%v", err)
						}
						pusher2ffmpegMap[pusher] = cmd
						logger.Printf("add ffmpeg [%v] to pull stream from pusher[%v]", cmd, pusher)
					} else {
						logger.Printf("addPusherChan closed")
					}
				}
			case pusher, removeChnOk = <-this->removePusherCh:
				if SaveStreamToLocal {
					if removeChnOk {
						cmd := pusher2ffmpegMap[pusher]
						proc := cmd.Process
						if proc != nil {
							logger.Printf("prepare to SIGTERM to process:%v", proc)
							proc.Signal(syscall.SIGTERM)
							proc.Wait()
							// proc.Kill()
							// no need to close attached log file.
							// see "Wait releases any resources associated with the Cmd."
							// if closer, ok := cmd.Stdout.(io.Closer); ok {
							// 	closer.Close()
							// 	logger.Printf("process:%v Stdout closed.", proc)
							// }
							logger.Printf("process:%v terminate.", proc)
						}
						delete(pusher2ffmpegMap, pusher)
						logger.Printf("delete ffmpeg from pull stream from pusher[%v]", pusher)
					} else {
						for _, cmd := range pusher2ffmpegMap {
							proc := cmd.Process
							if proc != nil {
								logger.Printf("prepare to SIGTERM to process:%v", proc)
								proc.Signal(syscall.SIGTERM)
							}
						}
						pusher2ffmpegMap = make(map[*Pusher]*exec.Cmd)
						logger.Printf("removePusherChan closed")
					}
				}
			}
		}
	}()
#endif
	this->Stoped = false;
	this->tcpListener = &lisetener;
	logger::info("rtsp server start on {:d}", this->TCPPort);
	auto networkBuffer = utils::Conf().Section("rtsp").Key("network_buffer").MustInt(1048576);

	while (!this->Stoped) {
        auto stream_warp = this->tcpListener->accept(1);
		if ( stream_warp.is_err()) {
			logger::info("{}", stream_warp.unwrap_err());
			continue;
		}

        auto conn = stream_warp.unwrap();

		//if tcpConn, ok := conn.(*net.TCPConn); ok {
			if (!conn.SetSoRcvbuf(networkBuffer)) {
				logger::info("rtsp server conn set read buffer error,  " );
			}
			if (!conn.SetSoSendbuf(networkBuffer) ) {
				logger::Printf("rtsp server conn set write buffer error, {}" );
			}
		//}

		auto session = NewSession(this, std::move(conn));
		//go session.Start();
        thread session_run(&Session::Start, session);
        session_run.detach();
	}
	return Ok();
}

void Server::Stop() 
{
	//logger := this->logger
	logger::Printf("rtsp server stop on {:d}", this->TCPPort);
	this->Stoped = true;
//	if (this->tcpListener != nullptr) {
//		//delete this->tcpListener;
//		this->tcpListener = nullptr;
//	}
	this->pushersLock.lock();
	this->pushers.clear();
	this->pushersLock.unlock();

	//close(this->addPusherCh);
	//close(this->removePusherCh);
}

bool Server::AddPusher(Pusher* pusher)
{
	//logger := this->logger
	auto added = false;
	this->pushersLock.lock();
	auto iter = this->pushers.find(pusher->Path());
	if ( iter != this->pushers.end()){
		this->pushers[pusher->Path()] = pusher;
		logger::Printf("%v start, now pusher size[%d]", pusher, this->pushers.size());
		added = true;
	}
    else {
		added = false;
	}
	this->pushersLock.unlock();
	if (added ){
		//go pusher.Start();
		//this->addPusherCh <- pusher;
        thread pusher_run(&Pusher::Start, pusher);
        pusher_run.detach();
	}
	return added ;
}

Result<Pusher*> Server::TryAttachToPusher(Session* session)
{
    int attached = 0;
    Pusher* pusher = nullptr;

	this->pushersLock.lock();
    auto _pusher = this->pushers.find(session->Path);
	if ( _pusher != this->pushers.end()){
		if( _pusher->second->RebindSession(session)) {
			logger::Printf("Attached to a pusher");
			attached = 1;
			pusher = _pusher->second;
		}
        else {
			attached = -1;
		}
	}
	this->pushersLock.unlock();

    if( attached )
        return Ok(pusher);

	return Err(string(""));
}

void Server::RemovePusher(Pusher* pusher)
{
	//logger := this->logger
	auto removed = false;
	this->pushersLock.lock();
    auto _pusher = this->pushers.find(pusher->Path());
	if (_pusher != this->pushers.end()  && pusher->ID() == _pusher->second->ID() ){
		this->pushers.erase(_pusher);
		logger::Printf("%v end, now pusher size{%d}\n", pusher, this->pushers.size());
		removed = true;
	}
	this->pushersLock.unlock();
//	if (removed) {
//		this->removePusherCh <- pusher
//	}
}

Pusher* Server::GetPusher(const string& path)
{   
    //Pusher* pusher;
	this->pushersLock.lock();
	auto pusher = this->pushers.find(path);
	this->pushersLock.unlock();
    if (pusher == this->pushers.end()  ){
        return nullptr;
    }
	return pusher->second;
}

map<string,Pusher*> Server::GetPushers()
{
    map<string, Pusher*> _pushers;
	this->pushersLock.lock();
	for (auto && iterm : this->pushers ){
        _pushers[iterm.first] = iterm.second;
	}
	this->pushersLock.unlock();
	return std::move(_pushers);
}

int Server::GetPusherSize()
{
	this->pushersLock.lock();
	auto size = this->pushers.size();
	this->pushersLock.unlock();
	return size;
}
