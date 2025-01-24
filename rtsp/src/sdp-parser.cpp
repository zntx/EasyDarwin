
#include <map>
#include <string>
#include "net.h"
#include "stringExtend.h"

using namespace std;



class SDPInfo {
	string       AVType;             
	string       Codec ;             
	int          TimeScale ;         
	string       Control;            
	int          Rtpmap;             
	Slice<char>  Config;             
	[][]byte     SpropParameterSets; 
	int          PayloadType ;       
	int          SizeLength;         
	int          IndexLength ;       
};

map<string, SDPInfo>  ParseSDP(string sdpRaw) 
{
	map<string, SDPInfo>  sdpMap ;
	SDPInfo* info;

    auto lines =  StringSplit( sdpRaw, "\n");
	for ( auto & line : lines) {
		StringTrim(line);

        string key;
        string value;

		if (mappify(line, key, value , "=")) {
			//fields := strings.SplitN(key, " ", 2)

            string item1;
            string item2;
            auto fields = mappify(key, item1, item2 , " ");
            
			//switch typeval[0] {
			if( key == "m") {
				if (fields > 0 ){
					if( item1 ==  "audio" || item1 ==  "video" ){
						sdpMap[item1] = SDPInfo{AVType: item1};
						info = &sdpMap[item1];

						auto mfields = StringSplit(item2, " ");
						if (mfields.size() >= 3) {
							info.PayloadType = strconv.Atoi(mfields[2]);
						}
					}
				}
            }
			else if( key == "a" ){
				if (info != nullptr ){
					for _, field := range fields {
						keyval := strings.SplitN(field, ":", 2)
						if len(keyval) >= 2 {
							key := keyval[0]
							val := keyval[1]
							switch key {
							case "control":
								info.Control = val
							case "rtpmap":
								info.Rtpmap, _ = strconv.Atoi(val)
							}
						}
						keyval = strings.Split(field, "/")
						if len(keyval) >= 2 {
							key := keyval[0]
							switch key {
							case "MPEG4-GENERIC":
								info.Codec = "aac"
							case "H264":
								info.Codec = "h264"
							case "H265":
								info.Codec = "h265"
							}
							if i, err := strconv.Atoi(keyval[1]); err == nil {
								info.TimeScale = i
							}
						}
						keyval = strings.Split(field, ";")
						if len(keyval) > 1 {
							for _, field := range keyval {
								keyval := strings.SplitN(field, "=", 2)
								if len(keyval) == 2 {
									key := strings.TrimSpace(keyval[0])
									val := keyval[1]
									switch key {
									case "config":
										info.Config, _ = hex.DecodeString(val)
									case "sizelength":
										info.SizeLength, _ = strconv.Atoi(val)
									case "indexlength":
										info.IndexLength, _ = strconv.Atoi(val)
									case "sprop-parameter-sets":
										fields := strings.Split(val, ",")
										for _, field := range fields {
											val, _ := base64.StdEncoding.DecodeString(field)
											info.SpropParameterSets = append(info.SpropParameterSets, val)
										}
									}
								}
							}
						}
					}
				}
            }

		}
	}
	return sdpMap ;
}
