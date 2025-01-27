//
// Created by zz on 2025/1/25.
//
#include <iostream>
#include <thread>
#include "net.h"

#if 0
// UDPServer 类定义
class UDPServer2 {
public:
    UDPServer() {
        std::cout << "UDPServer default constructor" << std::endl;
    }

    // 删除拷贝构造函数
    UDPServer(const UDPServer&) = delete;

    // 定义移动构造函数
    UDPServer(UDPServer&& other) noexcept {
        std::cout << "UDPServer move constructor" << std::endl;
    }

    // 定义移动赋值运算符
    UDPServer& operator=(UDPServer&& other) noexcept {
        std::cout << "UDPServer move assignment operator" << std::endl;
        return *this;
    }

    void start() {
        std::cout << "UDPServer started in thread: " << std::this_thread::get_id() << std::endl;
    }
};
#endif
int main() {
    UdpSocket usocket(1);

    auto udp_  = new UdpSocket(1);
    // 创建线程，使用右值引用接收移动后的对象
    std::thread stream([&](UdpSocket&& socket) {
        socket.start();
        udp_->start();
    }, std::move(usocket));

    // 等待线程完成
    stream.join();

    return 0;
}