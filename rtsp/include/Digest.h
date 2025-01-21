//
// Created by zhangyuzhu8 on 2024/12/31.
//

#ifndef APP_DIGEST_H
#define APP_DIGEST_H

#include <string>

using namespace std;

class Digest{
public:
    Digest();
    ~Digest();

    static Digest* From(const string& str);
    void setNamePasswd(const string& _name, const string& _passwd);
    string response(const string& uri,  const string& method);


    string to_string(const string& uri,  const string& method);

    static bool From(const string& str,  const string& passwd,  const string& method);


private:
    bool stale = false;
    string realm;
    string nonce;
    string name;
    string passwd;
};


#endif //APP_DIGEST_H
