
#ifndef EASYDARWIN_RTSP_REQUESTE_H
#define EASYDARWIN_RTSP_REQUESTE_H

#include <string>
#include <map>

using namespace std;

#define RTSP_VERSION  "RTSP/1.0"

	// Client to server for presentation and stream objects; recommended
#define  DESCRIBE  "DESCRIBE"
	// Bidirectional for client and stream objects; optional
#define ANNOUNCE  "ANNOUNCE"
	// Bidirectional for client and stream objects; optional
#define GET_PARAMETER  "GET_PARAMETER"
	// Bidirectional for client and stream objects; required for Client to server, optional for server to client
#define OPTIONS  "OPTIONS"
	// Client to server for presentation and stream objects; recommended
#define PAUSE  "PAUSE"
	// Client to server for presentation and stream objects; required
#define PLAY  "PLAY"
	// Client to server for presentation and stream objects; optional
#define RECORD  "RECORD"
	// Server to client for presentation and stream objects; optional
#define REDIRECT  "REDIRECT"
	// Client to server for stream objects; required
#define SETUP  "SETUP"
	// Bidirectional for presentation and stream objects; optional
#define SET_PARAMETER  "SET_PARAMETER"
	// Client to server for presentation and stream objects; required
#define TEARDOWN  "TEARDOWN"
#define DATA      "DATA"


class Request {
public:
	string 				Method ; 
	string 				URL ;    
	string 				Version; 
	map<string, string> Header ; 
	string 				Content ;
	string 				Body ;

    Request( string method, string url, string version,  string conten);
    string String() ;  
    int GetContentLength();
};

Request* NewRequest(string content );

#endif //EASYDARWIN_RTSP_REQUESTE_H
