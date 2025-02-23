



#include <cstdint>
#include <string>

#include "net/Slice.h"
#include "url.h"
#include "hdlog.h"


Result<Url> Url::Parse(Slice<char>& uri)
{
    Url url;
    size_t pos = 0;

    // 解析协议
    size_t protocolEnd = uri.find( "://" , 3);
    if (protocolEnd != uri.len ) {
        url.mothed = uri.subslice(0, protocolEnd);
        pos = protocolEnd + 3;
    }


    // 解析用户名和密码
    size_t at_pos = uri.find('@', pos);
    if (at_pos != uri.size()) {
        size_t colon_pos = uri.find(':', pos);

        if (colon_pos != uri.size() && colon_pos < at_pos) {
            url.name = uri.subslice(pos, colon_pos - pos);
            url.passwd = uri.subslice(colon_pos + 1, at_pos - colon_pos - 1);
        }
        else {
            url.name = uri.subslice(pos, at_pos - pos);
        }
        pos = at_pos + 1;
    }

    // 解析主机名和端口号
    size_t colon_pos = uri.find(':', pos);
    size_t path_start = uri.find('/', pos);
    if (colon_pos != std::string::npos && colon_pos < path_start) {
        url.host = uri.subslice(pos, colon_pos - pos);
        auto port_slice = uri.subslice(colon_pos + 1, path_start - colon_pos - 1);

        if( port_slice.is_legal() && port_slice.size() <= 5 ) {
            uint16_t _value = 0;

            for( size_t index = 0; index < port_slice.size(); index++) {
                if( port_slice[index] < 0 || port_slice[index] > 9) {
                    _value = 0;
                    break;
                }

                _value = _value * 10;
                _value = _value + port_slice[index] - '0';
            }

            url.port = _value;
        }

        pos = path_start;
    }
    else {
        url.host = uri.subslice(pos, path_start - pos);
        url.port = 0;
        pos = path_start;
    }

    // 解析路径
    if (pos != uri.size()) {
        url.path = uri.subslice(pos);
    }

    return Ok(url);
}

Result<Url> Url::Parse(string uri)
{
    auto slice = Slice<char>(  (char*)uri.c_str(), uri.size());
    return Parse( slice);
}

