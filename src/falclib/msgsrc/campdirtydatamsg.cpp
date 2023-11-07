/*
 * Machine Generated source file for message "Camp Dirty Data".
 * NOTE: The functions here must be completed by hand.
 * Generated on 17-November-1998 at 20:52:31
 * Generated from file EVENTS.XLS by Robin Heydon
 */

#include "MsgInc/CampDirtyDataMsg.h"
#include "mesg.h"
#include "falclib.h"
#include "falcmesg.h"
#include "falcgame.h"
#include "falcsess.h"

//sfr: added here for checks
#include "InvalidBufferException.h"

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

CampDirtyData::CampDirtyData(VU_ID entityId, VuTargetEntity *target, VU_BOOL loopback) : FalconEvent(CampDirtyDataMsg, FalconEvent::CampaignThread, entityId, target, loopback)
{
    dataBlock.data = NULL;
    dataBlock.size = 0;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

CampDirtyData::CampDirtyData(VU_MSG_TYPE type, VU_ID senderid, VU_ID target) : FalconEvent(CampDirtyDataMsg, FalconEvent::CampaignThread, senderid, target)
{
    dataBlock.data = NULL;
    dataBlock.size = 0;
    type;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

CampDirtyData::~CampDirtyData(void)
{
    if (dataBlock.data)
    {
        delete dataBlock.data;
    }

    dataBlock.data = NULL;
    dataBlock.size = 0;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int CampDirtyData::Size() const
{
    ShiAssert(dataBlock.size >= 0);
    return(FalconEvent::Size() + sizeof(ushort) + dataBlock.size);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//sfr: changed to long *
int CampDirtyData::Decode(VU_BYTE **buf, long *rem)
{
    long int init = *rem;

    FalconEvent::Decode(buf, rem);
    memcpychk(&dataBlock.size, buf, sizeof(ushort), rem);
    ShiAssert(dataBlock.size > 0);
    dataBlock.data = new uchar[dataBlock.size];
    memcpychk(dataBlock.data, buf, dataBlock.size, rem);

    // ShiAssert (size == Size());

    return init - *rem;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int CampDirtyData::Encode(VU_BYTE **buf)
{
    int size;

    size = FalconEvent::Encode(buf);
    ShiAssert(dataBlock.size > 0);
    memcpy(*buf, &dataBlock.size, sizeof(ushort));
    *buf += sizeof(ushort);
    size += sizeof(ushort);

    memcpy(*buf, dataBlock.data, dataBlock.size);
    *buf += dataBlock.size;
    size += dataBlock.size;

    ShiAssert(size == Size());

    return size;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int CampDirtyData::Process(uchar autodisp)
{
    FalconEntity *ent;

    ent = (FalconEntity*) vuDatabase->Find(EntityId());

    if (!ent || autodisp)
    {
        return 0;
    }

    // Only accept data if this is a remote entity
    //sfr: added size check, as we are receiving empy messages
    if ((!ent->IsLocal())/* && (dataBlock.size != 0)*/)
    {
        //sfr: was size = ent->DecodeDirty (&data);
        //we do this because we consume the buffer, and will need to free
        //dataBlock.data...
        VU_BYTE *data = dataBlock.data;
        long size = dataBlock.size;
#ifdef MP_DEBUG
        ent->DecodeDirty(&data, &size);
#else

        try
        {
            ent->DecodeDirty(&data, &size);
        }
        catch (InvalidBufferException)
        {
            fprintf(stderr, "%s %d: invalid buffer, check here!!!\n", __FILE__,  __LINE__);
        }

#endif
        assert(size == 0);
        //assert (size == dataBlock.size);
    }

    return 0;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

