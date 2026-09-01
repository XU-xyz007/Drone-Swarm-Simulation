#include <iostream>
#include "DroneMessage.h"

int main() {
    std::cout << "========== 无人机节点 (Drone) 启动 ==========" << std::endl;

    // 1. 声明自身状态数据
    SharedMessage::DroneState my_state;
    my_state.drone_id = 3; // 假设这是3号机
    my_state.status = SharedMessage::ST_IDLE;
    my_state.x = 0.0;
    my_state.y = 0.0;

    std::cout << "我是无人机 ID: " << my_state.drone_id << "，当前待命。" << std::endl;

    // TODO给组员: 在这里初始化 ZR-DDS 参与者
    // TODO给组员: 创建 DataReader 监听 Topic_Drone_Command
    // TODO给组员: 创建 DataWriter 发送 Topic_Drone_State (10Hz)

    // 模拟运行循环
    while (true) {
        // 保持进程不死
    }

    return 0;
}