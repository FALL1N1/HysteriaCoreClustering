#include "Creature.h"
#include "CreatureGroups.h"
#include "ObjectMgr.h"

#include "CreatureAI.h"
#include "MoveSplineInit.h"

FormationMgr::~FormationMgr()
{
    for (CreatureGroupInfoType::iterator itr = CreatureGroupMap.begin(); itr != CreatureGroupMap.end(); ++itr)
        delete itr->second;
}

void FormationMgr::AddCreatureToGroup(uint32 groupId, Creature* member)
{
    Map* map = member->FindMap();
    if (!map)
        return;

    CreatureGroupHolderType::iterator itr = map->CreatureGroupHolder.find(groupId);

    if (itr != map->CreatureGroupHolder.end())
    {
        itr->second->AddMember(member);
    }
    else
    {
        CreatureGroup* group = new CreatureGroup(groupId);
        map->CreatureGroupHolder[groupId] = group;
        group->AddMember(member);
    }
}

void FormationMgr::RemoveCreatureFromGroup(CreatureGroup* group, Creature* member)
{
    group->RemoveMember(member);

    if (group->isEmpty())
    {
        Map* map = member->FindMap();
        if (!map)
            return;

        map->CreatureGroupHolder.erase(group->GetId());
        delete group;
    }
}

void FormationMgr::LoadCreatureFormations()
{
    uint32 oldMSTime = getMSTime();

    for (CreatureGroupInfoType::iterator itr = CreatureGroupMap.begin(); itr != CreatureGroupMap.end(); ++itr) 
        delete itr->second;
    CreatureGroupMap.clear();

    QueryResult result = WorldDatabase.Query("SELECT leaderGUID, memberGUID, dist, angle, groupAI, point_1, point_2 FROM creature_formations ORDER BY leaderGUID");

    if (!result)
    {
        sLog->outErrorDb(">>  Loaded 0 creatures in formations. DB table `creature_formations` is empty!");
        sLog->outString();
        return;
    }

    uint32 count = 0;
    Field* fields;
    FormationInfo* group_member;

    do
    {
        fields = result->Fetch();

        group_member                        = new FormationInfo();
        group_member->leaderGUID            = fields[0].GetUInt32();
        uint32 memberGUID                   = fields[1].GetUInt32();
        group_member->groupAI               = fields[4].GetUInt32();
        group_member->point_1               = fields[5].GetUInt16();
        group_member->point_2               = fields[6].GetUInt16();

        if (group_member->leaderGUID != memberGUID)
        {
            group_member->follow_dist       = fields[2].GetFloat();
            group_member->follow_angle      = fields[3].GetFloat() * M_PI / 180;
        }
        else
        {
            group_member->follow_dist       = 0;
            group_member->follow_angle      = 0;
        }

        {
            if (!sObjectMgr->GetCreatureData(group_member->leaderGUID))
            {
                sLog->outErrorDb("creature_formations table leader guid %u incorrect (not exist)", group_member->leaderGUID);
                delete group_member;
                continue;
            }

            if (!sObjectMgr->GetCreatureData(memberGUID))
            {
                sLog->outErrorDb("creature_formations table member guid %u incorrect (not exist)", memberGUID);
                delete group_member;
                continue;
            }
        }

        CreatureGroupMap[memberGUID] = group_member;
        ++count;
    }
    while (result->NextRow());

    sLog->outString(">> Loaded %u creatures in formations in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
    sLog->outString();
}

void CreatureGroup::AddMember(Creature* member)
{
    if (member->GetDBTableGUIDLow() == m_groupID)
    {
        m_leader = member;
    }

    m_members[member] = sFormationMgr->CreatureGroupMap.find(member->GetDBTableGUIDLow())->second;
    member->SetFormation(this);
}

void CreatureGroup::RemoveMember(Creature* member)
{
    if (m_leader == member)
        m_leader = NULL;

    m_members.erase(member);
    member->SetFormation(NULL);
}

void CreatureGroup::MemberAttackStart(Creature* member, Unit* target)
{
    uint8 groupAI = sFormationMgr->CreatureGroupMap[member->GetDBTableGUIDLow()]->groupAI;
    if (!groupAI)
        return;

    if (groupAI == 1 && member != m_leader)
        return;

    for (CreatureGroupMemberType::iterator itr = m_members.begin(); itr != m_members.end(); ++itr)
    {
        if (itr->first == member)
            continue;

        if (!itr->first->IsAlive())
            continue;

        if (itr->first->GetVictim())
            continue;

        if (itr->first->IsValidAttackTarget(target) && itr->first->AI())
            itr->first->AI()->AttackStart(target);
    }
}

void CreatureGroup::FormationReset(bool dismiss)
{
    if (m_members.size() && m_members.begin()->second->groupAI == 5)
        return;
    
    for (CreatureGroupMemberType::iterator itr = m_members.begin(); itr != m_members.end(); ++itr)
    {
        if (itr->first != m_leader && itr->first->IsAlive())
        {
            if (dismiss)
                itr->first->GetMotionMaster()->Initialize();
            else
                itr->first->GetMotionMaster()->MoveIdle();
        }
    }
    m_Formed = !dismiss;
}

void CreatureGroup::LeaderMoveTo(float x, float y, float z, bool run)
{
    if (!m_leader)
        return;

    uint8 groupAI = sFormationMgr->CreatureGroupMap[m_leader->GetDBTableGUIDLow()]->groupAI;
    if (groupAI == 5)
        return;

    float pathDist = m_leader->GetExactDist(x, y, z);
    float pathAngle = m_leader->GetAngle(x, y);

    for (CreatureGroupMemberType::iterator itr = m_members.begin(); itr != m_members.end(); ++itr)
    {
        Creature* member = itr->first;
        if (member == m_leader || !member->IsAlive() || member->GetVictim())
            continue;

        if (member->HasUnitState(UNIT_STATE_NOT_MOVE))
            continue;

        if (M_PI - fabs(fabs(m_leader->GetOrientation() - pathAngle) - M_PI) > M_PI*0.50f)
        {
            itr->second->follow_angle = Position::NormalizeOrientation(itr->second->follow_angle + M_PI); //(2 * M_PI) - itr->second->follow_angle;
        }

        float followAngle = itr->second->follow_angle;
        float followDist = itr->second->follow_dist;

        float dx = x + cos(followAngle + pathAngle) * followDist;
        float dy = y + sin(followAngle + pathAngle) * followDist;
        float dz = z;

        Trinity::NormalizeMapCoord(dx);
        Trinity::NormalizeMapCoord(dy);
        Trinity::NormalizeMapCoord(dz);

        member->UpdateGroundPositionZ(dx, dy, dz);

        member->SetUnitMovementFlags(m_leader->GetUnitMovementFlags());

        if (run && member->IsWalking())
            member->RemoveUnitMovementFlag(MOVEMENTFLAG_WALKING);
        else if (!run && !member->IsWalking())
            member->AddUnitMovementFlag(MOVEMENTFLAG_WALKING);

        if (m_leader->HasUnitMovementFlag(MOVEMENTFLAG_WALKING))
            member->AddUnitMovementFlag(MOVEMENTFLAG_WALKING);

        member->UpdateGroundPositionZ(dx, dy, dz);

        if (member->IsWithinDist(m_leader, followDist + 5))
            member->SetUnitMovementFlags(MOVEMENTFLAG_WALKING);//->GetUnitMovementFlags());
        else
            member->SetWalk(false);

        UnitMoveType mtype = Movement::SelectSpeedType(member->GetUnitMovementFlags());
        member->SetSpeedRate(mtype, m_leader->GetSpeedRate(mtype) * member->GetExactDist(dx, dy, dz) / pathDist);


        // update speeds
        member->UpdateSpeed(MOVE_WALK, true);
        member->UpdateSpeed(MOVE_RUN, true);

        member->GetMotionMaster()->MovePoint(0, dx, dy, dz);
        member->SetHomePosition(dx, dy, dz, pathAngle);
    }
}
