/*
 * Timed.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "Timed.h"
#include "Registry.h"
#include "../ISpellMechanics.h"

#include "../../NetPacks.h"
#include "../../battle/IBattleState.h"
#include "../../CStack.h"
#include "../../serializer/JsonSerializeFormat.h"

static const std::string EFFECT_NAME = "core:timed";

namespace spells
{
namespace effects
{

VCMI_REGISTER_SPELL_EFFECT(Timed, EFFECT_NAME);

Timed::Timed(const int level)
	: StackEffect(level),
	cumulative(false),
	bonus()
{
}

Timed::~Timed() = default;


void Timed::convertBonus(const Mechanics * m, int32_t & duration, std::vector<Bonus> & converted) const
{
	int32_t maxDuration = 0;

	for(const std::shared_ptr<Bonus> & b : bonus)
	{
		Bonus nb(*b);

		//use configured duration if present
		if(nb.turnsRemain == 0)
			nb.turnsRemain = duration;
		vstd::amax(maxDuration, nb.turnsRemain);

		nb.sid = m->getSpellIndex(); //for all
		nb.source = Bonus::SPELL_EFFECT;//for all

		//fix to original config: shield should display damage reduction
		if((nb.sid == SpellID::SHIELD || nb.sid == SpellID::AIR_SHIELD) && (nb.type == Bonus::GENERAL_DAMAGE_REDUCTION))
			nb.val = 100 - nb.val;
		//we need to know who cast Bind
		else if(nb.sid == SpellID::BIND && nb.type == Bonus::BIND_EFFECT && m->casterStack)
			nb.additionalInfo = m->casterStack->ID;

		converted.push_back(nb);
	}

	//if all spell effects have special duration, use it later for special bonuses
	duration = maxDuration;
}

void Timed::apply(const PacketSender * server, RNG & rng, const Mechanics * m, const EffectTarget & target) const
{
	SetStackEffect sse;
	prepareEffects(sse, m, target);

	if(!(sse.toAdd.empty() && sse.toUpdate.empty()))
		server->sendAndApply(&sse);
}

void Timed::apply(IBattleState * battleState, RNG & rng, const Mechanics * m, const EffectTarget & target) const
{
	SetStackEffect sse;
	prepareEffects(sse, m, target);

	for(const auto & stackData : sse.toRemove)
		battleState->removeUnitBonus(stackData.first, stackData.second);

	for(const auto & stackData : sse.toUpdate)
		battleState->updateUnitBonus(stackData.first, stackData.second);

	for(const auto & stackData : sse.toAdd)
		battleState->addUnitBonus(stackData.first, stackData.second);
}

void Timed::prepareEffects(SetStackEffect & sse, const Mechanics * m, const EffectTarget & target) const
{
//get default spell duration (spell power with bonuses for heroes)
	int32_t duration = m->getEffectDuration();

	std::vector<Bonus> converted;
    convertBonus(m, duration, converted);

	std::shared_ptr<Bonus> bonus = nullptr;
	auto casterHero = dynamic_cast<const CGHeroInstance *>(m->caster);
	if(casterHero)
		bonus = casterHero->getBonusLocalFirst(Selector::typeSubtype(Bonus::SPECIAL_PECULIAR_ENCHANT, m->getSpellIndex()));
	//TODO does hero specialty should affects his stack casting spells?

	for(auto & t : target)
	{
		std::vector<Bonus> buffer;
		std::copy(converted.begin(), converted.end(), std::back_inserter(buffer));

		const battle::Unit * affected = t.unitValue;
		if(!affected)
		{
			logGlobal->error("[Internal error] Invalid target for timed effect");
			continue;
		}

		if(!affected->alive())
			continue;

		si32 power = 0;

		//Apply hero specials - peculiar enchants
		const auto tier = std::max(affected->creatureLevel(), 1); //don't divide by 0 for certain creatures (commanders, war machines)
		if(bonus)
		{
			switch(bonus->additionalInfo)
			{
			case 0: //normal
				switch(tier)
				{
				case 1:
				case 2:
					power = 3;
					break;
				case 3:
				case 4:
					power = 2;
					break;
				case 5:
				case 6:
					power = 1;
					break;
				}
				for(const Bonus & b : converted)
				{
					Bonus specialBonus(b);
					specialBonus.val = power; //it doesn't necessarily make sense for some spells, use it wisely
					specialBonus.turnsRemain = duration;

					//additional premy to given effect
					buffer.push_back(specialBonus);
				}
				break;
			case 1: //only Coronius as yet
				power = std::max(5 - tier, 0);
				Bonus specialBonus(Bonus::N_TURNS, Bonus::PRIMARY_SKILL, Bonus::SPELL_EFFECT, power, m->getSpellIndex(), PrimarySkill::ATTACK);
				specialBonus.turnsRemain = duration;
				buffer.push_back(specialBonus);
				break;
			}
		}
		if(casterHero && casterHero->hasBonusOfType(Bonus::SPECIAL_BLESS_DAMAGE, m->getSpellIndex())) //TODO: better handling of bonus percentages
		{
			int damagePercent = casterHero->level * casterHero->valOfBonuses(Bonus::SPECIAL_BLESS_DAMAGE, m->getSpellIndex()) / tier;
			Bonus specialBonus(Bonus::N_TURNS, Bonus::CREATURE_DAMAGE, Bonus::SPELL_EFFECT, damagePercent, m->getSpellIndex(), 0, Bonus::PERCENT_TO_ALL);
			specialBonus.turnsRemain = duration;
			buffer.push_back(specialBonus);
		}

        if(cumulative)
			sse.toAdd.push_back(std::make_pair(affected->unitId(), buffer));
		else
			sse.toUpdate.push_back(std::make_pair(affected->unitId(), buffer));
	}
}

void Timed::serializeJsonEffect(JsonSerializeFormat & handler)
{
	assert(!handler.saving);
	handler.serializeBool("cumulative", cumulative, false);
	{
		auto guard = handler.enterStruct("bonus");
		const JsonNode & data = handler.getCurrent();

		for(const auto & p : data.Struct())
		{
			//TODO: support JsonSerializeFormat in Bonus
			auto guard = handler.enterStruct(p.first);
			const JsonNode & bonusNode = handler.getCurrent();
			auto b = JsonUtils::parseBonus(bonusNode);
			bonus.push_back(b);
		}
	}
}

} // namespace effects
} // namespace spells
