#ifndef DroneMessageDataWriter_h__
#define DroneMessageDataWriter_h__
/*************************************************************/
/*           此文件由编译器生成，请勿随意修改                */
/*************************************************************/

#include "DroneMessage.h"
#include "ZRDDSDataWriter.h"

namespace SharedMessage
{

typedef DDS::ZRDDSDataWriter<DroneState> DroneStateDataWriter;


typedef DDS::ZRDDSDataWriter<DroneCmd> DroneCmdDataWriter;

}
#endif

