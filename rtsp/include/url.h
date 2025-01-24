//
// Created by zhangyuzhu8 on 2025/1/21.
//

#ifndef EASYDARWIN_URL_H
#define EASYDARWIN_URL_H


#include "net.h"
#include <cstdint>
#include <string>


using namespace std;

class Url{
    Slice<char> mothed;
    Slice<char> name;
    Slice<char> passwd;
    Slice<char> host;
    uint16_t    port;
    Slice<char> path;

    Slice<char> url;
public:
    static Result<Url> Parse(string uri);

    uint64_t Port();
    string Method();

    string Path();
    string Hostname();

    void setHost(string);

    string String();

} ;

#endif //EASYDARWIN_URL_H
