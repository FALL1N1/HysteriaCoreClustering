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

#include "ObjectAccessor.h"
#include "ObjectDefines.h"
#include "ObjectMgr.h"
#include "Opcodes.h"
#include "Player.h"
#include "Proxy.h"
#include "WorldPacket.h"

#include <cmath>

ObjectAccessor::ObjectAccessor()
{
}

ObjectAccessor::~ObjectAccessor()
{
}
/*
Player* ObjectAccessor::FindPlayer(uint64 guid)
{
    return GetObjectInWorld(guid, (Player*)NULL);
}
*/
Player* ObjectAccessor::FindPlayerInOrOutOfWorld(uint64 guid)
{
    return GetObjectInOrOutOfWorld(guid, (Player*)NULL); 
}

Player* ObjectAccessor::FindPlayerByName(std::string const& name, bool checkInWorld)
{
    /*TRINITY_READ_GUARD(HashMapHolder<Player>::LockType, *HashMapHolder<Player>::GetLock());
    std::string nameStr = name;
    std::transform(nameStr.begin(), nameStr.end(), nameStr.begin(), ::tolower);
    HashMapHolder<Player>::MapType const& m = GetPlayers();
    for (HashMapHolder<Player>::MapType::const_iterator iter = m.begin(); iter != m.end(); ++iter)
    {
        if (!iter->second->IsInWorld())
            continue;
        std::string currentName = iter->second->GetName();
        std::transform(currentName.begin(), currentName.end(), currentName.begin(), ::tolower);
        if (nameStr.compare(currentName) == 0)
            return iter->second;
    }*/

    // pussywizard: optimization
    std::string nameStr = name;
    std::transform(nameStr.begin(), nameStr.end(), nameStr.begin(), ::tolower);
    std::map<std::string, Player*>::iterator itr = playerNameToPlayerPointer.find(nameStr);
    if (itr != playerNameToPlayerPointer.end())
        return itr->second;

    return NULL;
}

std::map<std::string, Player*> ObjectAccessor::playerNameToPlayerPointer;

/// Define the static members of HashMapHolder

template <class T> std::unordered_map< uint64, T* > HashMapHolder<T>::m_objectMap;
template <class T> typename HashMapHolder<T>::LockType HashMapHolder<T>::i_lock;

/// Global definitions for the hashmap storage

template class HashMapHolder<Player>; 

template Player* ObjectAccessor::GetObjectInWorld<Player>(uint32 mapid, float x, float y, uint64 guid, Player* /*fake*/); 
