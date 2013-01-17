#include "F4error.h"

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
// ACMITape inlines.

inline int ACMITape::NumEntities()
{
    // F4Assert(_tape != NULL);

    return _tapeHdr.numEntities;
}


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

inline int ACMITape::EntityId(int index)
{
    return (int)(EntityData(index)->uniqueID);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

inline int ACMITape::EntityType(int index)
{
    return (int)(EntityData(index)->type);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

inline BOOL ACMITape::IsLoaded()
{
    if (_tape == NULL)
        return FALSE;

    return TRUE;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

inline BOOL ACMITape::IsPaused()
{
    return _paused;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

inline void ACMITape::Play()
{
    _unpause = TRUE;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

inline void ACMITape::Pause()
{
    _paused = TRUE;
    _unpause = FALSE;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

inline void ACMITape::StepTime(float numSeconds)
{
    _simTime += numSeconds;
    _stepTrail = numSeconds;
    AdvanceAllHeads();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

inline void ACMITape::SetPlayVelocity(float n)
{
    _playVelocity = n;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

inline float ACMITape::PlayVelocity()
{
    return _playVelocity;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

inline void ACMITape::SetPlayAcceleration(float n)
{
    _playAcceleration = n;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

inline float ACMITape::PlayAcceleration()
{
    return _playAcceleration;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

inline void ACMITape::SetMaxPlaySpeed(float n)
{
    _maxPlaySpeed = (float)fabs(n);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

inline float ACMITape::MaxPlaySpeed()
{
    return _maxPlaySpeed;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

inline float ACMITape::SimTime()
{
    return _simTime;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

inline float ACMITape::GetTapePercent()
{
    // F4Assert( _simTime >= _tapeHdr.startTime );
    // F4Assert( _tapeHdr.totPlayTime > 0.0f );

    return (_simTime - _tapeHdr.startTime) / _tapeHdr.totPlayTime;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

inline ACMIEntityData *ACMITape::EntityData(int index)
{
    // F4Assert(_tape != NULL);
    // F4Assert(index >= 0 && index < NumEntities());

    return
        (
            (ACMIEntityData *)
            (
                ((char *)_tape) +
                sizeof(ACMITapeHeader) +
                index * sizeof(ACMIEntityData)
            )
        );
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

inline ACMIEntityData *ACMITape::FeatureData(int index)
{
    // F4Assert(_tape != NULL);
    // F4Assert(index >= 0 && index < _tapeHdr.numFeat);

    return
        (
            (ACMIEntityData *)
            (
                ((char *)_tape) +
                _tapeHdr.featBlockOffset +
                index * sizeof(ACMIEntityData)
            )
        );
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

inline ACMIEntityPositionData *ACMITape::CurrentFeaturePositionHead(int index)
{
    long
    positionOffset;
    ACMIEntityPositionData *pd;
    ACMIEntityData *e;

    // F4Assert(_tape != NULL);
    // F4Assert(index >= 0 && index < _tapeHdr.numFeat);

    e = FeatureData(index);

    positionOffset = e->firstPositionDataOffset;
    pd = positionOffset == 0 ? NULL : (ACMIEntityPositionData *)
         (
             ((char *)_tape) +
             positionOffset
         );
    return pd;

}
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

inline ACMIEntityPositionData *ACMITape::CurrentEntityPositionHead(int index)
{
    long
    positionOffset;
    ACMIEntityPositionData *pd;

    // F4Assert(_tape != NULL);
    // F4Assert(_entityReadHeads != NULL);
    // F4Assert(index >= 0 && index < NumEntities());

    positionOffset = _entityReadHeads[index].positionDataOffset;
    pd = positionOffset == 0 ? NULL : (ACMIEntityPositionData *)
         (
             ((char *)_tape) +
             positionOffset
         );
    return pd;

}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

inline ACMIEntityPositionData *ACMITape::CurrentEntityEventHead(int index)
{
    long
    positionOffset;
    ACMIEntityPositionData *pd;

    // F4Assert(_tape != NULL);
    // F4Assert(_entityReadHeads != NULL);
    // F4Assert(index >= 0 && index < NumEntities());

    positionOffset = _entityReadHeads[index].eventDataOffset;
    pd = positionOffset == 0 ? NULL : (ACMIEntityPositionData *)
         (
             ((char *)_tape) +
             positionOffset
         );
    return pd;

}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

inline ACMIEntityPositionData *ACMITape::HeadNext(ACMIEntityPositionData *current)
{
    long
    positionOffset;
    ACMIEntityPositionData *pd;

    // F4Assert(_tape != NULL);
    // F4Assert(_entityReadHeads != NULL);

    if (current == NULL) return NULL;

    positionOffset = current->nextPositionUpdateOffset;
    pd = positionOffset == 0 ? NULL : (ACMIEntityPositionData *)
         (
             ((char *)_tape) +
             positionOffset
         );
    return pd;

}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

inline ACMIEntityPositionData *ACMITape::HeadPrev(ACMIEntityPositionData *current)
{
    long
    positionOffset;
    ACMIEntityPositionData *pd;

    // F4Assert(_tape != NULL);
    // F4Assert(_entityReadHeads != NULL);

    if (current == NULL) return NULL;

    positionOffset = current->prevPositionUpdateOffset;
    pd = positionOffset == 0 ? NULL : (ACMIEntityPositionData *)
         (
             ((char *)_tape) +
             positionOffset
         );
    return pd;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

inline ACMIEventHeader *ACMITape::GetGeneralEventData(int i)
{
    ACMIEventHeader
    *result;

    // F4Assert(_tape != NULL);

    if (i < 0 || i >= _tapeHdr.numEvents)
    {
        return NULL;
    }
    else
    {
        result = (ACMIEventHeader *)
                 (
                     ((char *)_tape) +
                     _tapeHdr.firstGeneralEventOffset +
                     sizeof(ACMIEventHeader) * i
                 );

        return result;
    }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

inline ACMIEventHeader *ACMITape::GeneralEventData(void)
{
    // F4Assert(_tape != NULL);

    return GetGeneralEventData(_generalEventReadHeadHeader);

}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

inline ACMIEventHeader *ACMITape::Next(ACMIEventHeader *current)
{
    // F4Assert(_tape != NULL);

    if (current == NULL)
    {
        return NULL;
    }

    return GetGeneralEventData(current->index + 1);

}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

inline ACMIEventHeader *ACMITape::Prev(ACMIEventHeader *current)
{
    // F4Assert(_tape != NULL);

    if (current == NULL)
    {
        return NULL;
    }

    return GetGeneralEventData(current->index - 1);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

inline ACMIEventTrailer *ACMITape::GeneralEventTrailer(void)
{
    // F4Assert(_tape != NULL);

    return  _generalEventReadHeadTrailer;

}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

inline ACMIEventTrailer *ACMITape::Next(ACMIEventTrailer *current)
{
    // F4Assert(_tape != NULL);

    if (current == NULL || current == _lastEventTrailer)
    {
        return NULL;
    }


    return  current + 1;

}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

inline ACMIEventTrailer *ACMITape::Prev(ACMIEventTrailer *current)
{
    // F4Assert(_tape != NULL);

    if (current == NULL || current == _firstEventTrailer)
    {
        return NULL;
    }

    return current - 1;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

inline ACMIFeatEvent *ACMITape::CurrFeatEvent(void)
{
    // F4Assert(_tape != NULL);

    return  _featEventReadHead;

}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

inline ACMIFeatEvent *ACMITape::Next(ACMIFeatEvent *current)
{
    // F4Assert(_tape != NULL);

    if (current == NULL || current == _lastFeatEvent)
    {
        return NULL;
    }


    return  current + 1;

}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

inline ACMIFeatEvent *ACMITape::Prev(ACMIFeatEvent *current)
{
    // F4Assert(_tape != NULL);

    if (current == NULL || current == _firstFeatEvent)
    {
        return NULL;
    }

    return current - 1;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

