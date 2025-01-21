//
// Created by zhangyuzhu8 on 2024/12/31.
//

#include <vector>
#include <iostream>
#include <map>
#include <string>
#include <sstream>
#include "Digest.h"
//#include "rtspdbg.h"
//#include "authentication.h"
#include "hdlog.h"
#include "MD5.h"


#define HASHLEN 16


/**@fn BOOL void compute_MD5Data
 *	@brief 计算产生MD5摘要数据
 *	@param unsigned char *data MD5要产生的原始数据
 *	@param unsigned int len  原始数据长度
 *	@param char *buf  MD5数据输出缓冲
 *	@return 无		
 */
static void compute_MD5_data (const unsigned char *data, unsigned int datalen, char *buf, unsigned int buflen)
{
	MD5_CTX ctx;
	unsigned char hash[16];
	unsigned short i;
	unsigned char j;
	if(nullptr == data || nullptr == buf || buflen > (2*HASHLEN+1))
	{
		return;
	}
	memset(hash, 0, sizeof(hash));
	(void)MD5_Init(&ctx);
	(void)MD5_Update(&ctx,  (uint8_t*)data, datalen);
	(void)MD5_Final(hash, &ctx);

	for (i = 0; i < 16; i++) 
	{
		j = (hash[i] >> 4) & 0xf;
		if (j <= 9)
		{
			buf[i*2] = (j + '0');
		}
		else
		{
			buf[i*2] = (j + 'a' - 10);
		}

		j = hash[i] & 0xf;
		if (j <= 9)
		{
			buf[i*2+1] = (j + '0');
		}
		else
		{
			buf[i*2+1] = (j + 'a' - 10);
		}
	}
	buf[2*i] = '\0';
	return;
}




Digest::Digest()
{

}

Digest::~Digest()
{

}

Digest* Digest::From(const string& str )
{
    vector<string> strList;
    auto* digest = new (std::nothrow) Digest;

    string type;
    string attr;
    mappify(str, type, attr, " ");

    std::cout << "type" << type << " " << std::endl;
    std::cout << "attr" << attr << " " << std::endl;

    StringSplit(attr, ",", strList);

    for (const auto& s : strList)
    {
        std::cout << s << " " << std::endl;
        string key;
        string value;
        mappify(s, key, value);
        if( key.empty() || value.empty())
        {
            std::cout <<  "key.empty()" << "value.empty() " << std::endl;
            continue;
        }

        if( key == "realm")
        {
            digest->realm = trim(value, "\"");
        }
        else if( key == "nonce")
        {
            digest->nonce = trim(value, "\"");
        }
        else if( key == "stale")
        {
            digest->stale = value == "\"TRUE\"";
        }
        else
        {
            ("not support %s", key.c_str());
        }
    }

    return digest;
}

void Digest::setNamePasswd(const string& _name, const string& _passwd)
{
    this->name = _name;
    this->passwd = _passwd;
}


string Digest::response(const string& uri, const string& method)
{
    char ha1buf[2*HASHLEN+1];
    char ha2buf[2*HASHLEN+1];
    char _response[2*HASHLEN+1];

    auto ha1data =  this->name + ":" + this->realm + ":" + this->passwd;
    compute_MD5_data((unsigned char*)ha1data.c_str(), ha1data.size(), ha1buf, sizeof(ha1buf));

    auto ha2data = method + ":" + uri;
    compute_MD5_data((unsigned char*)ha2data.c_str(), ha2data.size(), ha2buf, sizeof(ha2buf));

    auto digestdata =  string(ha1buf) + ":" + this->nonce + ":" + ha2buf;
    compute_MD5_data((unsigned char*)digestdata.c_str(), digestdata.size(), _response, 2*HASHLEN+1);

    return std::string(_response);
}

string Digest::to_string(const string& method, const string& uri)
{
    auto tmp = "Digest username=\""+ this->name + "\",";
    tmp += " realm=\"" + this->realm + "\",";
    tmp += " nonce=\"" + this->nonce + "\",";
    tmp += " uri=\"" + uri + "\",";
    tmp += " response=\"" + this->response(uri, method) + "\",";

    return tmp;
}

bool Digest::From(const string& str,  const string& passwd,  const string& method)
{
    vector<string> strList;
    string response;
    string uri;
    Digest digest;

    StringSplit(str, ",", strList);

    for (const auto& s : strList)
    {
        std::cout << s << " " << std::endl;
        string key;
        string value;
        mappify(s, key, value);
        if( key.empty() || value.empty())
        {
            continue;
        }

        if( key == "realm")
        {
            digest.realm = value;
        }
        else if( key == "nonce")
        {
            digest.nonce = value;
        }
        else if( key == "stale")
        {
            digest.stale = value == "TRUE";
        }
        else if( key == "username")
        {
            digest.name = value ;
        }
        else if( key == "uri")
        {
            uri = value ;
        }
        else if( key == "response")
        {
            response = value ;
        }
        else
        {
            logger::error("not support {}", key );
        }
        /*
         * username="admin", realm="44a642e85432", nonce="d152556554b31e4dd1bbec4cc53f4596",
         * uri="rtsp://10.112.217.117:554/Streaming/Channels/101",
         * response="85ba71c852f0839e493be82de6427af2"
         * */
    }

    digest.setNamePasswd("", passwd);

    auto _response = digest.response(uri, method);

    return _response == response;
}