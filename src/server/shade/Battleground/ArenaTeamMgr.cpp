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

#include "Define.h"
#include "ArenaTeamMgr.h"
#include "Proxy.h"
#include "Log.h"
#include "DatabaseEnv.h"
#include "Language.h" 
#include "Player.h"

ArenaTeamMgr::ArenaTeamMgr()
{
    NextArenaTeamId = 1;
    LastArenaLogId = 0;
}

ArenaTeamMgr::~ArenaTeamMgr()
{
    return;
}

// Arena teams collection
ArenaTeam* ArenaTeamMgr::GetArenaTeamById(uint32 arenaTeamId) const
{
    return nullptr;
}

ArenaTeam* ArenaTeamMgr::GetArenaTeamByName(const std::string& arenaTeamName) const
{
    return nullptr;
}

ArenaTeam* ArenaTeamMgr::GetArenaTeamByCaptain(uint64 guid) const
{
    return nullptr;
}

void ArenaTeamMgr::AddArenaTeam(ArenaTeam* arenaTeam)
{
    return;
}

void ArenaTeamMgr::RemoveArenaTeam(uint32 arenaTeamId)
{
    return;
}

uint32 ArenaTeamMgr::GenerateArenaTeamId()
{
    return 0;
}

void ArenaTeamMgr::LoadArenaTeams()
{
    return;
}

void ArenaTeamMgr::DistributeArenaPoints()
{
    return;
}
