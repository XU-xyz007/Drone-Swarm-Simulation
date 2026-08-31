/*************************************************************/
/*           此文件由编译器生成，请勿随意修改                */
/*************************************************************/
#include <stdlib.h>
#include "ZRDDSTypePlugin.h"
#include "DroneMessage.h"
#include "DroneMessageTypeSupport.h"
#include "DroneMessageDataReader.h"
#include "DroneMessageDataWriter.h"
#include "ZRDDSTypeSupport.cpp"

namespace SharedMessage
{

const DDS_Char* DroneState_TYPENAME = "DroneState";
DDSTypeSupportImpl(DroneStateTypeSupport, DroneState, DroneState_TYPENAME);


const DDS_Char* DroneCmd_TYPENAME = "DroneCmd";
DDSTypeSupportImpl(DroneCmdTypeSupport, DroneCmd, DroneCmd_TYPENAME);

}
