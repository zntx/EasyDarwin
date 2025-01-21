//
// Created by zhangyuzhu8 on 2025/1/20.
//

#ifndef EASYDARWIN_RTSP_RESPONSE_H
#define EASYDARWIN_RTSP_RESPONSE_H

#include <string>
#include <map>
using namespace std;

class Response  {
public:
    string Version;
    int StatusCode;
    string Status;
    map<string, string> Header;
    string Body ;

public:
    Response(int statusCode, string status, string body);
    string String() ;
    void SetBody(string body ) ;
};

Response* NewResponse(int statusCode , string status, string cSeq, string sid, const string& body );

#endif //EASYDARWIN_RTSP_RESPONSE_H
