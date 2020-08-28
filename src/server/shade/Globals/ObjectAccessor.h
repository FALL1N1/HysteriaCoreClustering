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

#ifndef TRINITY_OBJECTACCESSOR_H
#define TRINITY_OBJECTACCESSOR_H

#include "Define.h"
#include <ace/Singleton.h>
#include <ace/Thread_Mutex.h>
#include <unordered_map>
#include <set>


template <class T>
class HashMapHolder
{
    public:

        typedef std::unordered_map<uint64, T*> MapType;
        typedef ACE_RW_Thread_Mutex LockType;

        static void Insert(T* o)
        {
            TRINITY_WRITE_GUARD(LockType, i_lock);
            m_objectMap[o->GetGUID()] = o;
        }

        static void Remove(T* o)
        {
            TRINITY_WRITE_GUARD(LockType, i_lock);
            m_objectMap.erase(o->GetGUID());
        }

        static T* Find(uint64 guid)
        {
            TRINITY_READ_GUARD(LockType, i_lock);
            typename MapType::iterator itr = m_objectMap.find(guid);
            return (itr != m_objectMap.end()) ? itr->second : NULL;
        }

        static MapType& GetContainer() { return m_objectMap; }

        static LockType* GetLock() { return &i_lock; }

    private:

        //Non instanceable only static
        HashMapHolder() {}

        static LockType i_lock;
        static MapType  m_objectMap;
};

class ObjectAccessor
{
    friend class ACE_Singleton<ObjectAccessor, ACE_Null_Mutex>;
    private:
        ObjectAccessor();
        ~ObjectAccessor();
        ObjectAccessor(const ObjectAccessor&);
        ObjectAccessor& operator=(const ObjectAccessor&);

    public:
        // TODO: override these template functions for each holder type and add assertions

        template<class T> static T* GetObjectInOrOutOfWorld(uint64 guid, T* /*typeSpecifier*/)
        {
            return HashMapHolder<T>::Find(guid);
        }
         
        // returns object if is in world
        template<class T> static T* GetObjectInWorld(uint64 guid, T* /*typeSpecifier*/)
        {
            return HashMapHolder<T>::Find(guid);
        }

        // Player may be not in world while in ObjectAccessor
        static Player* GetObjectInWorld(uint64 guid, Player* /*typeSpecifier*/); 

        template<class T> static T* GetObjectInWorld(uint32 mapid, float x, float y, uint64 guid, T*) //fake
        {
            T* obj = HashMapHolder<T>::Find(guid);
            if (!obj || obj->GetMapId() != mapid)
                return NULL;
            else return obj;
            /*
            CellCoord p = Trinity::ComputeCellCoord(x, y);
            if (!p.IsCoordValid())
            {
                sLog->outError("ObjectAccessor::GetObjectInWorld: invalid coordinates supplied X:%f Y:%f grid cell [%u:%u]", x, y, p.x_coord, p.y_coord);
                return NULL;
            }

            CellCoord q = Trinity::ComputeCellCoord(obj->GetPositionX(), obj->GetPositionY());
            if (!q.IsCoordValid())
            {
                sLog->outError("ObjectAccessor::GetObjecInWorld: object (GUID: %u TypeId: %u) has invalid coordinates X:%f Y:%f grid cell [%u:%u]", obj->GetGUIDLow(), obj->GetTypeId(), obj->GetPositionX(), obj->GetPositionY(), q.x_coord, q.y_coord);
                return NULL;
            }

            int32 dx = int32(p.x_coord) - int32(q.x_coord);
            int32 dy = int32(p.y_coord) - int32(q.y_coord);

            if (dx > -2 && dx < 2 && dy > -2 && dy < 2)
                return obj;
            else
                return NULL;*/
        }

        // these functions return objects if found in whole world
        // ACCESS LIKE THAT IS NOT THREAD SAFE
        //static Player* FindPlayer(uint64);
        static Player* FindPlayerInOrOutOfWorld(uint64 m_guid);  
        static Player* FindPlayerByName(std::string const& name, bool checkInWorld = true);
        static std::map<std::string, Player*> playerNameToPlayerPointer; // pussywizard: optimization

        // when using this, you must use the hashmapholder's lock
        static HashMapHolder<Player>::MapType const& GetPlayers()
        {
            return HashMapHolder<Player>::GetContainer();
        } 

        template<class T> static void AddObject(T* object)
        {
            HashMapHolder<T>::Insert(object);
        }

        template<class T> static void RemoveObject(T* object)
        {
            HashMapHolder<T>::Remove(object);
        }

        static void SaveAllPlayers();

        //Thread unsafe
        void Update(uint32 diff);
        void RemoveOldCorpses();
        void UnloadAll();

    private:
        void _update();
         
        std::list<uint64> i_playerBones;

        ACE_Thread_Mutex i_objectLock;
};

#define sObjectAccessor ACE_Singleton<ObjectAccessor, ACE_Null_Mutex>::instance()
#endif
