//
// Created by zhangyuzhu8 on 2025/1/20.
//

#include <algorithm>
#include <cstring>
#include "stringExtend.h"

std::string& string_TrimSpace(std::string &s, const string& split)
{
    if (s.empty())
    {
        return s;
    }

    s.erase(0, s.find_first_not_of(split));
    s.erase(s.find_last_not_of(split) + 1);
    return s;
}

// ʹ���ַ��ָ�
vector<string> string_Split(const string& str, const char split )
{
    vector<string> res;

    if (str.empty())
        return res;
    //���ַ���ĩβҲ����ָ����������ȡ���һ��
    string strs = str + split;
    size_t pos = strs.find(split);

    // ���Ҳ����������ַ��������������� npos
    while (pos != std::string::npos)
    {
        string temp = strs.substr(0, pos);
        string_TrimSpace(temp);
        res.push_back(temp);
        //ȥ���ѷָ���ַ���,��ʣ�µ��ַ����н��зָ�
        strs = strs.substr(pos + 1, strs.size());
        pos = strs.find(split);
    }

    return res;
}
// ʹ���ַ����ָ�
vector<string> string_Split(const string& str, const string& splits )
{
    vector<string> res;

    if (str.empty())
        return res;
    //���ַ���ĩβҲ����ָ����������ȡ���һ��
    string strs = str + splits;
    size_t pos = strs.find(splits);
    int step = splits.size();

    // ���Ҳ����������ַ��������������� npos
    while (pos != strs.npos)
    {
        string temp = strs.substr(0, pos);
        string_TrimSpace(temp);
        res.push_back(temp);
        //ȥ���ѷָ���ַ���,��ʣ�µ��ַ����н��зָ�
        strs = strs.substr(pos + step, strs.size());
        pos = strs.find(splits);
    }

    return res;
}

// ʹ���ַ����ָ�
std::vector<std::string> string_SplitN(const std::string& str, const std::string& splits, size_t number) {
    std::vector<std::string> res;
    auto start = 0;

    if (str.empty())
        return res;

    if (number == 0) {
        std::string temp = str;
        string_TrimSpace(temp);
        res.push_back(temp);
        return res;
    }

    for (auto index = 0; index < number - 1; index++) {
        auto pos = str.find(splits, start);
        if (pos == std::string::npos)
            break;

        std::string temp = str.substr(start, pos - start);
        string_TrimSpace(temp);
        res.push_back(temp);

        start = pos + splits.length();
    }

    std::string temp = str.substr(start);
    string_TrimSpace(temp);
    res.push_back(temp);

    return res;
}


std::map<std::string, std::string> mappify2(std::string const& s)
{
    std::map<std::string, std::string> m;

    std::string::size_type key_pos = 0;
    std::string::size_type key_end;
    std::string::size_type val_pos;
    std::string::size_type val_end;

    while((key_end = s.find(':', key_pos)) != std::string::npos)
    {
        if((val_pos = s.find_first_not_of(": ", key_end)) == std::string::npos)
            break;

        val_end = s.find('\n', val_pos);
        m.emplace(s.substr(key_pos, key_end - key_pos), s.substr(val_pos, val_end - val_pos));

        key_pos = val_end;
        if(key_pos != std::string::npos)
            ++key_pos;
    }

    return m;
}

bool mappify(std::string const& key_pos, string& key, string& value, const string& split )
{
    unsigned int val_pos = key_pos.find(split );
    if( val_pos == std::string::npos)
    {
        std::cout <<  "val_pos 1: " << val_pos << std::endl;
        return false;
    }

    key = key_pos.substr(0, val_pos);
    value = key_pos.substr(val_pos + 1);

    return true;
}



bool string_isPrefix(std::string& str, std::string prefix)
{
    // ���ȼ�� prefix �ĳ����Ƿ�С�ڵ��� str �ĳ���
    if (prefix.length() <= str.length()) {
        // ʹ�� substr ��ȡ str ��ͷ���֣�����Ϊ prefix �ĳ���
        std::string sub = str.substr(0, prefix.length());
        // ʹ�� compare �Ƚ������ַ���
        if (sub.compare(prefix) == 0) {
            std::cout << "The string starts with the prefix." << std::endl;
            return true;
        }
    }

    return false;
}

bool string_isDigit(const std::string & str)
{
    return !str.empty() && std::all_of(str.begin(), str.end(), ::isdigit);
}

string& string_ToLower( std::string& str)
{
    //std::string str = "Hello, World!";
    // ʹ�� std::transform �� std::tolower ת��ΪСд
    std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c) { return std::tolower(c); });
    //std::cout << "Lowercase string: " << str << std::endl;
    return str;
}

bool string_start_with(const std::string& str, const std::string& prefix)
{
    return str.size() >= prefix.size() &&
           str.compare(0, prefix.size(), prefix) == 0;
}

bool string_start_with(const std::string& str, const char* prefix)
{
    return str.size() >= strlen(prefix) &&
           str.compare(0, strlen(prefix), prefix) == 0;
}