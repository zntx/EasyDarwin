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
    // ѡ����������棬����ʹ�� mt19937
    std::mt19937 engine(std::random_device{}());

    // ����һ����Χ�� [1, 100] �ľ��ȷֲ�������
    std::uniform_int_distribution<int> dist(1, 1000);

    // ���������
    int randomNumber = dist(engine);
    std::cout << "Random integer in [1, 100]: " << randomNumber << std::endl;

    return std::to_string(randomNumber);
}