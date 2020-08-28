#include "ClientSession.h"
#include "AchievementMgr.h"
#include "WorldPacket.h"
#include "Player.h"
#include "ObjectMgr.h"
#include "ClusterDefines.h"

void ClientSession::HandleQueryInspectAchievements(WorldPacket & recv_data)
{
    uint64 guid;
    recv_data.readPackGUID(guid);

    Player* player = sObjectMgr->GetPlayer(guid);
    if (!player)
        return;

    player->GetAchievementMgr().SendRespondInspectAchievements(_player);
}