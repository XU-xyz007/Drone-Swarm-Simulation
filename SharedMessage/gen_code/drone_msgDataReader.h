#ifndef drone_msgDataReader_h__
#define drone_msgDataReader_h__
/*************************************************************/
/*           此文件由编译器生成，请勿随意修改                */
/*************************************************************/

#include "drone_msg.h"
#include "ZRDDSDataReader.h"

typedef struct DroneCmdSeq DroneCmdSeq;

typedef DDS::ZRDDSDataReader<DroneCmd, DroneCmdSeq> DroneCmdDataReader;

typedef struct DroneStateSeq DroneStateSeq;

typedef DDS::ZRDDSDataReader<DroneState, DroneStateSeq> DroneStateDataReader;

#endif

