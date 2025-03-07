

#include <string>
#include "net.h"
#include "stringExtend.h"
#include "sdp-parser.h"



map<string, SDPInfo>  ParseSDP(const string& sdp_raw)
{
	map<string, SDPInfo>  sdpMap ;
	SDPInfo* info = nullptr;

    auto lines = string_Split(sdp_raw, "\n");
	for ( auto & line : lines) {
        string_TrimSpace(line);
        auto typeval = string_SplitN(line, "=", 2);
        if (typeval.size() == 2 ){
            auto fields = string_SplitN(typeval[1], " ", 2);

			//switch typeval[0] {
			if( typeval[0] == "m") {
				if (fields.size() > 0 ){
					if( fields[0] ==  "audio" || fields[0] ==  "video" ){
                        SDPInfo sdpInfo;
                        sdpInfo.AVType = fields[0];
						sdpMap[fields[0]] = sdpInfo;
						auto index = sdpMap.find(fields[0]);
                        info = &index->second;

						auto mfields = string_Split(fields[1], " ");
						if (mfields.size() >= 3) {
							info->PayloadType = std::stoi(mfields[2]);
						}
					}
				}
            }
			else if( typeval[0] == "a" ){
				if (info != nullptr ){
					for ( auto &&field :fields)  {
						auto keyval = string_SplitN(field, ":", 2);
						if (keyval.size() >= 2 ){
							auto key = keyval[0];
							auto val = keyval[1];
							//switch key {
							if( key == "control")
								info->Control = val;
							if( key == "rtpmap")
								info->Rtpmap  = std::stoi(val);
						}

                        keyval = string_Split(field, "/");
						if (keyval.size() >= 2) {
							auto key = keyval[0];

                            if( key == "MPEG4-GENERIC")
								info->Codec = "aac";
                            if( key == "H264")
								info->Codec = "h264";
                            if( key == "H265")
								info->Codec = "h265";

							if (string_isDigit(keyval[1])) {
								info->TimeScale = std::stoi(keyval[1]);
							}
						}

                        keyval = string_Split(field, ";");
						if (keyval.size() > 1 ) {
							for ( auto &&field : keyval ){
								keyval = string_SplitN(field, "=", 2);
								if (keyval.size() == 2) {
									auto key = string_TrimSpace(keyval[0]);
									auto val = keyval[1];
									//switch key {
//                                    if( key == "config")
//										info->Config  = hex.DecodeString(val);
                                    if( key == "sizelength")
										info->SizeLength = std::atoi(val.c_str());
                                    if( key == "indexlength")
										info->IndexLength = std::atoi(val.c_str());
//                                    if( key == "sprop-parameter-sets") {
//										auto fields = string_Split(val, ",");
//										for (auto &&field : fields ){
//											val, _ := base64.StdEncoding.DecodeString(field);
//											info->SpropParameterSets.append(  val);
//										}
//									}
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
