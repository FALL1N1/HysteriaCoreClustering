/*


 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "ObjectMgr.h"
#include "WorldPacket.h"
#include "ArenaTeam.h"
#include "Proxy.h" 
#include "ArenaTeamMgr.h"
#include "Player.h"
#include "ClientSession.h"
#include "Opcodes.h"

ArenaTeam::ArenaTeam()
    : TeamId(0), Type(0), TeamName(), CaptainGuid(0), BackgroundColor(0), EmblemStyle(0), EmblemColor(0),
    BorderStyle(0), BorderColor(0)
{
    Stats.WeekGames   = 0;
    Stats.SeasonGames = 0;
    Stats.Rank        = 0;
    Stats.Rating      = 0; // sWorld->getIntConfig(CONFIG_ARENA_START_RATING);
    Stats.WeekWins    = 0;
    Stats.SeasonWins  = 0;
}

ArenaTeam::~ArenaTeam()
{
}

bool ArenaTeam::Create(uint64 captainGuid, uint8 type, std::string const& teamName, uint32 backgroundColor, uint8 emblemStyle, uint32 emblemColor, uint8 borderStyle, uint32 borderColor)
{
    return true;
}

bool ArenaTeam::AddMember(uint64 playerGuid)
{
    return true;
}

bool ArenaTeam::LoadArenaTeamFromDB(QueryResult result)
{
    return true;
}

bool ArenaTeam::LoadMembersFromDB(QueryResult result)
{
   
    return true;
}

void ArenaTeam::SetCaptain(uint64 guid)
{
  
}

void ArenaTeam::DelMember(uint64 guid, bool cleanDb)
{
  
}

void ArenaTeam::Disband(WorldSession* session)
{
  
}

void ArenaTeam::Roster(WorldSession* session)
{
  
}

void ArenaTeam::Query(WorldSession* session)
{
 
}

void ArenaTeam::SendStats(WorldSession* session)
{
 
}

void ArenaTeam::NotifyStatsChanged()
{
  
}

void ArenaTeam::Inspect(WorldSession* session, uint64 guid)
{
 
}

void ArenaTeamMember::ModifyPersonalRating(Player* player, int32 mod, uint32 type)
{
 
}

void ArenaTeamMember::ModifyMatchmakerRating(int32 mod, uint32 /*slot*/)
{
  
}

void ArenaTeam::BroadcastPacket(WorldPacket* packet)
{

}

void ArenaTeam::BroadcastEvent(ArenaTeamEvents event, uint64 guid, uint8 strCount, std::string const& str1, std::string const& str2, std::string const& str3)
{
  
}

void ArenaTeam::MassInviteToEvent(WorldSession* session)
{
  
}

uint8 ArenaTeam::GetSlotByType(uint32 type)
{
    return 0;
}

bool ArenaTeam::IsMember(uint64 guid) const
{
    return false;
}

uint32 ArenaTeam::GetPoints(uint32 memberRating)
{
    return 0;
}

uint32 ArenaTeam::GetAverageMMR(Group* group) const
{
    return 0;
}

float ArenaTeam::GetChanceAgainst(uint32 ownRating, uint32 opponentRating)
{
    return 0;
}

int32 ArenaTeam::GetMatchmakerRatingMod(uint32 ownRating, uint32 opponentRating, bool won /*, float& confidence_factor*/)
{
    return 0;
}

int32 ArenaTeam::GetRatingMod(uint32 ownRating, uint32 opponentRating, bool won /*, float confidence_factor*/)
{
    return 0;

}

void ArenaTeam::FinishGame(int32 mod)
{

}

int32 ArenaTeam::WonAgainst(uint32 Own_MMRating, uint32 Opponent_MMRating, int32& rating_change)
{
    return 0;

}

int32 ArenaTeam::LostAgainst(uint32 Own_MMRating, uint32 Opponent_MMRating, int32& rating_change)
{
    return 0;

}

void ArenaTeam::MemberLost(Player* player, uint32 againstMatchmakerRating, int32 MatchmakerRatingChange)
{

}

void ArenaTeam::MemberWon(Player* player, uint32 againstMatchmakerRating, int32 MatchmakerRatingChange)
{

}

void ArenaTeam::UpdateArenaPointsHelper(std::map<uint32, uint32>& playerPoints)
{

}

void ArenaTeam::SaveToDB()
{

}

void ArenaTeam::FinishWeek()
{

}

bool ArenaTeam::IsFighting() const
{
    return false;
}
