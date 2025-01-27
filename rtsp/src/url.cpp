


#include "net/Slice.h"
#include <cstdint>
#include <string>

#include "url.h"
#include "hdlog.h"


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
	if (uri.no_legal())
	{
		logger::Printf("parse_rtsp_url:input parameter is error");
		return false;
	}

    auto pos_opt = uri.find('?');
    auto pos = pos_opt.unwrap_or(uri.size());
    
    auto base = uri.slice(0, pos).unwrap();
    //auto Oparam = uri.slice(pos+1 );
    //rtsp://admin:test1357@192.168.10.1:554/asddsa/asdasdsa

    auto pos_o = base.find(Slice<char>("://", 3));
    if( pos_o.is_empty() ) {
        return false;
    }

    pos =  pos_o.unwrap();

    if( pos + 3 > base.size() - 1 ) {
        return false;
    }

    auto method = base.slice(0, pos).unwrap();
    base =  base.slice( pos + 3 ).unwrap();
    
    pos_o = base.find('@');
    if( !pos_o.is_empty() )
    {   
        pos =  pos_o.unwrap();

        base =  base.slice( pos + 1).unwrap();
    }

    pos_o = base.find( '/');;
    if( !pos_o.is_empty() )
    {
        pos =  pos_o.unwrap();

        base =  base.slice( pos + 1).unwrap();
    }
    else
    {
        //uri->port  = 554;
    }

















    Url  url;

	return 0;
}