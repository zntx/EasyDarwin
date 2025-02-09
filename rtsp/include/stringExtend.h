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

//删除空白字符
std::string& string_Trim(std::string &s, const string& split = " ");

// 使用字符分割
vector<string> string_Split(const string& str, const char split );

// 使用字符串分割
vector<string> string_Split(const string& str, const string& splits );

// 分割 key value
std::map<std::string, std::string> mappify2(std::string const& s);

// 分割 key value
bool mappify(std::string const& key_pos, string& key, string& value, const string& split = "=");

// 字符串是否以指定字符串开头
bool StringHasPrefix(std::string& str, std::string prefix);

//判断字符串是数字
bool string_isDigit(const std::string & str);

// 字符串是转小写
string& string_ToLower( std::string& str);

//字符串是否已指定的字符串开头
bool string_start_with(const std::string& str, const std::string& prefix);

#endif //EASYDARWIN_STRINGEXTEND_H
