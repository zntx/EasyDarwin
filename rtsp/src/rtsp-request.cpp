#include "rtsp-request.h"

#include <utility>
#include "fmt/include/fmt/core.h"
#include "stringExtend.h"
#include "hdlog.h"

Request* NewRequest(string content ) 
{
	//lines := strings.Split(strings.TrimSpace(content), "\r\n")
    auto lines = string_Split(string_Trim(content), "\r\n");
	if ( lines.empty()) {
        logger::error("invalid rtsp request, line[0] " );
		return nullptr;
	}
    auto items = string_Split(string_Trim(lines[0]), " ");
	if (items.size() < 3) {
        logger::error("invalid rtsp request, line[0] {}", lines[0]);
		return nullptr;
	}
	if (!StringHasPrefix(items[2], string("RTSP") )) {
		logger::error("invalid rtsp request, line[0] {}", lines[0]);
		return nullptr;
	}

    map<string, string> header;
	for ( size_t i = 1; i < lines.size(); i++) {
		auto line = string_Trim(lines[i]);
        string key;
        string value;
        if (mappify(line, key, value, ":")) {
			continue ;
		}
		header[key] = value;
	}

	auto request = new(std::nothrow) Request( items[0],  items[1], items[2],   content);
    request->Header = header;
    return request;
}

Request::Request( string method, string url, string version, string conten)
{
    Method =  std::move(method);
    URL =     std::move(url);
    Version = std::move(version);
    //Header =  header;
    Content = std::move(conten);
    Body =    "";
}


string Request::String() 
{
	auto str = fmt::format("{} {} {}\r\n", this->Method, this->URL, this->Version);
	for (auto & iter : this->Header) {
        str += fmt::format("{}: {}\r\n",  iter.first, iter.second);
    }
	str += "\r\n";
	str += this->Body;
	return str;
}

int Request::GetContentLength()
{
//    auto length = this->Header["Content-Length"];
//	v, err := strconv.ParseInt(this->Header["Content-Length"], 10, 64)
    auto iter = this->Header.find("Content-Length");
	if ( iter == this->Header.end()) {
		return 0;
	}

    if(!string_isDigit(iter->second)) {
        return 0;
	}

    return std::stoi(iter->second);
}
