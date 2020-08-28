#include "GameObjects.h"
#include "WorldPacket.h"
#include "Opcodes.h"
#include "ClientSession.h"
#include "LocalesMgr.h"

void GameObjects::WarmingCache()
{
    uint32 oldMSTime = getMSTime();

    //                                                 0      1      2        3       4             5          6      7       8     9        10         11          12
    QueryResult result = WorldDatabase.Query("SELECT entry, type, displayId, name, IconName, castBarCaption, unk1, faction, flags, size, questItem1, questItem2, questItem3, "
    //                                            13          14          15       16     17     18     19     20     21     22     23     24     25      26      27      28
                                             "questItem4, questItem5, questItem6, data0, data1, data2, data3, data4, data5, data6, data7, data8, data9, data10, data11, data12, "
    //                                          29      30      31      32      33      34      35      36      37      38      39      40        41
                                             "data13, data14, data15, data16, data17, data18, data19, data20, data21, data22, data23, AIName, ScriptName "
                                             "FROM gameobject_template");

    if (!result)
    {
        sLog->outString(">> Loaded 0 gameobject definitions. DB table `gameobject_template` is empty.");
        sLog->outString();
        return;
    }

    //_gameObjectTemplateStore.rehash(result->GetRowCount());
    uint32 count = 0;
    do
    {
        Field* fields = result->Fetch();

        uint32 entry = fields[0].GetUInt32();

        GameObjectTemplate& got = _gameObjectTemplateStore[entry];
        
        got.entry          = entry;
        got.type           = uint32(fields[1].GetUInt8());
        got.displayId      = fields[2].GetUInt32();
        got.Name           = fields[3].GetString();
        got.IconName       = fields[4].GetString();
        got.CastBarCaption = fields[5].GetString();
        got.unk1           = fields[6].GetString();
        got.size           = fields[9].GetFloat();

        for (uint8 i = 0; i < MAX_GAMEOBJECT_QUEST_ITEMS; ++i)
            got.questItems[i] = fields[10 + i].GetUInt32();

        for (uint8 i = 0; i < MAX_GAMEOBJECT_DATA; ++i)
            got.raw.data[i] = fields[16 + i].GetInt32(); // data1 and data6 can be -1

       ++count;
    }
    while (result->NextRow());

    sLog->outString(">> Loaded %u game object templates in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
}

GameObjectTemplate const* GameObjects::GetGameObjectTemplate(uint32 entry)
{
    GameObjectTemplateContainer::const_iterator itr = _gameObjectTemplateStore.find(entry);
    if (itr != _gameObjectTemplateStore.end())
        return &(itr->second);

    return NULL;
}

void GameObjects::CacheGameObjectTemplate(GameObjectTemplate* gob)
{
    //To prevent a crash we should set an Mutex here
    TRINITY_WRITE_GUARD(ACE_RW_Thread_Mutex, rwMutex_);

    GameObjectTemplate& go = _gameObjectTemplateStore[gob->entry];
    go = *gob;
    return;
}

/// Only _static_ data is sent in this packet !!!
void ClientSession::HandleGameObjectQueryOpcode(WorldPacket & recv_data)
{
    uint32 entry;
    recv_data >> entry;
    uint64 guid;
    recv_data >> guid;

    const GameObjectTemplate* info = sGameObjects->GetGameObjectTemplate(entry);
    if (info)
    {
        std::string Name;
        std::string IconName;
        std::string CastBarCaption;

        Name = info->Name;
        IconName = info->IconName;
        CastBarCaption = info->CastBarCaption;

        int loc_idx = GetSessionDbLocaleIndex();
        if (loc_idx >= 0)
        {
            if (GameObjectLocale const* gl = sLocalesMgr->GetGameObjectLocale(entry))
            {
                LocalesMgr::GetLocaleString(gl->Name, loc_idx, Name);
                LocalesMgr::GetLocaleString(gl->CastBarCaption, loc_idx, CastBarCaption);
            }
        }
        sLog->outDetail("WORLD: CMSG_GAMEOBJECT_QUERY '%s' - Entry: %u. ", info->Name.c_str(), entry);
        WorldPacket data (SMSG_GAMEOBJECT_QUERY_RESPONSE, 150);
        data << uint32(entry);
        data << uint32(info->type);
        data << uint32(info->displayId);
        data << Name;
        data << uint8(0) << uint8(0) << uint8(0);           // name2, name3, name4
        data << IconName;                                   // 2.0.3, string. Icon name to use instead of default icon for go's (ex: "Attack" makes sword)
        data << CastBarCaption;                             // 2.0.3, string. Text will appear in Cast Bar when using GO (ex: "Collecting")
        data << info->unk1;                                 // 2.0.3, string
        data.append(info->raw.data, 24);
        data << float(info->size);                          // go size
        for (uint32 i = 0; i < MAX_GAMEOBJECT_QUEST_ITEMS; ++i)
            data << uint32(info->questItems[i]);              // itemId[6], quest drop
        SendPacket(&data);
        sLog->outString("WORLD: Sent SMSG_GAMEOBJECT_QUERY_RESPONSE");
    }
    else
        SendPacketToNode(&recv_data);
}

//Opcodes
void ClientSession::HandleSMSG_GAMEOBJECT_QUERY(WorldPacket & recv_data)
{
    uint8 trash;
    GameObjectTemplate* m_template = new GameObjectTemplate;

    uint32 entry;
    recv_data >> entry;

    //Sollte als nicht existent interpretiert werden, denk ich mal
    if (entry >= 0x80000000)
        return;

    m_template->entry = entry;
    recv_data >> m_template->type;
    recv_data >> m_template->displayId;
    recv_data >> m_template->Name;
    recv_data >> trash >> trash >> trash;
    recv_data >> m_template->IconName;
    recv_data >> m_template->CastBarCaption;
    recv_data >> m_template->unk1;
    for (uint32 i = 0; i < MAX_GAMEOBJECT_DATA; ++i)
        recv_data >> m_template->raw.data[i];

    recv_data >> m_template->size;

    for (uint32 i = 0; i < MAX_GAMEOBJECT_QUEST_ITEMS; ++i)
        recv_data >> m_template->questItems[i];              // itemId[6], quest drop

    sGameObjects->CacheGameObjectTemplate(m_template);
}