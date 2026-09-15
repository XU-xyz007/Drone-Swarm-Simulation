/*************************************************************/
/*           此文件由编译器生成，请勿随意修改                */
/*************************************************************/
#include "ZRMemPool.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "DroneMessage.h"

namespace SharedMessage
{

#define T DroneStatus
#define TSeq DroneStatusSeq

#include "ZRSequence.cpp"
#include "ZRCPlusPlusSequence.cpp"

#undef TSeq
#undef T

DDS::TypeCode* DroneStatusGetTypeCode()
{
    static DDS::TypeCode* s_typeCode = NULL;
    if (s_typeCode != NULL) { return s_typeCode; }

    s_typeCode = DDS::TypeCodeFactory::getInstance().createEnumTC(
        "SharedMessage::DroneStatus",
        32,
        DDS_EXTENSIBLE_EXTENSIBILITY);
    if (s_typeCode == NULL)
    {
        printf("create enum DroneStatus typecode failed.");
        return s_typeCode;
    }
    DDS_Long ret;
    ret = s_typeCode->addMemberToEnum(0, "ST_IDLE", 0);
    if (ret < 0)
    {
        printf("add enum member ST_IDLE failed(%d).", ret);
        DDS::TypeCodeFactory::getInstance().deleteTC(s_typeCode);
        s_typeCode = NULL;
        return NULL;
    }
    ret = s_typeCode->addMemberToEnum(1, "ST_TAKEOFF", 1);
    if (ret < 0)
    {
        printf("add enum member ST_TAKEOFF failed(%d).", ret);
        DDS::TypeCodeFactory::getInstance().deleteTC(s_typeCode);
        s_typeCode = NULL;
        return NULL;
    }
    ret = s_typeCode->addMemberToEnum(2, "ST_CRUISE", 2);
    if (ret < 0)
    {
        printf("add enum member ST_CRUISE failed(%d).", ret);
        DDS::TypeCodeFactory::getInstance().deleteTC(s_typeCode);
        s_typeCode = NULL;
        return NULL;
    }
    ret = s_typeCode->addMemberToEnum(3, "ST_LANDING", 3);
    if (ret < 0)
    {
        printf("add enum member ST_LANDING failed(%d).", ret);
        DDS::TypeCodeFactory::getInstance().deleteTC(s_typeCode);
        s_typeCode = NULL;
        return NULL;
    }
    ret = s_typeCode->addMemberToEnum(4, "ST_LANDED", 4);
    if (ret < 0)
    {
        printf("add enum member ST_LANDED failed(%d).", ret);
        DDS::TypeCodeFactory::getInstance().deleteTC(s_typeCode);
        s_typeCode = NULL;
        return NULL;
    }
    ret = s_typeCode->addMemberToEnum(5, "ST_FAULT", 5);
    if (ret < 0)
    {
        printf("add enum member ST_FAULT failed(%d).", ret);
        DDS::TypeCodeFactory::getInstance().deleteTC(s_typeCode);
        s_typeCode = NULL;
        return NULL;
    }
    ret = s_typeCode->addMemberToEnum(6, "ST_LOST", 6);
    if (ret < 0)
    {
        printf("add enum member ST_LOST failed(%d).", ret);
        DDS::TypeCodeFactory::getInstance().deleteTC(s_typeCode);
        s_typeCode = NULL;
        return NULL;
    }
    ret = s_typeCode->addMemberToEnum(7, "ST_HOVER", 7);
    if (ret < 0)
    {
        printf("add enum member ST_HOVER failed(%d).", ret);
        DDS::TypeCodeFactory::getInstance().deleteTC(s_typeCode);
        s_typeCode = NULL;
        return NULL;
    }
    ret = s_typeCode->addMemberToEnum(8, "ST_IN_TRANSIT", 8);
    if (ret < 0)
    {
        printf("add enum member ST_IN_TRANSIT failed(%d).", ret);
        DDS::TypeCodeFactory::getInstance().deleteTC(s_typeCode);
        s_typeCode = NULL;
        return NULL;
    }
    ret = s_typeCode->addMemberToEnum(9, "ST_FALLBACK_CATCHING", 9);
    if (ret < 0)
    {
        printf("add enum member ST_FALLBACK_CATCHING failed(%d).", ret);
        DDS::TypeCodeFactory::getInstance().deleteTC(s_typeCode);
        s_typeCode = NULL;
        return NULL;
    }
    return s_typeCode;
}

TypeCodeHeader* DroneStatusGetInnerTypeCode()
{
#ifdef _ZRDDS_INCLUDE_TYPECODE
    DDS::TypeCode* userTypeCode = DroneStatusGetTypeCode();
    if (userTypeCode == NULL) { return NULL; }
    return userTypeCode->getImpl();
#else
    return NULL;
#endif
}

#define T DroneCmdType
#define TSeq DroneCmdTypeSeq

#include "ZRSequence.cpp"
#include "ZRCPlusPlusSequence.cpp"

#undef TSeq
#undef T

DDS::TypeCode* DroneCmdTypeGetTypeCode()
{
    static DDS::TypeCode* s_typeCode = NULL;
    if (s_typeCode != NULL) { return s_typeCode; }

    s_typeCode = DDS::TypeCodeFactory::getInstance().createEnumTC(
        "SharedMessage::DroneCmdType",
        32,
        DDS_EXTENSIBLE_EXTENSIBILITY);
    if (s_typeCode == NULL)
    {
        printf("create enum DroneCmdType typecode failed.");
        return s_typeCode;
    }
    DDS_Long ret;
    ret = s_typeCode->addMemberToEnum(0, "CMD_TAKEOFF", 0);
    if (ret < 0)
    {
        printf("add enum member CMD_TAKEOFF failed(%d).", ret);
        DDS::TypeCodeFactory::getInstance().deleteTC(s_typeCode);
        s_typeCode = NULL;
        return NULL;
    }
    ret = s_typeCode->addMemberToEnum(1, "CMD_FORMATION", 1);
    if (ret < 0)
    {
        printf("add enum member CMD_FORMATION failed(%d).", ret);
        DDS::TypeCodeFactory::getInstance().deleteTC(s_typeCode);
        s_typeCode = NULL;
        return NULL;
    }
    ret = s_typeCode->addMemberToEnum(2, "CMD_LAND", 2);
    if (ret < 0)
    {
        printf("add enum member CMD_LAND failed(%d).", ret);
        DDS::TypeCodeFactory::getInstance().deleteTC(s_typeCode);
        s_typeCode = NULL;
        return NULL;
    }
    ret = s_typeCode->addMemberToEnum(3, "CMD_EMERGENCY", 3);
    if (ret < 0)
    {
        printf("add enum member CMD_EMERGENCY failed(%d).", ret);
        DDS::TypeCodeFactory::getInstance().deleteTC(s_typeCode);
        s_typeCode = NULL;
        return NULL;
    }
    return s_typeCode;
}

TypeCodeHeader* DroneCmdTypeGetInnerTypeCode()
{
#ifdef _ZRDDS_INCLUDE_TYPECODE
    DDS::TypeCode* userTypeCode = DroneCmdTypeGetTypeCode();
    if (userTypeCode == NULL) { return NULL; }
    return userTypeCode->getImpl();
#else
    return NULL;
#endif
}

#define T DroneState
#define TSeq DroneStateSeq
#define TINITIALIZE DroneStateInitializeEx
#define TFINALIZE DroneStateFinalizeEx
#define TCOPY DroneStateCopyEx

#include "ZRSequence.cpp"
#include "ZRCPlusPlusSequence.cpp"

#undef TCOPY
#undef TFINALIZE
#undef TINITIALIZE
#undef TSeq
#undef T

DDS_Boolean DroneStateInitialize(DroneState* self)
{
    return DroneStateInitializeEx(self, NULL, true);
}

void DroneStateFinalize(DroneState* self)
{
    DroneStateFinalizeEx(self, NULL, true);
}

DDS_Boolean DroneStateCopy(
    DroneState* dst,
    const DroneState* src)
{
    return DroneStateCopyEx(dst, src, NULL);
}

DroneState* DroneStateCreateSample(
    ZRMemPool* pool,
    DDS_Boolean allocMutable)
{
    DroneState* newSample = (DroneState*)ZRMalloc(pool, sizeof(DroneState));
    if (newSample == NULL)
    {
        printf("malloc for DroneState failed.");
        return NULL;
    }
    if (!DroneStateInitializeEx(newSample, pool, allocMutable))
    {
        printf("initial Sample failed.");
        DroneStateDestroySample(pool, newSample);
        return NULL;
    }
    return newSample;
}

void DroneStateDestroySample(ZRMemPool* pool, DroneState* sample)
{
    if (sample == NULL) { return; }
    DroneStateFinalizeEx(sample, pool, true);
    ZRDealloc(pool, sample);
}

DDS_ULong DroneStateGetSerializedSampleMaxSize()
{
    return 96;
}

DDS_ULong DroneStateGetSerializedKeyMaxSize()
{
    return 4;
}

DDS_Long DroneStateGetKeyHash(
    const DroneState* sample,
    CDRSerializer* cdr,
    DDS::KeyHash_t* result)
{
    DDS_Long ret = DroneStateSerializeKey(sample, cdr);
    if (ret < 0)
    {
        printf("serialize key failed.");
        *result = DDS_HANDLE_NIL_NATIVE;
        return -1;
    }
    ret = CDRSerializeGetKeyHash(cdr, result->value, false);
    if (ret < 0)
    {
        printf("get keyhash failed.");
        *result = DDS_HANDLE_NIL_NATIVE;
        return -1;
    }
    result->valid = true;
    return 0;
}

DDS_Boolean DroneStateHasKey()
{
    return true;
}

TypeCodeHeader* DroneStateGetInnerTypeCode()
{
#ifdef _ZRDDS_INCLUDE_TYPECODE
    DDS::TypeCode* userTypeCode = DroneStateGetTypeCode();
    if (userTypeCode == NULL) { return NULL; }
    return userTypeCode->getImpl();
#else
    return NULL;
#endif
}

DDS_Boolean DroneStateInitializeEx(
    DroneState* self,
    ZRMemPool* pool,
    DDS_Boolean allocateMemory)
{
    self->drone_id = 0;

    self->status = SharedMessage::ST_IDLE;

    self->x = 0;

    self->y = 0;

    self->current_cmd_id = 0;

    self->vx = 0;

    self->vy = 0;

    DDS_ULongSeq_initialize_ex(&self->suspected_lost_ids, pool, allocateMemory);

    self->seq_num = 0;

    if (allocateMemory)
    {
        if (!DDS_ULongSeq_set_maximum(&self->suspected_lost_ids, 10))
        {
            printf("Set maximum for self->suspected_lost_ids failed.");
            return false;
        }
    }
    else
    {
    }
    return true;
}

void DroneStateFinalizeEx(
    DroneState* self,
    ZRMemPool* pool,
    DDS_Boolean deletePointers)
{
    DDS_ULongSeq_finalize(&self->suspected_lost_ids);
}

DDS_Boolean DroneStateCopyEx(
    DroneState* dst,
    const DroneState* src,
    ZRMemPool* pool)
{
    dst->drone_id = src->drone_id;
    dst->status = src->status;
    dst->x = src->x;
    dst->y = src->y;
    dst->current_cmd_id = src->current_cmd_id;
    dst->vx = src->vx;
    dst->vy = src->vy;
    if (!DDS_ULongSeq_copy(&dst->suspected_lost_ids, &src->suspected_lost_ids))
    {
        printf("copy member suspected_lost_ids failed.");
        return false;
    }
    dst->seq_num = src->seq_num;
    return true;
}

void DroneStatePrintData(const DroneState *sample)
{
    if (sample == NULL)
    {
        printf("NULL\n");
        return;
    }
    printf("sample->drone_id: %u\n", sample->drone_id);
    printf("\n");

    printf("sample->status: %d\n", sample->status);
    printf("\n");

    printf("sample->x: %lf\n", sample->x);
    printf("\n");

    printf("sample->y: %lf\n", sample->y);
    printf("\n");

    printf("sample->current_cmd_id: %u\n", sample->current_cmd_id);
    printf("\n");

    printf("sample->vx: %lf\n", sample->vx);
    printf("\n");

    printf("sample->vy: %lf\n", sample->vy);
    printf("\n");

    DDS_ULong suspected_lost_idsTmpLen = DDS_ULongSeq_get_length(&sample->suspected_lost_ids);
    printf("sample->suspected_lost_ids: %d\n", suspected_lost_idsTmpLen);
    for (DDS_ULong i = 0; i < suspected_lost_idsTmpLen; ++i)
    {
        printf("sample->suspected_lost_ids[%u]: %u\n", i, *DDS_ULongSeq_get_reference(&sample->suspected_lost_ids, i));
    }
    printf("\n");

    printf("sample->seq_num: %u\n", sample->seq_num);
    printf("\n");

}

DDS::TypeCode* DroneStateGetTypeCode()
{
    static DDS::TypeCode* s_typeCode = NULL;
    if (s_typeCode != NULL) { return s_typeCode; }
    DDS::TypeCodeFactory &factory = DDS::TypeCodeFactory::getInstance();

    s_typeCode = factory.createStructTC(
        "SharedMessage::DroneState",
        DDS_EXTENSIBLE_EXTENSIBILITY);
    if (s_typeCode == NULL)
    {
        printf("create struct DroneState typecode failed.");
        return s_typeCode;
    }
    DDS_Long ret = 0;
    DDS::TypeCode* memberTc = NULL;
    DDS::TypeCode* eleTc = NULL;

    memberTc = factory.getPrimitiveTC(DDS_TK_UINT);
    if (memberTc == NULL)
    {
        printf("Get Member drone_id TypeCode failed.");
        factory.deleteTC(s_typeCode);
        s_typeCode = NULL;
        return NULL;
    }
    ret = s_typeCode->addMemberToStruct(
        0,
        0,
        "drone_id",
        memberTc,
        true,
        false);
    if (ret < 0)
    {
        factory.deleteTC(s_typeCode);
        s_typeCode = NULL;
        return NULL;
    }

    memberTc = SharedMessage::DroneStatusGetTypeCode();
    if (memberTc == NULL)
    {
        printf("Get Member status TypeCode failed.");
        factory.deleteTC(s_typeCode);
        s_typeCode = NULL;
        return NULL;
    }
    ret = s_typeCode->addMemberToStruct(
        1,
        1,
        "status",
        memberTc,
        false,
        false);
    if (ret < 0)
    {
        factory.deleteTC(s_typeCode);
        s_typeCode = NULL;
        return NULL;
    }

    memberTc = factory.getPrimitiveTC(DDS_TK_DOUBLE);
    if (memberTc == NULL)
    {
        printf("Get Member x TypeCode failed.");
        factory.deleteTC(s_typeCode);
        s_typeCode = NULL;
        return NULL;
    }
    ret = s_typeCode->addMemberToStruct(
        2,
        2,
        "x",
        memberTc,
        false,
        false);
    if (ret < 0)
    {
        factory.deleteTC(s_typeCode);
        s_typeCode = NULL;
        return NULL;
    }

    memberTc = factory.getPrimitiveTC(DDS_TK_DOUBLE);
    if (memberTc == NULL)
    {
        printf("Get Member y TypeCode failed.");
        factory.deleteTC(s_typeCode);
        s_typeCode = NULL;
        return NULL;
    }
    ret = s_typeCode->addMemberToStruct(
        3,
        3,
        "y",
        memberTc,
        false,
        false);
    if (ret < 0)
    {
        factory.deleteTC(s_typeCode);
        s_typeCode = NULL;
        return NULL;
    }

    memberTc = factory.getPrimitiveTC(DDS_TK_UINT);
    if (memberTc == NULL)
    {
        printf("Get Member current_cmd_id TypeCode failed.");
        factory.deleteTC(s_typeCode);
        s_typeCode = NULL;
        return NULL;
    }
    ret = s_typeCode->addMemberToStruct(
        4,
        4,
        "current_cmd_id",
        memberTc,
        false,
        false);
    if (ret < 0)
    {
        factory.deleteTC(s_typeCode);
        s_typeCode = NULL;
        return NULL;
    }

    memberTc = factory.getPrimitiveTC(DDS_TK_DOUBLE);
    if (memberTc == NULL)
    {
        printf("Get Member vx TypeCode failed.");
        factory.deleteTC(s_typeCode);
        s_typeCode = NULL;
        return NULL;
    }
    ret = s_typeCode->addMemberToStruct(
        5,
        5,
        "vx",
        memberTc,
        false,
        false);
    if (ret < 0)
    {
        factory.deleteTC(s_typeCode);
        s_typeCode = NULL;
        return NULL;
    }

    memberTc = factory.getPrimitiveTC(DDS_TK_DOUBLE);
    if (memberTc == NULL)
    {
        printf("Get Member vy TypeCode failed.");
        factory.deleteTC(s_typeCode);
        s_typeCode = NULL;
        return NULL;
    }
    ret = s_typeCode->addMemberToStruct(
        6,
        6,
        "vy",
        memberTc,
        false,
        false);
    if (ret < 0)
    {
        factory.deleteTC(s_typeCode);
        s_typeCode = NULL;
        return NULL;
    }

    memberTc = factory.getPrimitiveTC(DDS_TK_UINT);
    if (memberTc != NULL)
    {
        memberTc = factory.createSequenceTC(10, memberTc);
    }
    if (memberTc == NULL)
    {
        printf("Get Member suspected_lost_ids TypeCode failed.");
        factory.deleteTC(s_typeCode);
        s_typeCode = NULL;
        return NULL;
    }
    ret = s_typeCode->addMemberToStruct(
        7,
        7,
        "suspected_lost_ids",
        memberTc,
        false,
        false);
    factory.deleteTC(memberTc);
    if (ret < 0)
    {
        factory.deleteTC(s_typeCode);
        s_typeCode = NULL;
        return NULL;
    }

    memberTc = factory.getPrimitiveTC(DDS_TK_UINT);
    if (memberTc == NULL)
    {
        printf("Get Member seq_num TypeCode failed.");
        factory.deleteTC(s_typeCode);
        s_typeCode = NULL;
        return NULL;
    }
    ret = s_typeCode->addMemberToStruct(
        8,
        8,
        "seq_num",
        memberTc,
        false,
        false);
    if (ret < 0)
    {
        factory.deleteTC(s_typeCode);
        s_typeCode = NULL;
        return NULL;
    }

    return s_typeCode;
}

DDS_Long DroneStateSerialize(const DroneState* sample, CDRSerializer *cdr)
{
    if (!CDRSerializerPutUntype(cdr, (DDS_Octet*) &sample->drone_id, 4))
    {
        printf("serialize sample->drone_id failed.");
        return -2;
    }

    if (!CDRSerializerPutUntype(cdr, (DDS_Octet*) &sample->status, 4))
    {
        printf("serialize sample->status failed.");
        return -2;
    }

    if (!CDRSerializerPutUntype(cdr, (DDS_Octet*) &sample->x, 8))
    {
        printf("serialize sample->x failed.");
        return -2;
    }

    if (!CDRSerializerPutUntype(cdr, (DDS_Octet*) &sample->y, 8))
    {
        printf("serialize sample->y failed.");
        return -2;
    }

    if (!CDRSerializerPutUntype(cdr, (DDS_Octet*) &sample->current_cmd_id, 4))
    {
        printf("serialize sample->current_cmd_id failed.");
        return -2;
    }

    if (!CDRSerializerPutUntype(cdr, (DDS_Octet*) &sample->vx, 8))
    {
        printf("serialize sample->vx failed.");
        return -2;
    }

    if (!CDRSerializerPutUntype(cdr, (DDS_Octet*) &sample->vy, 8))
    {
        printf("serialize sample->vy failed.");
        return -2;
    }

    if (!CDRSerializerPutUntype(cdr, (DDS_Octet*) &(sample->suspected_lost_ids)._length, 4))
    {
        printf("serialize length of sample->suspected_lost_ids failed.");
        return -2;
    }
#ifdef _ZRDDS_INCLUDE_NO_SERIALIZE_MODE
    if (!CDRSerializerNoSerializingMode(cdr))
    {
#endif/*_ZRDDS_INCLUDE_NO_SERIALIZE_MODE*/
        if ((sample->suspected_lost_ids)._contiguousBuffer)
        {
            if (!CDRSerializerPutUntypeArray(cdr, (DDS_Octet*)(sample->suspected_lost_ids)._contiguousBuffer, (sample->suspected_lost_ids)._length, 4))
            {
                printf("serialize sample->suspected_lost_ids failed.");
                return -2;
            }
        }
        else
        {
            for (DDS_ULong i = 0; i < (sample->suspected_lost_ids)._length; ++i)
            {
                if (!CDRSerializerPutUntype(cdr, (DDS_Octet*) &*DDS_ULongSeq_get_reference(&sample->suspected_lost_ids, i), 4))
                {
                    printf("serialize sample->suspected_lost_ids failed.");
                    return -2;
                }
            }
        }
#ifdef _ZRDDS_INCLUDE_NO_SERIALIZE_MODE
    }
#endif/*_ZRDDS_INCLUDE_NO_SERIALIZE_MODE*/

    if (!CDRSerializerPutUntype(cdr, (DDS_Octet*) &sample->seq_num, 4))
    {
        printf("serialize sample->seq_num failed.");
        return -2;
    }

    return 0;
}

DDS_Long DroneStateDeserialize(
    DroneState* sample,
    CDRDeserializer* cdr,
    ZRMemPool* pool)
{
    //has key :last key name:drone_id
    if (!CDRDeserializerGetUntype(cdr, (DDS_Octet*) &sample->drone_id, 4))
    {
        printf("deserialize sample->drone_id failed.");
        return -2;
    }
    //last key :drone_id has been deserialized
    if (!CDRDeserializerGetUntype(cdr, (DDS_Octet*) &sample->status, 4))
    {
        sample->status = SharedMessage::ST_IDLE;
        sample->x = 0;
        sample->y = 0;
        sample->current_cmd_id = 0;
        sample->vx = 0;
        sample->vy = 0;
        DDS_ULongSeq_initialize_ex(&sample->suspected_lost_ids, pool, true);
        sample->seq_num = 0;
        return 0;
    }
    if (!CDRDeserializerGetUntype(cdr, (DDS_Octet*) &sample->x, 8))
    {
        sample->x = 0;
        sample->y = 0;
        sample->current_cmd_id = 0;
        sample->vx = 0;
        sample->vy = 0;
        DDS_ULongSeq_initialize_ex(&sample->suspected_lost_ids, pool, true);
        sample->seq_num = 0;
        return 0;
    }
    if (!CDRDeserializerGetUntype(cdr, (DDS_Octet*) &sample->y, 8))
    {
        sample->y = 0;
        sample->current_cmd_id = 0;
        sample->vx = 0;
        sample->vy = 0;
        DDS_ULongSeq_initialize_ex(&sample->suspected_lost_ids, pool, true);
        sample->seq_num = 0;
        return 0;
    }
    if (!CDRDeserializerGetUntype(cdr, (DDS_Octet*) &sample->current_cmd_id, 4))
    {
        sample->current_cmd_id = 0;
        sample->vx = 0;
        sample->vy = 0;
        DDS_ULongSeq_initialize_ex(&sample->suspected_lost_ids, pool, true);
        sample->seq_num = 0;
        return 0;
    }
    if (!CDRDeserializerGetUntype(cdr, (DDS_Octet*) &sample->vx, 8))
    {
        sample->vx = 0;
        sample->vy = 0;
        DDS_ULongSeq_initialize_ex(&sample->suspected_lost_ids, pool, true);
        sample->seq_num = 0;
        return 0;
    }
    if (!CDRDeserializerGetUntype(cdr, (DDS_Octet*) &sample->vy, 8))
    {
        sample->vy = 0;
        DDS_ULongSeq_initialize_ex(&sample->suspected_lost_ids, pool, true);
        sample->seq_num = 0;
        return 0;
    }
    DDS_ULong suspected_lost_idsTmpLen = 0;
    if (!CDRDeserializerGetUntype(cdr, (DDS_Octet*) &suspected_lost_idsTmpLen, 4))
    {
        DDS_ULongSeq_initialize_ex(&sample->suspected_lost_ids, pool, true);
        sample->seq_num = 0;
        return 0;
    }
    if (!DDS_ULongSeq_ensure_length(&sample->suspected_lost_ids, suspected_lost_idsTmpLen, suspected_lost_idsTmpLen))
    {
        printf("Set maxiumum member sample->suspected_lost_ids failed.");
        return -3;
    }
    if (sample->suspected_lost_ids._contiguousBuffer)
    {
        if (!CDRDeserializerGetUntypeArray(cdr, (DDS_Octet*)sample->suspected_lost_ids._contiguousBuffer, suspected_lost_idsTmpLen, 4))
        {
            printf("deserialize sample->suspected_lost_ids failed.");
            return -2;
        }
    }
    else
    {
        for (DDS_ULong i = 0; i < suspected_lost_idsTmpLen; ++i)
        {
            if (!CDRDeserializerGetUntype(cdr, (DDS_Octet*) &*DDS_ULongSeq_get_reference(&sample->suspected_lost_ids, i), 4))
            {
                printf("deserialize sample->suspected_lost_ids failed.");
                return -2;
            }
        }
    }
    if (!CDRDeserializerGetUntype(cdr, (DDS_Octet*) &sample->seq_num, 4))
    {
        sample->seq_num = 0;
        return 0;
    }
    return 0;
}

DDS_ULong DroneStateGetSerializedSampleSize(const DroneState* sample, DDS_ULong currentAlignment)
{
    DDS_ULong initialAlignment = currentAlignment;

    currentAlignment += CDRSerializerGetUntypeSize(4, currentAlignment);

    currentAlignment += CDRSerializerGetUntypeSize(4, currentAlignment);

    currentAlignment += CDRSerializerGetUntypeSize(8, currentAlignment);

    currentAlignment += CDRSerializerGetUntypeSize(8, currentAlignment);

    currentAlignment += CDRSerializerGetUntypeSize(4, currentAlignment);

    currentAlignment += CDRSerializerGetUntypeSize(8, currentAlignment);

    currentAlignment += CDRSerializerGetUntypeSize(8, currentAlignment);

    currentAlignment += CDRSerializerGetUntypeSize(4, currentAlignment);
    DDS_ULong suspected_lost_idsLen = DDS_ULongSeq_get_length(&sample->suspected_lost_ids);
    if (suspected_lost_idsLen != 0)
    {
        currentAlignment += 4 * suspected_lost_idsLen;
    }

    currentAlignment += CDRSerializerGetUntypeSize(4, currentAlignment);

    return currentAlignment - initialAlignment;
}

DDS_Long DroneStateSerializeKey(const DroneState* sample, CDRSerializer *cdr)
{
    if (!CDRSerializerPutUntype(cdr, (DDS_Octet*) &sample->drone_id, 4))
    {
        printf("serialize sample->drone_id failed.");
        return -2;
    }

    return 0;
}

DDS_Long DroneStateDeserializeKey(
    DroneState* sample,
    CDRDeserializer* cdr,
    ZRMemPool* pool)
{
    if (!CDRDeserializerGetUntype(cdr, (DDS_Octet*) &sample->drone_id, 4))
    {
        printf("deserialize sample->drone_id failed.");
        return -2;
    }

    return 0;
}

DDS_ULong DroneStateGetSerializedKeySize(const DroneState* sample, DDS_ULong currentAlignment)
{
    DDS_ULong initialAlignment = currentAlignment;

    currentAlignment += CDRSerializerGetUntypeSize(4, currentAlignment);

    return currentAlignment - initialAlignment;
}

#ifdef _ZRDDS_INCLUDE_NO_SERIALIZE_MODE
DDS_Char* DroneStateLoanSampleBuf(DroneState* sample, DDS_Boolean takeBuffer)
{
    return NULL;
}

void DroneStateReturnSampleBuf(DDS_Char* sampleBuf)
{
    ;
}

DDS_Long DroneStateLoanDeserialize(DroneState* sampleBuf,
    CDRDeserializer* cdr,
    DDS_ULong curIndex,
    DDS_ULong totalNum,
    DDS_Char* base,
    DDS_ULong offset,
    DDS_ULong space,
    DDS_ULong fixedHeaderLen)
{
    return 0;
}

#endif /*_ZRDDS_INCLUDE_NO_SERIALIZE_MODE*/

#ifdef _ZRDDS_INCLUDE_ONSITE_DESERILIZE
DDS_Long DroneStateOnSiteDeserialize(CDRDeserializer* cdr,
    DroneState* sample,
    DDS_ULong offset,
    DDS_ULong totalSize,
    DDS_Char* payload,
    DDS_ULong payloadLen,
    DDS_ULong fixedHeaderLen)
{
    return 0;
}

DDS_Boolean DroneStateNoSerializingSupported()
{
    return false;
}

DDS_ULong DroneStateFixedHeaderLength()
{
    return 0;
}

#endif/*_ZRDDS_INCLUDE_ONSITE_DESERILIZE*/
#define T DroneCmd
#define TSeq DroneCmdSeq
#define TINITIALIZE DroneCmdInitializeEx
#define TFINALIZE DroneCmdFinalizeEx
#define TCOPY DroneCmdCopyEx

#include "ZRSequence.cpp"
#include "ZRCPlusPlusSequence.cpp"

#undef TCOPY
#undef TFINALIZE
#undef TINITIALIZE
#undef TSeq
#undef T

DDS_Boolean DroneCmdInitialize(DroneCmd* self)
{
    return DroneCmdInitializeEx(self, NULL, true);
}

void DroneCmdFinalize(DroneCmd* self)
{
    DroneCmdFinalizeEx(self, NULL, true);
}

DDS_Boolean DroneCmdCopy(
    DroneCmd* dst,
    const DroneCmd* src)
{
    return DroneCmdCopyEx(dst, src, NULL);
}

DroneCmd* DroneCmdCreateSample(
    ZRMemPool* pool,
    DDS_Boolean allocMutable)
{
    DroneCmd* newSample = (DroneCmd*)ZRMalloc(pool, sizeof(DroneCmd));
    if (newSample == NULL)
    {
        printf("malloc for DroneCmd failed.");
        return NULL;
    }
    if (!DroneCmdInitializeEx(newSample, pool, allocMutable))
    {
        printf("initial Sample failed.");
        DroneCmdDestroySample(pool, newSample);
        return NULL;
    }
    return newSample;
}

void DroneCmdDestroySample(ZRMemPool* pool, DroneCmd* sample)
{
    if (sample == NULL) { return; }
    DroneCmdFinalizeEx(sample, pool, true);
    ZRDealloc(pool, sample);
}

DDS_ULong DroneCmdGetSerializedSampleMaxSize()
{
    return 300;
}

DDS_ULong DroneCmdGetSerializedKeyMaxSize()
{
    return 300;
}

DDS_Long DroneCmdGetKeyHash(
    const DroneCmd* sample,
    CDRSerializer* cdr,
    DDS::KeyHash_t* result)
{
    DDS_Long ret = DroneCmdSerializeKey(sample, cdr);
    if (ret < 0)
    {
        printf("serialize key failed.");
        *result = DDS_HANDLE_NIL_NATIVE;
        return -1;
    }
    ret = CDRSerializeGetKeyHash(cdr, result->value, true);
    if (ret < 0)
    {
        printf("get keyhash failed.");
        *result = DDS_HANDLE_NIL_NATIVE;
        return -1;
    }
    result->valid = true;
    return 0;
}

DDS_Boolean DroneCmdHasKey()
{
    return false;
}

TypeCodeHeader* DroneCmdGetInnerTypeCode()
{
#ifdef _ZRDDS_INCLUDE_TYPECODE
    DDS::TypeCode* userTypeCode = DroneCmdGetTypeCode();
    if (userTypeCode == NULL) { return NULL; }
    return userTypeCode->getImpl();
#else
    return NULL;
#endif
}

DDS_Boolean DroneCmdInitializeEx(
    DroneCmd* self,
    ZRMemPool* pool,
    DDS_Boolean allocateMemory)
{
    self->cmd_id = 0;

    self->cmd_type = SharedMessage::CMD_TAKEOFF;

    self->formation_id = 0;

    self->formation_text = NULL;

    self->target_x = 0;

    self->target_y = 0;

    self->spacing = 0;

    self->expected_count = 0;

    if (allocateMemory)
    {
        self->formation_text = (DDS_Char*) ZRMalloc(pool, 255 + 1);
        if (self->formation_text == NULL)
        {
            printf("Malloc for self->formation_text failed.");
            return false;
        }
        self->formation_text[0] = '\0';
    }
    else
    {
        if (self->formation_text != NULL)
        {
            self->formation_text[0] = '\0';
        }
    }
    return true;
}

void DroneCmdFinalizeEx(
    DroneCmd* self,
    ZRMemPool* pool,
    DDS_Boolean deletePointers)
{
    if (deletePointers)
    {
        ZRDealloc(pool, self->formation_text);
        self->formation_text = NULL;
    }
}

DDS_Boolean DroneCmdCopyEx(
    DroneCmd* dst,
    const DroneCmd* src,
    ZRMemPool* pool)
{
    dst->cmd_id = src->cmd_id;
    dst->cmd_type = src->cmd_type;
    dst->formation_id = src->formation_id;
    if (src->formation_text == NULL)
    {
        ZRDealloc(pool, dst->formation_text);
        dst->formation_text = NULL;
    }
    else
    {
        if (dst->formation_text == NULL)
        {
            dst->formation_text = (DDS_Char*) ZRMalloc(pool, 255 + 1);
            if (dst->formation_text == NULL)
            {
                printf("malloc for formation_text failed.");
                return false;
            }
        }
        strcpy(dst->formation_text, src->formation_text);
    }
    dst->target_x = src->target_x;
    dst->target_y = src->target_y;
    dst->spacing = src->spacing;
    dst->expected_count = src->expected_count;
    return true;
}

void DroneCmdPrintData(const DroneCmd *sample)
{
    if (sample == NULL)
    {
        printf("NULL\n");
        return;
    }
    printf("sample->cmd_id: %u\n", sample->cmd_id);
    printf("\n");

    printf("sample->cmd_type: %d\n", sample->cmd_type);
    printf("\n");

    printf("sample->formation_id: %u\n", sample->formation_id);
    printf("\n");

    if (sample->formation_text != NULL)
    {
        printf("sample->formation_text(%d): %s\n", strlen(sample->formation_text), sample->formation_text);
    }
    else
    {
        printf("sample->formation_text(0): NULL\n");
    }
    printf("\n");

    printf("sample->target_x: %lf\n", sample->target_x);
    printf("\n");

    printf("sample->target_y: %lf\n", sample->target_y);
    printf("\n");

    printf("sample->spacing: %lf\n", sample->spacing);
    printf("\n");

    printf("sample->expected_count: %u\n", sample->expected_count);
    printf("\n");

}

DDS::TypeCode* DroneCmdGetTypeCode()
{
    static DDS::TypeCode* s_typeCode = NULL;
    if (s_typeCode != NULL) { return s_typeCode; }
    DDS::TypeCodeFactory &factory = DDS::TypeCodeFactory::getInstance();

    s_typeCode = factory.createStructTC(
        "SharedMessage::DroneCmd",
        DDS_EXTENSIBLE_EXTENSIBILITY);
    if (s_typeCode == NULL)
    {
        printf("create struct DroneCmd typecode failed.");
        return s_typeCode;
    }
    DDS_Long ret = 0;
    DDS::TypeCode* memberTc = NULL;
    DDS::TypeCode* eleTc = NULL;

    memberTc = factory.getPrimitiveTC(DDS_TK_UINT);
    if (memberTc == NULL)
    {
        printf("Get Member cmd_id TypeCode failed.");
        factory.deleteTC(s_typeCode);
        s_typeCode = NULL;
        return NULL;
    }
    ret = s_typeCode->addMemberToStruct(
        0,
        0,
        "cmd_id",
        memberTc,
        false,
        false);
    if (ret < 0)
    {
        factory.deleteTC(s_typeCode);
        s_typeCode = NULL;
        return NULL;
    }

    memberTc = SharedMessage::DroneCmdTypeGetTypeCode();
    if (memberTc == NULL)
    {
        printf("Get Member cmd_type TypeCode failed.");
        factory.deleteTC(s_typeCode);
        s_typeCode = NULL;
        return NULL;
    }
    ret = s_typeCode->addMemberToStruct(
        1,
        1,
        "cmd_type",
        memberTc,
        false,
        false);
    if (ret < 0)
    {
        factory.deleteTC(s_typeCode);
        s_typeCode = NULL;
        return NULL;
    }

    memberTc = factory.getPrimitiveTC(DDS_TK_UINT);
    if (memberTc == NULL)
    {
        printf("Get Member formation_id TypeCode failed.");
        factory.deleteTC(s_typeCode);
        s_typeCode = NULL;
        return NULL;
    }
    ret = s_typeCode->addMemberToStruct(
        2,
        2,
        "formation_id",
        memberTc,
        false,
        false);
    if (ret < 0)
    {
        factory.deleteTC(s_typeCode);
        s_typeCode = NULL;
        return NULL;
    }

    memberTc = factory.createStringTC(255);
    if (memberTc == NULL)
    {
        printf("Get Member formation_text TypeCode failed.");
        factory.deleteTC(s_typeCode);
        s_typeCode = NULL;
        return NULL;
    }
    ret = s_typeCode->addMemberToStruct(
        3,
        3,
        "formation_text",
        memberTc,
        false,
        false);
    factory.deleteTC(memberTc);
    if (ret < 0)
    {
        factory.deleteTC(s_typeCode);
        s_typeCode = NULL;
        return NULL;
    }

    memberTc = factory.getPrimitiveTC(DDS_TK_DOUBLE);
    if (memberTc == NULL)
    {
        printf("Get Member target_x TypeCode failed.");
        factory.deleteTC(s_typeCode);
        s_typeCode = NULL;
        return NULL;
    }
    ret = s_typeCode->addMemberToStruct(
        4,
        4,
        "target_x",
        memberTc,
        false,
        false);
    if (ret < 0)
    {
        factory.deleteTC(s_typeCode);
        s_typeCode = NULL;
        return NULL;
    }

    memberTc = factory.getPrimitiveTC(DDS_TK_DOUBLE);
    if (memberTc == NULL)
    {
        printf("Get Member target_y TypeCode failed.");
        factory.deleteTC(s_typeCode);
        s_typeCode = NULL;
        return NULL;
    }
    ret = s_typeCode->addMemberToStruct(
        5,
        5,
        "target_y",
        memberTc,
        false,
        false);
    if (ret < 0)
    {
        factory.deleteTC(s_typeCode);
        s_typeCode = NULL;
        return NULL;
    }

    memberTc = factory.getPrimitiveTC(DDS_TK_DOUBLE);
    if (memberTc == NULL)
    {
        printf("Get Member spacing TypeCode failed.");
        factory.deleteTC(s_typeCode);
        s_typeCode = NULL;
        return NULL;
    }
    ret = s_typeCode->addMemberToStruct(
        6,
        6,
        "spacing",
        memberTc,
        false,
        false);
    if (ret < 0)
    {
        factory.deleteTC(s_typeCode);
        s_typeCode = NULL;
        return NULL;
    }

    memberTc = factory.getPrimitiveTC(DDS_TK_UINT);
    if (memberTc == NULL)
    {
        printf("Get Member expected_count TypeCode failed.");
        factory.deleteTC(s_typeCode);
        s_typeCode = NULL;
        return NULL;
    }
    ret = s_typeCode->addMemberToStruct(
        7,
        7,
        "expected_count",
        memberTc,
        false,
        false);
    if (ret < 0)
    {
        factory.deleteTC(s_typeCode);
        s_typeCode = NULL;
        return NULL;
    }

    return s_typeCode;
}

DDS_Long DroneCmdSerialize(const DroneCmd* sample, CDRSerializer *cdr)
{
    if (!CDRSerializerPutUntype(cdr, (DDS_Octet*) &sample->cmd_id, 4))
    {
        printf("serialize sample->cmd_id failed.");
        return -2;
    }

    if (!CDRSerializerPutUntype(cdr, (DDS_Octet*) &sample->cmd_type, 4))
    {
        printf("serialize sample->cmd_type failed.");
        return -2;
    }

    if (!CDRSerializerPutUntype(cdr, (DDS_Octet*) &sample->formation_id, 4))
    {
        printf("serialize sample->formation_id failed.");
        return -2;
    }

    if (!CDRSerializerPutString(cdr, (DDS_Char*) sample->formation_text, sample->formation_text == NULL ? 0 : strlen(sample->formation_text) + 1))
    {
        printf("serialize sample->formation_text failed.");
        return -2;
    }

    if (!CDRSerializerPutUntype(cdr, (DDS_Octet*) &sample->target_x, 8))
    {
        printf("serialize sample->target_x failed.");
        return -2;
    }

    if (!CDRSerializerPutUntype(cdr, (DDS_Octet*) &sample->target_y, 8))
    {
        printf("serialize sample->target_y failed.");
        return -2;
    }

    if (!CDRSerializerPutUntype(cdr, (DDS_Octet*) &sample->spacing, 8))
    {
        printf("serialize sample->spacing failed.");
        return -2;
    }

    if (!CDRSerializerPutUntype(cdr, (DDS_Octet*) &sample->expected_count, 4))
    {
        printf("serialize sample->expected_count failed.");
        return -2;
    }

    return 0;
}

DDS_Long DroneCmdDeserialize(
    DroneCmd* sample,
    CDRDeserializer* cdr,
    ZRMemPool* pool)
{
    // no key
    if (!CDRDeserializerGetUntype(cdr, (DDS_Octet*) &sample->cmd_id, 4))
    {
        sample->cmd_id = 0;
        sample->cmd_type = SharedMessage::CMD_TAKEOFF;
        sample->formation_id = 0;
        sample->formation_text = NULL;
        sample->target_x = 0;
        sample->target_y = 0;
        sample->spacing = 0;
        sample->expected_count = 0;
        return 0;
    }
    if (!CDRDeserializerGetUntype(cdr, (DDS_Octet*) &sample->cmd_type, 4))
    {
        sample->cmd_type = SharedMessage::CMD_TAKEOFF;
        sample->formation_id = 0;
        sample->formation_text = NULL;
        sample->target_x = 0;
        sample->target_y = 0;
        sample->spacing = 0;
        sample->expected_count = 0;
        return 0;
    }
    if (!CDRDeserializerGetUntype(cdr, (DDS_Octet*) &sample->formation_id, 4))
    {
        sample->formation_id = 0;
        sample->formation_text = NULL;
        sample->target_x = 0;
        sample->target_y = 0;
        sample->spacing = 0;
        sample->expected_count = 0;
        return 0;
    }
    DDS_ULong formation_textTmpLen = 0;
    if (!CDRDeserializerGetUntype(cdr, (DDS_Octet*) &formation_textTmpLen, 4))
    {
        sample->formation_text = NULL;
        sample->target_x = 0;
        sample->target_y = 0;
        sample->spacing = 0;
        sample->expected_count = 0;
        return 0;
    }
    if (0 == formation_textTmpLen)
    {
        ZRDealloc(pool, sample->formation_text);
        sample->formation_text = NULL;
    }
    else
    {
        if (sample->formation_text == NULL)
        {
            sample->formation_text = (DDS_Char*) ZRMalloc(pool, formation_textTmpLen);
            if (sample->formation_text == NULL)
            {
                printf("malloc for sample->formation_text failed(%d).", formation_textTmpLen);
                return -3;
            }
        }
        if (!CDRDeserializerGetUntypeArray(cdr, (DDS_Octet*)sample->formation_text, formation_textTmpLen, 1))
        {
            printf("deserialize member sample->formation_text failed.");
            return -4;
        }
    }
    if (!CDRDeserializerGetUntype(cdr, (DDS_Octet*) &sample->target_x, 8))
    {
        sample->target_x = 0;
        sample->target_y = 0;
        sample->spacing = 0;
        sample->expected_count = 0;
        return 0;
    }
    if (!CDRDeserializerGetUntype(cdr, (DDS_Octet*) &sample->target_y, 8))
    {
        sample->target_y = 0;
        sample->spacing = 0;
        sample->expected_count = 0;
        return 0;
    }
    if (!CDRDeserializerGetUntype(cdr, (DDS_Octet*) &sample->spacing, 8))
    {
        sample->spacing = 0;
        sample->expected_count = 0;
        return 0;
    }
    if (!CDRDeserializerGetUntype(cdr, (DDS_Octet*) &sample->expected_count, 4))
    {
        sample->expected_count = 0;
        return 0;
    }
    return 0;
}

DDS_ULong DroneCmdGetSerializedSampleSize(const DroneCmd* sample, DDS_ULong currentAlignment)
{
    DDS_ULong initialAlignment = currentAlignment;

    currentAlignment += CDRSerializerGetUntypeSize(4, currentAlignment);

    currentAlignment += CDRSerializerGetUntypeSize(4, currentAlignment);

    currentAlignment += CDRSerializerGetUntypeSize(4, currentAlignment);

    currentAlignment += CDRSerializerGetStringSize(sample->formation_text == NULL ? 0 : strlen(sample->formation_text) + 1, currentAlignment);

    currentAlignment += CDRSerializerGetUntypeSize(8, currentAlignment);

    currentAlignment += CDRSerializerGetUntypeSize(8, currentAlignment);

    currentAlignment += CDRSerializerGetUntypeSize(8, currentAlignment);

    currentAlignment += CDRSerializerGetUntypeSize(4, currentAlignment);

    return currentAlignment - initialAlignment;
}

DDS_Long DroneCmdSerializeKey(const DroneCmd* sample, CDRSerializer *cdr)
{
    if (DroneCmdSerialize(sample, cdr) < 0)
    {
        return -1;
    }
    return 0;
}

DDS_Long DroneCmdDeserializeKey(
    DroneCmd* sample,
    CDRDeserializer* cdr,
    ZRMemPool* pool)
{
    if (DroneCmdDeserialize(sample, cdr, pool) < 0)
    {
        return -1;
    }
    return 0;
}

DDS_ULong DroneCmdGetSerializedKeySize(const DroneCmd* sample, DDS_ULong currentAlignment)
{
    DDS_ULong initialAlignment = currentAlignment;

    currentAlignment += DroneCmdGetSerializedSampleSize(sample, currentAlignment);
    return currentAlignment - initialAlignment;
}

#ifdef _ZRDDS_INCLUDE_NO_SERIALIZE_MODE
DDS_Char* DroneCmdLoanSampleBuf(DroneCmd* sample, DDS_Boolean takeBuffer)
{
    return NULL;
}

void DroneCmdReturnSampleBuf(DDS_Char* sampleBuf)
{
    ;
}

DDS_Long DroneCmdLoanDeserialize(DroneCmd* sampleBuf,
    CDRDeserializer* cdr,
    DDS_ULong curIndex,
    DDS_ULong totalNum,
    DDS_Char* base,
    DDS_ULong offset,
    DDS_ULong space,
    DDS_ULong fixedHeaderLen)
{
    return 0;
}

#endif /*_ZRDDS_INCLUDE_NO_SERIALIZE_MODE*/

#ifdef _ZRDDS_INCLUDE_ONSITE_DESERILIZE
DDS_Long DroneCmdOnSiteDeserialize(CDRDeserializer* cdr,
    DroneCmd* sample,
    DDS_ULong offset,
    DDS_ULong totalSize,
    DDS_Char* payload,
    DDS_ULong payloadLen,
    DDS_ULong fixedHeaderLen)
{
    return 0;
}

DDS_Boolean DroneCmdNoSerializingSupported()
{
    return false;
}

DDS_ULong DroneCmdFixedHeaderLength()
{
    return 0;
}

#endif/*_ZRDDS_INCLUDE_ONSITE_DESERILIZE*/
}
