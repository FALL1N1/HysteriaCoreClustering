#include "ScriptMgr.h"
#include "InstanceScript.h"
#include "deadmines.h"

class instance_deadmines : public InstanceMapScript
{
    public:
        instance_deadmines() : InstanceMapScript("instance_deadmines", 36) { }

        struct instance_deadmines_InstanceMapScript : public InstanceScript
        {
            instance_deadmines_InstanceMapScript(Map* map) : InstanceScript(map)
            {
            }

            void Initialize()
            {
                memset(&_encounters, 0, sizeof(_encounters));
            }

            void OnGameObjectCreate(GameObject* gameobject)
            {
                switch (gameobject->GetEntry())
                {
                    case GO_FACTORY_DOOR:
                        if (_encounters[TYPE_RHAHK_ZOR] == DONE)
                            HandleGameObject(0, true, gameobject);
                        break;
                    case GO_IRON_CLAD_DOOR:
                        if (_encounters[TYPE_CANNON] == DONE)
                            HandleGameObject(0, true, gameobject);
                        break;
                }
            }

            void SetData(uint32 type, uint32 data)
            {
                switch (type)
                {
                    case TYPE_RHAHK_ZOR:
                    case TYPE_CANNON:
                        _encounters[type] = data;
                        break;
                }

                if (data == DONE)
                    SaveToDB();
            }

            std::string GetSaveData()
            {
                std::ostringstream saveStream;
                saveStream << "D E " << _encounters[0] << ' ' << _encounters[1];
                return saveStream.str();
            }

            void Load(const char* in)
            {
                if (!in)
                    return;

                char dataHead1, dataHead2;
                std::istringstream loadStream(in);
                loadStream >> dataHead1 >> dataHead2;
                if (dataHead1 == 'D' && dataHead2 == 'E')
                {
                    for (uint8 i = 0; i < MAX_ENCOUNTERS; ++i)
                    {
                        loadStream >> _encounters[i];
                        if (_encounters[i] == IN_PROGRESS)
                            _encounters[i] = NOT_STARTED;
                    }
                }
            }

        private:
            uint32 _encounters[MAX_ENCOUNTERS];
        };

        InstanceScript* GetInstanceScript(InstanceMap* map) const
        {
            return new instance_deadmines_InstanceMapScript(map);
        }
};

#define SPELL_MOLTEN_METAL 5213
#define RAND_TALK 1

enum Events
{
  EVENT_MOLTEN_METAL = 1,
  EVENT_RAND_TALK    = 2,
  EVENT_SUMMON_AFTER = 3,
};

enum Adds
{
    NPC_DEFIAS_TASKMASTER = 4417, // 2x
    NPC_DEFIAS_WIZZARD = 4418,    // 1x

};

class boss_gilnid_the_smelter : public CreatureScript
{
    public:
        boss_gilnid_the_smelter() : CreatureScript("boss_gilnid_the_smelter") { }

        CreatureAI* GetAI(Creature* creature) const
        {
            return GetInstanceAI<boss_gilnid_the_smelterAI>(creature);
        }

        struct boss_gilnid_the_smelterAI : public ScriptedAI
        {
            boss_gilnid_the_smelterAI(Creature* creature) : ScriptedAI(creature) 
            { 
                events.ScheduleEvent(EVENT_RAND_TALK, 1000);
            }
            EventMap events;

            void Reset() { events.Reset(); }

            void EnterCombat(Unit* /*who*/)
            {
                events.ScheduleEvent(EVENT_MOLTEN_METAL, 500);
            }

            void JustSummoned(Creature* creature)
            {
                sLog->outString("!!!YES!!!");
            }

            void SummonDefiasAdds(Creature* me)
            {
                me->GetMap()->SummonCreatureGroup(1);
            }

            void JustDied(Unit* killer)
            {
                SummonDefiasAdds(me);
            }

            void UpdateAI(uint32 diff)
            {
                if (!UpdateVictim())
                    return;

                events.Update(diff);
                switch (events.ExecuteEvent())
                {
                    case EVENT_MOLTEN_METAL:
                        me->CastSpell(me->GetVictim(), SPELL_MOLTEN_METAL, false);
                        events.ScheduleEvent(EVENT_MOLTEN_METAL, 6000);
                        break;
                    case EVENT_RAND_TALK:
                        if (!me->IsInCombat())
                            Talk(urand(1, 3)); // random between 1 and 3
                        events.ScheduleEvent(EVENT_RAND_TALK, 7000);
                        break;
                }

                DoMeleeAttackIfReady();
            }
        };
};

void AddSC_instance_deadmines()
{
    new instance_deadmines();
    new boss_gilnid_the_smelter();
}
