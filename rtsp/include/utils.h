//
// Created by zhangyuzhu8 on 2025/1/21.
//

#ifndef EASYDARWIN_UTL_H
#define EASYDARWIN_UTL_H

#include "ini.hpp"

class utils {
private:
    static inicpp::IniManager ini;
public:

    utils( const std::string &configFileName);
    static inicpp::IniManager& Conf();
    static std::string RandomNumber();
};

inline inicpp::IniManager& utils::Conf()
{
    return ini ;
}





#endif //EASYDARWIN_UTL_H
