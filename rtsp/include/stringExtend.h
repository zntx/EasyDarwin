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

//ɾ���հ��ַ�
std::string& StringTrim(std::string &s, const string& split = " ");

// ʹ���ַ��ָ�
vector<string> StringSplit(const string& str, const char split );

// ʹ���ַ����ָ�
vector<string> StringSplit(const string& str, const string& splits );

// �ָ� key value
std::map<std::string, std::string> mappify2(std::string const& s);

// �ָ� key value
bool mappify(std::string const& key_pos, string& key, string& value, const string& split = "=");

// �ַ����Ƿ���ָ���ַ�����ͷ
bool StringHasPrefix(std::string& str, std::string prefix);

//�ж��ַ���������
bool string_isDigit(const std::string & str);

#endif //EASYDARWIN_STRINGEXTEND_H
