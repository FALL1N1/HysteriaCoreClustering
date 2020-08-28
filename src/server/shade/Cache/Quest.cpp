#include "Quest.h"
#include "WorldPacket.h"
#include "Opcodes.h"
#include "ClientSession.h"
#include "LocalesMgr.h"
#include "Formulas.h"

void Quest::WarmingCache()
{
    uint32 oldMSTime = getMSTime();

    // For reload case
    for (QuestTemplateContainer::const_iterator itr=_questTemplateStore.begin(); itr != _questTemplateStore.end(); ++itr)
        delete itr->second;
    _questTemplateStore.clear();

   // mExclusiveQuestGroups.clear();

    QueryResult result = WorldDatabase.Query("SELECT "
        //0     1      2        3        4           5       6            7             8              9               10             11                 12
        "Id, Method, Level, MinLevel, MaxLevel, ZoneOrSort, Type, SuggestedPlayers, LimitTime, RequiredClasses, RequiredRaces, RequiredSkillId, RequiredSkillPoints, "
        //         13                 14                    15                   16                      17                  18                         19                  20
        "RequiredFactionId1, RequiredFactionId2, RequiredFactionValue1, RequiredFactionValue2, RequiredMinRepFaction, RequiredMaxRepFaction, RequiredMinRepValue, RequiredMaxRepValue, "
        //     21         22             23                24             25              26                    27                28            29              30              31
        "PrevQuestId, NextQuestId, ExclusiveGroup, NextQuestIdChain, RewardXPId, RewardOrRequiredMoney, RewardMoneyMaxLevel, RewardSpell, RewardSpellCast, RewardHonor, RewardHonorMultiplier, "
        //         32                  33            34             35               36         37         38            39                40                41               42
        "RewardMailTemplateId, RewardMailDelay, SourceItemId, SourceItemCount, SourceSpellId, Flags, SpecialFlags, RewardTitleId, RequiredPlayerKills, RewardTalents, RewardArenaPoints, "
        //      43            44             45              46              47               48                 49               50
        "RewardItemId1, RewardItemId2, RewardItemId3, RewardItemId4, RewardItemCount1, RewardItemCount2, RewardItemCount3, RewardItemCount4, "
        //         51                  52                  53                    54                    55                   56                      57                  58                        59                       60                      61                      62
        "RewardChoiceItemId1, RewardChoiceItemId2, RewardChoiceItemId3, RewardChoiceItemId4, RewardChoiceItemId5, RewardChoiceItemId6, RewardChoiceItemCount1, RewardChoiceItemCount2, RewardChoiceItemCount3, RewardChoiceItemCount4, RewardChoiceItemCount5, RewardChoiceItemCount6, "
        //        63                64                65                66               67                    68                        69                        70                        71                        72
        "RewardFactionId1, RewardFactionId2, RewardFactionId3, RewardFactionId4, RewardFactionId5, RewardFactionValueId1, RewardFactionValueId2, RewardFactionValueId3, RewardFactionValueId4, RewardFactionValueId5, "
        //                73                                  74                              75                               76                                77
        "RewardFactionValueIdOverride1, RewardFactionValueIdOverride2, RewardFactionValueIdOverride3, RewardFactionValueIdOverride4, RewardFactionValueIdOverride5, "
        //    78        79      80         81
        "PointMapId, PointX, PointY, PointOption, "
        // 82        83         84      85           86                87              88
        "Title, Objectives, Details, EndText, OfferRewardText, RequestItemsText, CompletedText, "
        //        89               90                 91                92                 93                    94                       95                     96
        "RequiredNpcOrGo1, RequiredNpcOrGo2, RequiredNpcOrGo3, RequiredNpcOrGo4, RequiredNpcOrGoCount1, RequiredNpcOrGoCount2, RequiredNpcOrGoCount3, RequiredNpcOrGoCount4, "
        //          97                     98                     99                   100                        101                         102                        103                        104
        "RequiredSourceItemId1, RequiredSourceItemId2, RequiredSourceItemId3, RequiredSourceItemId4, RequiredSourceItemCount1, RequiredSourceItemCount2, RequiredSourceItemCount3, RequiredSourceItemCount4, "
        //       105               106             107             108              109             110                 111                   112                   113                  114                   115                   116
        "RequiredItemId1, RequiredItemId2, RequiredItemId3, RequiredItemId4, RequiredItemId5, RequiredItemId6, RequiredItemCount1, RequiredItemCount2, RequiredItemCount3, RequiredItemCount4, RequiredItemCount5, RequiredItemCount6, "
        //        117                 118                 119                120             121          122            123              124            125
        "AddedInBuild, AddedInBuild, AddedInBuild, AddedInBuild, Unknown0, ObjectiveText1, ObjectiveText2, ObjectiveText3, ObjectiveText4, "
        //     126            127            128            129              130                  131                132                 133                134              135
        "DetailsEmote1, DetailsEmote2, DetailsEmote3, DetailsEmote4, DetailsEmoteDelay1, DetailsEmoteDelay2, DetailsEmoteDelay3, DetailsEmoteDelay4, EmoteOnIncomplete, EmoteOnComplete, "
        //      136                 137                 138               139                  140                     141                     142                      143
        "OfferRewardEmote1, OfferRewardEmote2, OfferRewardEmote3, OfferRewardEmote4, OfferRewardEmoteDelay1, OfferRewardEmoteDelay2, OfferRewardEmoteDelay3, OfferRewardEmoteDelay4, "
        //    144           145           146
        "AddedInBuild, AddedInBuild, AddedInBuild"
        " FROM quest_template");
    if (!result)
    {
        sLog->outErrorDb(">> Loaded 0 quests definitions. DB table `quest_template` is empty.");
        sLog->outString();
        return;
    }
    uint32 count = 0;
    // create multimap previous quest for each existed quest
    // some quests can have many previous maps set by NextQuestId in previous quest
    // for example set of race quests can lead to single not race specific quest
    do
    {
        Field* fields = result->Fetch();

        QuestTemplate* newQuest = new QuestTemplate(fields);
        _questTemplateStore[newQuest->GetQuestId()] = newQuest;
        count++;
    } while (result->NextRow());
    sLog->outString(">> Loaded %u quest templates in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
}

QuestTemplate const* Quest::GetQuestTemplate(uint32 entry)
{
    QuestTemplateContainer::const_iterator itr = _questTemplateStore.find(entry);
    if (itr != _questTemplateStore.end())
        return (itr->second);

    return NULL;
}

void Quest::CacheQuestTemplate(QuestTemplate* quest)
{
    //To prevent a crash we should set an Mutex here
    TRINITY_WRITE_GUARD(ACE_RW_Thread_Mutex, rwMutex_);

    QuestTemplate* _quest = _questTemplateStore[quest->Id];
    _quest = quest;
    return;
}

void ClientSession::HandleQuestQueryOpcode(WorldPacket & recv_data)
{
    if (!_player)
        return;

    uint32 questId;
    recv_data >> questId;

    QuestTemplate const* quest = sQuest->GetQuestTemplate(questId);
    if (quest)
    {

        std::string questTitle = quest->GetTitle();
        std::string questDetails = quest->GetDetails();
        std::string questObjectives = quest->GetObjectives();
        std::string questEndText = quest->GetEndText();
        std::string questCompletedText = quest->GetCompletedText();

        std::string questObjectiveText[QUEST_OBJECTIVES_COUNT];
        for (uint32 i = 0; i < QUEST_OBJECTIVES_COUNT; ++i)
            questObjectiveText[i] = quest->ObjectiveText[i];

        int32 locale = GetSessionDbLocaleIndex();
        if (locale >= 0)
        {
            if (QuestLocale const* localeData = sLocalesMgr->GetQuestLocale(quest->GetQuestId()))
            {
                LocalesMgr::GetLocaleString(localeData->Title, locale, questTitle);
                LocalesMgr::GetLocaleString(localeData->Details, locale, questDetails);
                LocalesMgr::GetLocaleString(localeData->Objectives, locale, questObjectives);
                LocalesMgr::GetLocaleString(localeData->EndText, locale, questEndText);
                LocalesMgr::GetLocaleString(localeData->CompletedText, locale, questCompletedText);

                for (int i = 0; i < QUEST_OBJECTIVES_COUNT; ++i)
                    LocalesMgr::GetLocaleString(localeData->ObjectiveText[i], locale, questObjectiveText[i]);
            }
        }

        WorldPacket data(SMSG_QUEST_QUERY_RESPONSE, 100);       // guess size

        data << uint32(quest->GetQuestId());                    // quest id
        data << uint32(quest->GetQuestMethod());                // Accepted values: 0, 1 or 2. 0 == IsAutoComplete() (skip objectives/details)
        data << uint32(quest->GetQuestLevel());                 // may be -1, static data, in other cases must be used dynamic level: Player::GetQuestLevel (0 is not known, but assuming this is no longer valid for quest intended for client)
        data << uint32(quest->GetMinLevel());                   // min level
        data << uint32(quest->GetZoneOrSort());                 // zone or sort to display in quest log

        data << uint32(quest->GetType());                       // quest type
        data << uint32(quest->GetSuggestedPlayers());           // suggested players count

        data << uint32(quest->GetRepObjectiveFaction());        // shown in quest log as part of quest objective
        data << uint32(quest->GetRepObjectiveValue());          // shown in quest log as part of quest objective

        data << uint32(quest->GetRepObjectiveFaction2());       // shown in quest log as part of quest objective OPPOSITE faction
        data << uint32(quest->GetRepObjectiveValue2());         // shown in quest log as part of quest objective OPPOSITE faction

        data << uint32(quest->GetNextQuestInChain());           // client will request this quest from NPC, if not 0
        data << uint32(quest->GetXPId());                       // used for calculating rewarded experience

        if (quest->HasFlag(QUEST_FLAGS_HIDDEN_REWARDS))
            data << uint32(0);                                  // Hide money rewarded
        else
            data << uint32(quest->GetRewOrReqMoney());          // reward money (below max lvl)

        data << uint32(quest->GetRewMoneyMaxLevel());           // used in XP calculation at client
        data << uint32(quest->GetRewSpell());                   // reward spell, this spell will display (icon) (casted if RewSpellCast == 0)
        data << int32(quest->GetRewSpellCast());                // casted spell

        // rewarded honor points
        data << Trinity::Honor::hk_honor_at_level(GetPlayer()->getLevel(), quest->GetRewHonorMultiplier());
        data << float(0);                                       // new reward honor (multipled by ~62 at client side)
        data << uint32(quest->GetSrcItemId());                  // source item id
        data << uint32(quest->GetFlags() & 0xFFFF);             // quest flags
        data << uint32(quest->GetCharTitleId());                // CharTitleId, new 2.4.0, player gets this title (id from CharTitles)
        data << uint32(quest->GetPlayersSlain());               // players slain
        data << uint32(quest->GetBonusTalents());               // bonus talents
        data << uint32(quest->GetRewArenaPoints());             // bonus arena points
        data << uint32(0);                                      // review rep show mask

        if (quest->HasFlag(QUEST_FLAGS_HIDDEN_REWARDS))
        {
            for (uint32 i = 0; i < QUEST_REWARDS_COUNT; ++i)
                data << uint32(0) << uint32(0);
            for (uint32 i = 0; i < QUEST_REWARD_CHOICES_COUNT; ++i)
                data << uint32(0) << uint32(0);
        }
        else
        {
            for (uint32 i = 0; i < QUEST_REWARDS_COUNT; ++i)
            {
                data << uint32(quest->RewardItemId[i]);
                data << uint32(quest->RewardItemIdCount[i]);
            }
            for (uint32 i = 0; i < QUEST_REWARD_CHOICES_COUNT; ++i)
            {
                data << uint32(quest->RewardChoiceItemId[i]);
                data << uint32(quest->RewardChoiceItemCount[i]);
            }
        }

        for (uint32 i = 0; i < QUEST_REPUTATIONS_COUNT; ++i)        // reward factions ids
            data << uint32(quest->RewardFactionId[i]);

        for (uint32 i = 0; i < QUEST_REPUTATIONS_COUNT; ++i)        // columnid+1 QuestFactionReward.dbc?
            data << int32(quest->RewardFactionValueId[i]);

        for (int i = 0; i < QUEST_REPUTATIONS_COUNT; ++i)           // unk (0)
            data << int32(quest->RewardFactionValueIdOverride[i]);

        data << quest->GetPointMapId();
        data << quest->GetPointX();
        data << quest->GetPointY();
        data << quest->GetPointOpt();

        data << questTitle;
        data << questObjectives;
        data << questDetails;
        data << questEndText;
        data << questCompletedText;                                  // display in quest objectives window once all objectives are completed

        for (uint32 i = 0; i < QUEST_OBJECTIVES_COUNT; ++i)
        {
            if (quest->RequiredNpcOrGo[i] < 0)
                data << uint32((quest->RequiredNpcOrGo[i] * (-1)) | 0x80000000);    // client expects gameobject template id in form (id|0x80000000)
            else
                data << uint32(quest->RequiredNpcOrGo[i]);

            data << uint32(quest->RequiredNpcOrGoCount[i]);
            data << uint32(quest->RequiredSourceItemId[i]);
            data << uint32(0);                                  // req source count?
        }

        for (uint32 i = 0; i < QUEST_ITEM_OBJECTIVES_COUNT; ++i)
        {
            data << uint32(quest->RequiredItemId[i]);
            data << uint32(quest->RequiredItemCount[i]);
        }

        for (uint32 i = 0; i < QUEST_OBJECTIVES_COUNT; ++i)
            data << questObjectiveText[i];

        SendPacket(&data);
        sLog->outString("WORLD: Sent SMSG_QUEST_QUERY_RESPONSE questid=%u", quest->GetQuestId());
    }
    else
        SendPacketToNode(&recv_data);
}