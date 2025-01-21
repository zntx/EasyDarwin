


#include "net/Slice.h"
#include <cstdint>

#define MAX_RTSP_URL  256 //rtsp 的url最大长度

#define MAX_PROFILE_LEN 64 // onvif中含有profile字段的profile参数的最大长度


class Url{
    Slice<char> mothed;
    Slice<char> name;
    Slice<char> passwd;        
    Slice<char> host;
    uint16_t    port;

    Slice<char> url;
public:
    uint64_t Port();
    string Method();

} ;






/**	@fn    parse_rtsp_url 
 *	@brief 根据URL地址分析出用户要点播的通道号，主子码流，是回放还是预览情况 是否是多播发送
 *	@param pURL(URL地址)
 *	@param stream_id(码流id)，0表示子码流1表示主码流
 *	@param chan_id(通道号) 默认都是1
 *	@param playback:是否是回放
 *	@param multicast :是否是多播发送
 *	@return 无
 */
int Parse(Slice<char> uri)
{
	if (url.no_legal())
	{
		RTSP_ERR("parse_rtsp_url:input parameter is error");
		return ret;
	}

    auto Opos = uri.find('?');
    auto pos = Opos.unwrap_or(uri.size()); 
    
    auto base = uri.slice(0, pos).unwrap();
    //auto Oparam = uri.slice(pos+1 );

    auto pos_o = base.find(Slice<char>("://", 3);
    if( pos_o.no_legal() ) {
        return false;
    }

    pos =  pos_o.unwrap();

    if( pos + 3 > base.size() - 1 ) {
        return false;
    }

    auto methd = base.slice( 0, pos).unwrap();
    base =  base.slice( pos + 3 + 1).unwrap();
    
    pos_o = base1.find( "@");
    if( !pos_o.is_empty() )
    {   
        pos =  pos_o.unwrap();

        base =  base.slice( pos + 1).unwrap();
    }

    pos_o = base.find( "/");;
    if( !pos_o.is_empty() )
    {
        pos =  pos_o.unwrap();

        base =  base.slice( pos + 1).unwrap();
    }
    else
    {
        uri->port  = 554;
    }

















    Url  url;

	return 0;
}