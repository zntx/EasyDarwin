//
// Created by zhangyuzhu8 on 2025/1/20.
//

#include "fmt/include/fmt/core.h"
#include "rtsp-response.h"
#include "rtsp-request.h"

Response* NewResponse(int statusCode , string status, string cSeq, string sid, const string& body )
{
	auto res = new(std::nothrow) Response(statusCode, status, body);
    if( res == nullptr){
        return res;
    }
    
    res->Header["CSeq"] = cSeq;
    res->Header["Session"] = sid;

	auto len = body.size();
	if (len > 0) {
		res->Header["Content-Length"] = to_string(len);
	}
    else {
		res->Header.erase( "Content-Length");
	}
	return res;
}

Response::Response(int statusCode, string status, string body)
{
    Version =    RTSP_VERSION;
    StatusCode = statusCode;
    Status =    status;
    //Header =     map[string]interface{}{"CSeq": cSeq, "Session": sid};
    Body =      body;
}

string  Response::String() 
{
	auto str = fmt::format("{} {:d} {}\r\n", this->Version, this->StatusCode, this->Status);

    for (auto iter = this->Header.begin(); iter != this->Header.end(); ++iter) {
        str += fmt::format("{}: {}\r\n",  iter->first, iter->second);
    }

	str += "\r\n";
	str += this->Body;
	return str;
}

void Response::SetBody(string body ) 
{
	auto len = body.size();
	this->Body = body;
	if (len > 0) {
		this->Header["Content-Length"] = to_string(len);
	} 
    else {
		this->Header.erase( "Content-Length");
	}
}
