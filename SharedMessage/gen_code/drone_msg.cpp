/*************************************************************/
/*           此文件由编译器生成，请勿随意修改                */
/*************************************************************/
#include "ZRMemPool.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "drone_msg.h"

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
    return 24;
}

DDS_ULong DroneCmdGetSerializedKeyMaxSize()
{
    return 4;
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

DDS_Boolean DroneCmdHasKey()
{
    return true;
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
    self->drone_id = 0;

    self->target_x = 0;

    self->target_y = 0;

    return true;
}

void DroneCmdFinalizeEx(
    DroneCmd* self,
    ZRMemPool* pool,
    DDS_Boolean deletePointers)
{
}

DDS_Boolean DroneCmdCopyEx(
    DroneCmd* dst,
    const DroneCmd* src,
    ZRMemPool* pool)
{
    dst->drone_id = src->drone_id;
    dst->target_x = src->target_x;
    dst->target_y = src->target_y;
    return true;
}

void DroneCmdPrintData(const DroneCmd *sample)
{
    if (sample == NULL)
    {
        printf("NULL\n");
        return;
    }
    printf("sample->drone_id: %d\n", sample->drone_id);
    printf("\n");

    printf("sample->target_x: %lf\n", sample->target_x);
    printf("\n");

    printf("sample->target_y: %lf\n", sample->target_y);
    printf("\n");

}

DDS::TypeCode* DroneCmdGetTypeCode()
{
    static DDS::TypeCode* s_typeCode = NULL;
    if (s_typeCode != NULL) { return s_typeCode; }
    DDS::TypeCodeFactory &factory = DDS::TypeCodeFactory::getInstance();

    s_typeCode = factory.createStructTC(
        "DroneCmd",
        DDS_EXTENSIBLE_EXTENSIBILITY);
    if (s_typeCode == NULL)
    {
        printf("create struct DroneCmd typecode failed.");
        return s_typeCode;
    }
    DDS_Long ret = 0;
    DDS::TypeCode* memberTc = NULL;
    DDS::TypeCode* eleTc = NULL;

    memberTc = factory.getPrimitiveTC(DDS_TK_INT);
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

    memberTc = factory.getPrimitiveTC(DDS_TK_DOUBLE);
    if (memberTc == NULL)
    {
        printf("Get Member target_x TypeCode failed.");
        factory.deleteTC(s_typeCode);
        s_typeCode = NULL;
        return NULL;
    }
    ret = s_typeCode->addMemberToStruct(
        1,
        1,
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
        2,
        2,
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

    return s_typeCode;
}

DDS_Long DroneCmdSerialize(const DroneCmd* sample, CDRSerializer *cdr)
{
    if (!CDRSerializerPutUntype(cdr, (DDS_Octet*) &sample->drone_id, 4))
    {
        printf("serialize sample->drone_id failed.");
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

    return 0;
}

DDS_Long DroneCmdDeserialize(
    DroneCmd* sample,
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
    if (!CDRDeserializerGetUntype(cdr, (DDS_Octet*) &sample->target_x, 8))
    {
        sample->target_x = 0;
        sample->target_y = 0;
        return 0;
    }
    if (!CDRDeserializerGetUntype(cdr, (DDS_Octet*) &sample->target_y, 8))
    {
        sample->target_y = 0;
        return 0;
    }
    return 0;
}

DDS_ULong DroneCmdGetSerializedSampleSize(const DroneCmd* sample, DDS_ULong currentAlignment)
{
    DDS_ULong initialAlignment = currentAlignment;

    currentAlignment += CDRSerializerGetUntypeSize(4, currentAlignment);

    currentAlignment += CDRSerializerGetUntypeSize(8, currentAlignment);

    currentAlignment += CDRSerializerGetUntypeSize(8, currentAlignment);

    return currentAlignment - initialAlignment;
}

DDS_Long DroneCmdSerializeKey(const DroneCmd* sample, CDRSerializer *cdr)
{
    if (!CDRSerializerPutUntype(cdr, (DDS_Octet*) &sample->drone_id, 4))
    {
        printf("serialize sample->drone_id failed.");
        return -2;
    }

    return 0;
}

DDS_Long DroneCmdDeserializeKey(
    DroneCmd* sample,
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

DDS_ULong DroneCmdGetSerializedKeySize(const DroneCmd* sample, DDS_ULong currentAlignment)
{
    DDS_ULong initialAlignment = currentAlignment;

    currentAlignment += CDRSerializerGetUntypeSize(4, currentAlignment);

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
    return 24;
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

    self->curr_x = 0;

    self->curr_y = 0;

    return true;
}

void DroneStateFinalizeEx(
    DroneState* self,
    ZRMemPool* pool,
    DDS_Boolean deletePointers)
{
}

DDS_Boolean DroneStateCopyEx(
    DroneState* dst,
    const DroneState* src,
    ZRMemPool* pool)
{
    dst->drone_id = src->drone_id;
    dst->curr_x = src->curr_x;
    dst->curr_y = src->curr_y;
    return true;
}

void DroneStatePrintData(const DroneState *sample)
{
    if (sample == NULL)
    {
        printf("NULL\n");
        return;
    }
    printf("sample->drone_id: %d\n", sample->drone_id);
    printf("\n");

    printf("sample->curr_x: %lf\n", sample->curr_x);
    printf("\n");

    printf("sample->curr_y: %lf\n", sample->curr_y);
    printf("\n");

}

DDS::TypeCode* DroneStateGetTypeCode()
{
    static DDS::TypeCode* s_typeCode = NULL;
    if (s_typeCode != NULL) { return s_typeCode; }
    DDS::TypeCodeFactory &factory = DDS::TypeCodeFactory::getInstance();

    s_typeCode = factory.createStructTC(
        "DroneState",
        DDS_EXTENSIBLE_EXTENSIBILITY);
    if (s_typeCode == NULL)
    {
        printf("create struct DroneState typecode failed.");
        return s_typeCode;
    }
    DDS_Long ret = 0;
    DDS::TypeCode* memberTc = NULL;
    DDS::TypeCode* eleTc = NULL;

    memberTc = factory.getPrimitiveTC(DDS_TK_INT);
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

    memberTc = factory.getPrimitiveTC(DDS_TK_DOUBLE);
    if (memberTc == NULL)
    {
        printf("Get Member curr_x TypeCode failed.");
        factory.deleteTC(s_typeCode);
        s_typeCode = NULL;
        return NULL;
    }
    ret = s_typeCode->addMemberToStruct(
        1,
        1,
        "curr_x",
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
        printf("Get Member curr_y TypeCode failed.");
        factory.deleteTC(s_typeCode);
        s_typeCode = NULL;
        return NULL;
    }
    ret = s_typeCode->addMemberToStruct(
        2,
        2,
        "curr_y",
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

    if (!CDRSerializerPutUntype(cdr, (DDS_Octet*) &sample->curr_x, 8))
    {
        printf("serialize sample->curr_x failed.");
        return -2;
    }

    if (!CDRSerializerPutUntype(cdr, (DDS_Octet*) &sample->curr_y, 8))
    {
        printf("serialize sample->curr_y failed.");
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
    if (!CDRDeserializerGetUntype(cdr, (DDS_Octet*) &sample->curr_x, 8))
    {
        sample->curr_x = 0;
        sample->curr_y = 0;
        return 0;
    }
    if (!CDRDeserializerGetUntype(cdr, (DDS_Octet*) &sample->curr_y, 8))
    {
        sample->curr_y = 0;
        return 0;
    }
    return 0;
}

DDS_ULong DroneStateGetSerializedSampleSize(const DroneState* sample, DDS_ULong currentAlignment)
{
    DDS_ULong initialAlignment = currentAlignment;

    currentAlignment += CDRSerializerGetUntypeSize(4, currentAlignment);

    currentAlignment += CDRSerializerGetUntypeSize(8, currentAlignment);

    currentAlignment += CDRSerializerGetUntypeSize(8, currentAlignment);

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
