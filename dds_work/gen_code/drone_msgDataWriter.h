#ifndef drone_msgDataWriter_h__
#define drone_msgDataWriter_h__
/*************************************************************/
/*           此文件由编译器生成，请勿随意修改                */
/*************************************************************/

#include "drone_msg.h"
#include "ZRDDSDataWriter.h"


typedef DDS::ZRDDSDataWriter<DroneCmd> DroneCmdDataWriter;


typedef DDS::ZRDDSDataWriter<DroneState> DroneStateDataWriter;

#endif

