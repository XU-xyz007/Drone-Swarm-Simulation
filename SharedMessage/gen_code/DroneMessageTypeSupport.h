#ifndef DroneMessageTypeSupport_h__
#define DroneMessageTypeSupport_h__
/*************************************************************/
/*           此文件由编译器生成，请勿随意修改                */
/*************************************************************/

#include "ZRDDSTypeSupport.h"

namespace SharedMessage
{
DDSTypeSupport(DroneStateTypeSupport, DroneState);
DDSTypeSupport(DroneCmdTypeSupport, DroneCmd);
}
#endif

