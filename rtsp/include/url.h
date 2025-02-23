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
    Slice<char> mothed = {nullptr, 0};
    Slice<char> name= {nullptr, 0};
    Slice<char> passwd= {nullptr, 0};
    Slice<char> host= {nullptr, 0};
    uint16_t    port = 0;
    Slice<char> path = {nullptr, 0};
    Slice<char> param = {nullptr, 0};

    Slice<char> url= {nullptr, 0};
public:
    static Result<Url> Parse(string uri);
    static Result<Url> Parse(Slice<char>& uri);


    uint64_t Port();
    string Method();

    string Path();
    string Hostname();

    void setHost(string);

    string String();

} ;

#endif //EASYDARWIN_URL_H
