#include "stdhdr.h"
#include "hdigi.h"
#include "otwdrive.h"
#include "PilotInputs.h"
#include "campwp.h"
#include "simveh.h"
#include "fcc.h"
#include "unit.h"
#include "helimm.h"
#include "Graphics/Include/drawBSP.h"

// Brain Choices
#define GENERIC_BRAIN     0
#define SEAD_BRAIN        1
#define STRIKE_BRAIN      2
#define INTERCEPT_BRAIN   3
#define AIR_CAP_BRAIN     4
#define AIR_SWEEP_BRAIN   5
#define ESCORT_BRAIN      6
#define WAYPOINTER_BRAIN  7

void HeliBrain::FollowWaypoints(void)
{
    if (self->curWaypoint == NULL)
    {
        AddMode(LoiterMode);
        return;
    }

    GoToCurrentWaypoint();

    //switch (self->curWaypoint->GetWPAction())
    //{
    //   case WP_LAND:
    // if ( onStation == Arrived || onStation == Stabilizing )
    //   LandMe();
    // else if ( !(onStation == OnStation) )
    // GoToCurrentWaypoint();
    // break;
    //
    // default:
    // GoToCurrentWaypoint();
    // break;
    //}
}

void HeliBrain::GoToCurrentWaypoint(void)
{
    float rng, desHeading;
    float rollLoad;
    float rollDir;
    float desSpeed;
    float wpX, wpY, wpZ;
    float dx, dy, time;

    Unit cargo;
    Unit unit;
    self->SetWPalt(0.0f);

    if (self->OnGround())
    {
        self->UnSetFlag(ON_GROUND);
    }

    unit = (Unit)self->GetCampaignObject();

    // RV - Biker - If we did miss our pickup do it now
    if (self->curWaypoint->GetWPAction() == WP_AIRDROP && !unit->GetCargo() && onStation == NotThereYet)
    {
        cargo = (Unit) self->curWaypoint->GetWPTarget();

        if (cargo && unit)
        {
            unit->SetCargoId(cargo->Id());
            cargo->SetCargoId(unit->Id());
            cargo->SetInactive(1);
            unit->LoadUnit(cargo);
        }
    }

    if (self->curWaypoint->GetWPAction() == WP_PICKUP && onStation == NotThereYet)
    {
        cargo = (Unit) self->curWaypoint->GetWPTarget();

        if (cargo)
        {
            wpX = cargo->XPos();
            wpY = cargo->YPos();
            wpZ = cargo->ZPos();
        }
        else
        {
            self->curWaypoint->GetLocation(&wpX, &wpY, &wpZ);
        }
    }
    else
    {
        self->curWaypoint->GetLocation(&wpX, &wpY, &wpZ);
    }

    // If we are at starting WP stay on ground
    if (self->curWaypoint->GetWPFlags() & WPF_TAKEOFF  &&
        self->curWaypoint->GetWPDepartureTime() > SimLibElapsedTime &&
        self->curWaypoint->GetPrevWP() == NULL)
    {
        LevelTurn(0.0f, 0.0f, TRUE);
        MachHold(0.0f, 0.0f, FALSE);
        // RV - Biker - Extend landing gear
        ((DrawableBSP*)self->drawPointer)->SetSwitchMask(2, 1);
        return;
    }

    // RV - Biker - Retract landing gear
    ((DrawableBSP*)self->drawPointer)->SetSwitchMask(2, 0);

    float wpalt = (float)self->curWaypoint->GetWPAltitude();

    // Max altitude
    if (wpalt > 12000.0f)
        wpalt = 12000.0f;

    // Convert to AGL
    wpalt = wpalt - OTWDriver.GetGroundLevel(wpX, wpY);

    if (wpalt < 20.0f)
        wpalt = 20.0f;

    self->SetWPalt(wpalt);

    desSpeed = 1.0f;
    rollDir = 0.0f;
    rollLoad = 0.0f;

    if (curMode != lastMode)
    {
        onStation = NotThereYet;
    }

    // Range to current waypoint - sqrt???
    dx = (wpX - self->XPos());
    dy = (wpY - self->YPos());
    rng = dx * dx + dy * dy;

    // Heading error for current waypoint
    desHeading = (float)atan2(dy, dx) - self->Yaw();

    if (desHeading > 180.0F * DTR)
        desHeading -= 360.0F * DTR;
    else if (desHeading < -180.0F * DTR)
        desHeading += 360.0F * DTR;

    // rollLoad is normalized (0-1) factor of how far off-heading we are to target
    rollLoad = desHeading / (180.0F * DTR);

    if (rollLoad < 0.0F)
        rollLoad = -rollLoad;

    if (desHeading > 0.0f)
        rollDir = 1.0f;
    else
        rollDir = -1.0f;

    // Reached the next waypoint?
    // RV - Biker - Never skip waypoints
    if (rng < (600.0F * 600.0F) || (onStation != NotThereYet) /*|| SimLibElapsedTime > self->curWaypoint->GetWPDepartureTime()*/)
    {
        if (onStation == NotThereYet)
        {
            onStation = Arrived;
        }
        else if (onStation == OnStation && SimLibElapsedTime > self->curWaypoint->GetWPDepartureTime())
        {
            SelectNextWaypoint();
        }
    }

    // landing?
    if (onStation == Landing ||
        onStation == DropOff ||
        onStation == Landed ||
        onStation == PickUp)
    {
        LandMe();
        return;
    }

    // On Station ?
    if (onStation == Arrived)
    {
        if (self->curWaypoint->GetWPFlags() & WPF_LAND)
        {
            LandMe();
            return;
        }

        onStation = OnStation;
    }

    // get waypoint speed based on our dist and arrival time
    if (rng < 600.0f * 600.0f)
    {
        rollLoad = 0.0f;
        desSpeed = 0.0f;
    }
    else
    {
        rng = (float)sqrt(rng);

        if (self->curWaypoint->GetWPArrivalTime() > SimLibElapsedTime)
            time = (float)(self->curWaypoint->GetWPArrivalTime() - SimLibElapsedTime) / SEC_TO_MSEC;
        else
            time = -1.0f;

        if (time <= 0.0f)
        {
            // we're late
            desSpeed = 1.0f;
        }
        else
        {
            desSpeed = (rng / time) / MAX_HELI_FPS;

            if (desSpeed > 1.0f)
                desSpeed = 1.0f;
            //TJL 11/20/03 Minimum speed
            else if (desSpeed < 0.5f)
                desSpeed = 0.5f;

            //end new
        }
    }

    // if we're close, just point to spot then go
    if (fabs(rollLoad) > 0.1f && rng < 1000.0f * 1000.0f)
        desSpeed = 0.5f;

    LevelTurn(rollLoad, rollDir, TRUE);
    MachHold(desSpeed, self->GetWPalt(), TRUE);
    //MachHold(desSpeed, 300.0f, TRUE);
}

void HeliBrain::SelectNextWaypoint(void)
{
    WayPointClass* tmpWaypoint = self->curWaypoint;
    WayPointClass* wlist = self->waypoint;
    UnitClass *campUnit = NULL;
    WayPointClass *campCurWP = NULL;
    int waypointIndex, i;

    // first get our current waypoint index in the list
    for (waypointIndex = 0;
         wlist && wlist != tmpWaypoint;
         wlist = wlist->GetNextWP(), waypointIndex++);

    // see if we're running in tactical or campaign.  If so, we want to
    // synch the campaign's waypoints with ours
    // if ( SimDriver.RunningCampaignOrTactical() )
    {
        // get the pointer to our campaign unit
        campUnit = (UnitClass *)self->GetCampaignObject();

        if (campUnit)
        {
            campCurWP = campUnit->GetFirstUnitWP();

            // now get the camp waypoint that corresponds to our next in the
            // list by index
            for (i = 0; i <= waypointIndex; i++)
            {
                if (campCurWP)
                {
                    campCurWP = campCurWP->GetNextWP();
                }
            }
        }
    }

    onStation = NotThereYet;
    self->curWaypoint = self->curWaypoint->GetNextWP();

    // KCK: This isn't working anyway - so I'm commentting it out in order to prevent bugs
    // in the ATC and Campaign
    // edg: this should be OK now that an obsolute waypoint index is used to
    // synch the current WP between sim and camp units.
    if (campCurWP)
    {
        campUnit->SetCurrentUnitWP(campCurWP);
    }

    waypointMode = 0;

    if (!self->curWaypoint)
    {
        // go back to the beginning
        self->curWaypoint = self->waypoint;
        campUnit->SetCurrentUnitWP(campUnit->GetFirstUnitWP());
    }

    if (self->OnGround())
    {
        self->UnSetFlag(ON_GROUND);
    }

    ChooseBrain();
}

void HeliBrain::ChooseBrain(void)
{
    if (self->curWaypoint)
    {
        switch (self->curWaypoint->GetWPAction())
        {
            case WP_NOTHING:
            case WP_TAKEOFF:
            case WP_ASSEMBLE:
            case WP_POSTASSEMBLE:
            case WP_REFUEL:
            case WP_REARM:
            case WP_LAND:
            case WP_ELINT:
            case WP_RECON:
            case WP_RESCUE:
            case WP_ASW:
            case WP_TANKER:
            case WP_AIRDROP:
            case WP_JAM:
            case WP_PICKUP:
                // MonoPrint ("Helo Digi Chose Waypoint BRAIN\n");
                // modeData = digitalBrains->brainData[AIR_SWEEP_BRAIN];
                break;

            case WP_ESCORT:
                // MonoPrint ("Helo Digi Chose ESCORT BRAIN\n");
                break;

            case WP_CA:
                // MonoPrint ("Helo Digi Chose AIR SWEEP BRAIN\n");
                break;

            case WP_CAP:
                // MonoPrint ("Helo Digi Chose AIR CAP BRAIN\n");
                break;

            case WP_INTERCEPT:
                // MonoPrint ("Helo Digi Chose AIR INTERCEPT BRAIN\n");
                break;

            case WP_GNDSTRIKE:
            case WP_NAVSTRIKE:
            case WP_STRIKE:
            case WP_BOMB:
            case WP_SAD:
                // MonoPrint ("Helo Digi Chose STRIKE BRAIN\n");
                break;

            case WP_SEAD:
                // MonoPrint ("Helo Digi Chose SEAD BRAIN\n");
                break;

            default:
                // MonoPrint ("Why am I here (Helo Digi GetBrain)\n");
                // MonoPrint ("===>Waypoint action %d\n", self->curWaypoint->GetWPAction());
                break;
        }
    }
}
