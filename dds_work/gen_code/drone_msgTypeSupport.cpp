/*************************************************************/
/*           此文件由编译器生成，请勿随意修改                */
/*************************************************************/
#include <stdlib.h>
#include "ZRDDSTypePlugin.h"
#include "drone_msg.h"
#include "drone_msgTypeSupport.h"
#include "drone_msgDataReader.h"
#include "drone_msgDataWriter.h"
#include "ZRDDSTypeSupport.cpp"


const DDS_Char* DroneCmd_TYPENAME = "DroneCmd";
DDSTypeSupportImpl(DroneCmdTypeSupport, DroneCmd, DroneCmd_TYPENAME);


const DDS_Char* DroneState_TYPENAME = "DroneState";
DDSTypeSupportImpl(DroneStateTypeSupport, DroneState, DroneState_TYPENAME);

