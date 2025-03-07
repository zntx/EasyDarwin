#ifndef SDP_PARSER_H
#define SDP_PARSER_H

#include <map>

using namespace std;



struct SDPInfo {
    string       AVType;
    string       Codec ;
    int          TimeScale =0 ;
    string       Control;
    int          Rtpmap = 0;
    Slice<char>  Config = {nullptr, 0};
    //[][]byte     SpropParameterSets;

    int          PayloadType  = 0;
    int          SizeLength = 0 ;
    int          IndexLength = 0;
};


map<string, SDPInfo>  ParseSDP(const string& sdp_raw);

#endif