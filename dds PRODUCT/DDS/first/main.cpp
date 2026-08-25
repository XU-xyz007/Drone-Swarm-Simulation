#include <iostream>
// 引入你刚刚生成的 DDS 消息头文件
#include "drone_msg.h" 
#include "drone_msgTypeSupport.h"

int main()
{
    std::cout << "DDS 环境配置完美成功！无人机编队项目准备起飞！" << std::endl;

    // 这里以后会写 DDS 的初始化、发布目标点、订阅坐标等代码

    return 0;
}