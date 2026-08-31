#ifndef DroneMessageDataReader_h__
#define DroneMessageDataReader_h__
/*************************************************************/
/*           此文件由编译器生成，请勿随意修改                */
/*************************************************************/

#include "DroneMessage.h"
#include "ZRDDSDataReader.h"

namespace SharedMessage
{
typedef struct DroneStateSeq DroneStateSeq;

typedef DDS::ZRDDSDataReader<DroneState, DroneStateSeq> DroneStateDataReader;

typedef struct DroneCmdSeq DroneCmdSeq;

typedef DDS::ZRDDSDataReader<DroneCmd, DroneCmdSeq> DroneCmdDataReader;

}
#endif

