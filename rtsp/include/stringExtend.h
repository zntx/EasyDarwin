//
// Created by zhangyuzhu8 on 2025/1/20.
//

#ifndef EASYDARWIN_STRINGEXTEND_H
#define EASYDARWIN_STRINGEXTEND_H




//
// Created by zhangyuzhu8 on 2025/1/20.
//
#include <iostream>
#include <vector>
#include <map>
#include <string>

using namespace std;

//É¾³ý¿Õ°××Ö·û
std::string& StringTrim(std::string &s, const string& split = " ");

// Ê¹ÓÃ×Ö·û·Ö¸î
vector<string> StringSplit(const string& str, const char split );

// Ê¹ÓÃ×Ö·û´®·Ö¸î
vector<string> StringSplit(const string& str, const string& splits );

// ·Ö¸î key value
std::map<std::string, std::string> mappify2(std::string const& s);

// ·Ö¸î key value
bool mappify(std::string const& key_pos, string& key, string& value, const string& split = "=");

// ×Ö·û´®ÊÇ·ñÒÔÖ¸¶¨×Ö·û´®¿ªÍ·
bool StringHasPrefix(std::string& str, std::string prefix);

//ÅÐ¶Ï×Ö·û´®ÊÇÊý×Ö
bool string_isDigit(const std::string & str);

#endif //EASYDARWIN_STRINGEXTEND_H
