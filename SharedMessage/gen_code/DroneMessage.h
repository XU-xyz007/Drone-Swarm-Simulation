#ifndef DroneMessage_H_
#define DroneMessage_H_

/*************************************************************/
/*           此文件由编译器生成，请勿随意修改                */
/*************************************************************/
#include "OsResource.h"
#include "ZRSequence.h"
#include "TypeCode.h"
#include "CDRStream.h"
#include "ZRDDSCppWrapper.h"
#include "ZRBuiltinTypes.h"

typedef struct ZRMemPool ZRMemPool;


namespace SharedMessage
{
typedef enum DroneStatus
{
    ST_IDLE = 0,
    ST_TAKEOFF = 1,
    ST_CRUISE = 2,
    ST_LANDING = 3,
    ST_LANDED = 4,
    ST_FAULT = 5,
    ST_LOST = 6
} DroneStatus;// @BitBound(32)

DDS_USER_SEQUENCE_CPP(DroneStatusSeq, DroneStatus);

DDS::TypeCode* DroneStatusGetTypeCode();

TypeCodeHeader* DroneStatusGetInnerTypeCode();

typedef enum DroneCmdType
{
    CMD_TAKEOFF = 0,
    CMD_FORMATION = 1,
    CMD_LAND = 2,
    CMD_EMERGENCY = 3
} DroneCmdType;// @BitBound(32)

DDS_USER_SEQUENCE_CPP(DroneCmdTypeSeq, DroneCmdType);

DDS::TypeCode* DroneCmdTypeGetTypeCode();

TypeCodeHeader* DroneCmdTypeGetInnerTypeCode();

typedef struct DroneState
{
    DDS_ULong drone_id; // @ID(0) @Key
    SharedMessage::DroneStatus status; // @ID(1)
    DDS_Double x; // @ID(2)
    DDS_Double y; // @ID(3)
    DDS_ULong current_cmd_id; // @ID(4)
} DroneState; // @Extensibility(EXTENSIBLE)

DDS_USER_SEQUENCE_CPP(DroneStateSeq, DroneState);

// 用户使用接口
DDS_Boolean DroneStateInitialize(
    DroneState* self);

DDS_Boolean DroneStateInitializeEx(
    DroneState* self,
    ZRMemPool* pool,
    DDS_Boolean allocateMemory);

void DroneStateFinalize(
    DroneState* self);

void DroneStateFinalizeEx(
    DroneState* self,
    ZRMemPool* pool,
    DDS_Boolean deletePointers);

DDS_Boolean DroneStateCopy(
    DroneState* dst,
    const DroneState* src);

DDS_Boolean DroneStateCopyEx(
    DroneState* dst,
    const DroneState* src,
    ZRMemPool* pool);

void DroneStatePrintData(
    const DroneState* sample);

DDS::TypeCode* DroneStateGetTypeCode();

// 底层使用函数
DroneState* DroneStateCreateSample(
    ZRMemPool* pool,
    DDS_Boolean allocMutable);

void DroneStateDestroySample(
    ZRMemPool* pool,
    DroneState* sample);

DDS_ULong DroneStateGetSerializedSampleMaxSize();

DDS_ULong DroneStateGetSerializedSampleSize(
    const DroneState* sample,
    DDS_ULong currentAlignment);

DDS_Long DroneStateSerialize(
    const DroneState* sample,
    CDRSerializer* cdr);

DDS_Long DroneStateDeserialize(
    DroneState* sample,
    CDRDeserializer* cdr,
    ZRMemPool* pool);

DDS_ULong DroneStateGetSerializedKeyMaxSize();

DDS_ULong DroneStateGetSerializedKeySize(
    const DroneState* sample,
    DDS_ULong currentAlignment);

DDS_Long DroneStateSerializeKey(
    const DroneState* sample,
    CDRSerializer* cdr);

DDS_Long DroneStateDeserializeKey(
    DroneState* sample,
    CDRDeserializer* cdr,
    ZRMemPool* pool);

DDS_Long DroneStateGetKeyHash(
    const DroneState* sample,
    CDRSerializer* cdr,
    DDS::KeyHash_t* result);

DDS_Boolean DroneStateHasKey();

TypeCodeHeader* DroneStateGetInnerTypeCode();

#ifdef _ZRDDS_INCLUDE_ONSITE_DESERILIZE
DDS_Boolean DroneStateNoSerializingSupported();

DDS_ULong DroneStateFixedHeaderLength();

DDS_Long DroneStateOnSiteDeserialize(CDRDeserializer* cdr,
    DroneState* sample,
    DDS_ULong offset,
    DDS_ULong totalSize,
    DDS_Char* payload,
    DDS_ULong payloadLen,
    DDS_ULong fixedHeaderLen);

#endif/*_ZRDDS_INCLUDE_ONSITE_DESERILIZE*/

#ifdef _ZRDDS_INCLUDE_NO_SERIALIZE_MODE
DDS_Char* DroneStateLoanSampleBuf(DroneState* sample, DDS_Boolean takeBuffer);

void DroneStateReturnSampleBuf(DDS_Char* sampleBuf);

DDS_Long DroneStateLoanDeserialize(DroneState* sampleBuf,
    CDRDeserializer* cdr,
    DDS_ULong curIndex,
    DDS_ULong totalNum,
    DDS_Char* base,
    DDS_ULong offset,
    DDS_ULong space,
    DDS_ULong fixedHeaderLen);

#endif/*_ZRDDS_INCLUDE_NO_SERIALIZE_MODE*/
typedef struct DroneCmd
{
    DDS_ULong cmd_id; // @ID(0)
    SharedMessage::DroneCmdType cmd_type; // @ID(1)
    DDS_ULong formation_id; // @ID(2)
    DDS_Char* formation_text; // @ID(3) /* maximum length = (255) */
    DDS_Double target_x; // @ID(4)
    DDS_Double target_y; // @ID(5)
    DDS_Double spacing; // @ID(6)
    DDS_ULong expected_count; // @ID(7)
} DroneCmd; // @Extensibility(EXTENSIBLE)

DDS_USER_SEQUENCE_CPP(DroneCmdSeq, DroneCmd);

// 用户使用接口
DDS_Boolean DroneCmdInitialize(
    DroneCmd* self);

DDS_Boolean DroneCmdInitializeEx(
    DroneCmd* self,
    ZRMemPool* pool,
    DDS_Boolean allocateMemory);

void DroneCmdFinalize(
    DroneCmd* self);

void DroneCmdFinalizeEx(
    DroneCmd* self,
    ZRMemPool* pool,
    DDS_Boolean deletePointers);

DDS_Boolean DroneCmdCopy(
    DroneCmd* dst,
    const DroneCmd* src);

DDS_Boolean DroneCmdCopyEx(
    DroneCmd* dst,
    const DroneCmd* src,
    ZRMemPool* pool);

void DroneCmdPrintData(
    const DroneCmd* sample);

DDS::TypeCode* DroneCmdGetTypeCode();

// 底层使用函数
DroneCmd* DroneCmdCreateSample(
    ZRMemPool* pool,
    DDS_Boolean allocMutable);

void DroneCmdDestroySample(
    ZRMemPool* pool,
    DroneCmd* sample);

DDS_ULong DroneCmdGetSerializedSampleMaxSize();

DDS_ULong DroneCmdGetSerializedSampleSize(
    const DroneCmd* sample,
    DDS_ULong currentAlignment);

DDS_Long DroneCmdSerialize(
    const DroneCmd* sample,
    CDRSerializer* cdr);

DDS_Long DroneCmdDeserialize(
    DroneCmd* sample,
    CDRDeserializer* cdr,
    ZRMemPool* pool);

DDS_ULong DroneCmdGetSerializedKeyMaxSize();

DDS_ULong DroneCmdGetSerializedKeySize(
    const DroneCmd* sample,
    DDS_ULong currentAlignment);

DDS_Long DroneCmdSerializeKey(
    const DroneCmd* sample,
    CDRSerializer* cdr);

DDS_Long DroneCmdDeserializeKey(
    DroneCmd* sample,
    CDRDeserializer* cdr,
    ZRMemPool* pool);

DDS_Long DroneCmdGetKeyHash(
    const DroneCmd* sample,
    CDRSerializer* cdr,
    DDS::KeyHash_t* result);

DDS_Boolean DroneCmdHasKey();

TypeCodeHeader* DroneCmdGetInnerTypeCode();

#ifdef _ZRDDS_INCLUDE_ONSITE_DESERILIZE
DDS_Boolean DroneCmdNoSerializingSupported();

DDS_ULong DroneCmdFixedHeaderLength();

DDS_Long DroneCmdOnSiteDeserialize(CDRDeserializer* cdr,
    DroneCmd* sample,
    DDS_ULong offset,
    DDS_ULong totalSize,
    DDS_Char* payload,
    DDS_ULong payloadLen,
    DDS_ULong fixedHeaderLen);

#endif/*_ZRDDS_INCLUDE_ONSITE_DESERILIZE*/

#ifdef _ZRDDS_INCLUDE_NO_SERIALIZE_MODE
DDS_Char* DroneCmdLoanSampleBuf(DroneCmd* sample, DDS_Boolean takeBuffer);

void DroneCmdReturnSampleBuf(DDS_Char* sampleBuf);

DDS_Long DroneCmdLoanDeserialize(DroneCmd* sampleBuf,
    CDRDeserializer* cdr,
    DDS_ULong curIndex,
    DDS_ULong totalNum,
    DDS_Char* base,
    DDS_ULong offset,
    DDS_ULong space,
    DDS_ULong fixedHeaderLen);

#endif/*_ZRDDS_INCLUDE_NO_SERIALIZE_MODE*/
}
#endif
