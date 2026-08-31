#include <iostream>
#include "DroneMessage.h" // 引入你的新合同

int main() {
    std::cout << "========== 主控台 (Controller) 启动 ==========" << std::endl;

    // 1. 声明数据结构，证明包含成功
    SharedMessage::DroneCmd current_cmd;
    current_cmd.cmd_id = 1;
    current_cmd.cmd_type = SharedMessage::CMD_FORMATION;
    current_cmd.target_x = 10.0;
    current_cmd.target_y = 10.0;
    current_cmd.expected_count = 5;

    std::cout << "准备广播队形命令，目标机群数量: " << current_cmd.expected_count << std::endl;

    // TODO给组员: 在这里初始化 ZR-DDS 参与者
    // TODO给组员: 创建 DataWriter 发送 Topic_Drone_Command
    // TODO给组员: 创建 DataReader 监听 Topic_Drone_State

    // 模拟运行循环
    while (true) {
        // 保持进程不死
    }

    return 0;
}