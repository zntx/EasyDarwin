//
// Created by zhangyuzhu8 on 2025/1/21.
//
#include <iostream>
#include <random>
#include "utils.h"




utils::utils(const std::string &configFileName) :ini( configFileName)
{

}

std::string utils::RandomNumber()
{
    // 选择随机数引擎，这里使用 mt19937
    std::mt19937 engine(std::random_device{}());

    // 生成一个范围在 [1, 100] 的均匀分布的整数
    std::uniform_int_distribution<int> dist(1, 1000);

    // 生成随机数
    int randomNumber = dist(engine);
    std::cout << "Random integer in [1, 100]: " << randomNumber << std::endl;

    return std::to_string(randomNumber);
}