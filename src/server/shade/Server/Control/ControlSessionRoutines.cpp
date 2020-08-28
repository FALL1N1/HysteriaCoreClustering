#include "ControlSession.h"
#include "Opcodes.h"
#include "WorldPacket.h"
#include "RoutingHelper.h"
#include "ObjectMgr.h"
#include "Proxy.h"
#include "ClusterDefines.h"

//Range,Item,Mail
void ControlSession::SendInitAuthedNode()
{
    //sLog->outError("ControlSession::SendInitAuthedNode() for node %u", _nodeId);

    WorldPacket packet(LOGON_INIT_NODE);
    packet << sRoutingHelper->GetNodeCount();
    //sLog->outError("ControlSession::SendInitAuthedNode() for node %u sRoutingHelper->GetNodeCount() %u", _nodeId, sRoutingHelper->GetNodeCount());

    //ItemGUID
    if (uint32 LastItemGUID = sRoutingHelper->GetNodeConnectionData(_nodeId).LastItemGUID)
    {
        packet << LastItemGUID;
        //sLog->outError("ControlSession::SendInitAuthedNode() for node %u LastItemGUID %u", _nodeId, LastItemGUID);
    }
    else
    {
        LastItemGUID = sObjectMgr->m_hiItemGuid;
        //sLog->outError("ControlSession::SendInitAuthedNode() for node %u sObjectMgr->m_hiItemGuid %u", _nodeId, sObjectMgr->m_hiItemGuid);
        LastItemGUID += _nodeId - 1; // That's why we do not allow nodeId = 0 AND it also is the id for the logon itself
        //sLog->outError("ControlSession::SendInitAuthedNode() for node %u sObjectMgr->m_hiItemGuid %u", _nodeId, LastItemGUID);

        sRoutingHelper->SetLastGUID(_nodeId, ENTITYID_ITEM, LastItemGUID);
        packet << LastItemGUID;
        //sLog->outError("ControlSession::SendInitAuthedNode() for node %u LastItemGUID %u", _nodeId, LastItemGUID);
    }

    //MailGUID
    if (uint32 LastMailGUID = sRoutingHelper->GetNodeConnectionData(_nodeId).LastMailGUID)
        packet << LastMailGUID;
    else
    {
        LastMailGUID = sObjectMgr->m_mailid;
        LastMailGUID += _nodeId - 1; // That's why we do not allow nodeId = 0 AND it also is the id for the logon itself

        sRoutingHelper->SetLastGUID(_nodeId, ENTITYID_MAIL, LastMailGUID);
        packet << LastMailGUID;
    }

    //GroupGUID
    packet << _nodeId;
    //if (uint32 LastGroupGUID = sRoutingHelper->GetNodeConnectionData(_nodeId).LastGroupGUID)
    //    packet << LastGroupGUID;
    //else // This only happens at startup
    //{
    //    LastGroupGUID = sObjectMgr->m_hiGroupGuid;
    //    LastGroupGUID += _nodeId - 1; // That's why we do not allow nodeId = 0 AND it also is the id for the logon itself
    //
    //    sRoutingHelper->SetLastGUID(_nodeId, ENTITYID_GROUP, LastGroupGUID);
    //    packet << LastGroupGUID;
    //}

    //CorpseGUID
    if (uint32 LastCorpseID = sRoutingHelper->GetNodeConnectionData(_nodeId).LastCorpseID)
        packet << LastCorpseID;
    else
    {
        LastCorpseID = sObjectMgr->m_hiCorpseGuid;
        LastCorpseID += _nodeId - 1; // That's why we do not allow nodeId = 0 AND it also is the id for the logon itself

        sRoutingHelper->SetLastGUID(_nodeId, ENTITYID_CORPSE, LastCorpseID);
        packet << LastCorpseID;
    }

    //Rates
    packet << sLogon->getRate(RATE_HEALTH);
    packet << sLogon->getRate(RATE_POWER_MANA);
    packet << sLogon->getRate(RATE_POWER_RAGE_INCOME);
    packet << sLogon->getRate(RATE_POWER_RAGE_LOSS);
    packet << sLogon->getRate(RATE_POWER_RUNICPOWER_INCOME);
    packet << sLogon->getRate(RATE_POWER_RUNICPOWER_LOSS);
    packet << sLogon->getRate(RATE_POWER_FOCUS);
    packet << sLogon->getRate(RATE_POWER_ENERGY);
    packet << sLogon->getRate(RATE_SKILL_DISCOVERY);
    packet << sLogon->getRate(RATE_DROP_ITEM_POOR);
    packet << sLogon->getRate(RATE_DROP_ITEM_NORMAL);
    packet << sLogon->getRate(RATE_DROP_ITEM_UNCOMMON);
    packet << sLogon->getRate(RATE_DROP_ITEM_RARE);
    packet << sLogon->getRate(RATE_DROP_ITEM_EPIC);
    packet << sLogon->getRate(RATE_DROP_ITEM_LEGENDARY);
    packet << sLogon->getRate(RATE_DROP_ITEM_ARTIFACT);
    packet << sLogon->getRate(RATE_DROP_ITEM_REFERENCED);
    packet << sLogon->getRate(RATE_DROP_ITEM_REFERENCED_AMOUNT);
    packet << sLogon->getRate(RATE_DROP_MONEY);
    packet << sLogon->getRate(RATE_XP_KILL);
    packet << sLogon->getRate(RATE_XP_QUEST);
    packet << sLogon->getRate(RATE_XP_EXPLORE);
    packet << sLogon->getRate(RATE_REPAIRCOST);
    packet << sLogon->getRate(RATE_REST_INGAME);
    packet << sLogon->getRate(RATE_REST_OFFLINE_IN_TAVERN_OR_CITY);
    packet << sLogon->getRate(RATE_REST_OFFLINE_IN_WILDERNESS);
    packet << sLogon->getRate(RATE_DAMAGE_FALL);
    packet << sLogon->getRate(RATE_AUCTION_TIME);
    packet << sLogon->getRate(RATE_AUCTION_DEPOSIT);
    packet << sLogon->getRate(RATE_AUCTION_CUT);
    packet << sLogon->getRate(RATE_HONOR);
    packet << sLogon->getRate(RATE_MINING_AMOUNT);
    packet << sLogon->getRate(RATE_MINING_NEXT);
    packet << sLogon->getRate(RATE_TALENT);
    packet << sLogon->getRate(RATE_REPUTATION_GAIN);
    packet << sLogon->getRate(RATE_REPUTATION_LOWLEVEL_KILL);
    packet << sLogon->getRate(RATE_REPUTATION_LOWLEVEL_QUEST);
    packet << sLogon->getRate(RATE_REPUTATION_RECRUIT_A_FRIEND_BONUS);

    packet << sLogon->getIntConfig(CONFIG_SKILL_GAIN_CRAFTING);
    packet << sLogon->getIntConfig(CONFIG_SKILL_GAIN_DEFENSE);
    packet << sLogon->getIntConfig(CONFIG_SKILL_GAIN_GATHERING);
    packet << sLogon->getIntConfig(CONFIG_SKILL_GAIN_WEAPON);
    packet << sLogon->getIntConfig(CONFIG_SKILL_CHANCE_ORANGE);
    packet << sLogon->getIntConfig(CONFIG_SKILL_CHANCE_YELLOW);
    packet << sLogon->getIntConfig(CONFIG_SKILL_CHANCE_GREEN);
    packet << sLogon->getIntConfig(CONFIG_SKILL_CHANCE_GREY);
    packet << sLogon->getIntConfig(CONFIG_SKILL_CHANCE_MINING_STEPS);
    packet << sLogon->getIntConfig(CONFIG_SKILL_CHANCE_SKINNING_STEPS);
    //Send
    SendPacket(&packet);
}

