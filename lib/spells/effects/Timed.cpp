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

void Timed::apply(const PacketSender * server, RNG & rng, const Mechanics * m, const BattleCast & p, const EffectTarget & target) const
{
	SetStackEffect sse;
	//get default spell duration (spell power with bonuses for heroes)
	si32 duration = p.effectDuration;
	//generate actual stack bonuses
	//TODO: make cumulative a parameter of SetStackEffect

	auto addUniqueBonus = [&sse, this](Bonus && b, const CStack * s)
	{
		if(cumulative)
			sse.cumulativeUniqueBonuses.push_back(std::pair<ui32, Bonus>(s->ID, b));
		else
			sse.uniqueBonuses.push_back(std::pair<ui32, Bonus>(s->ID, b));
	};

	std::vector<Bonus> & converted = cumulative ? sse.cumulativeEffects : sse.effect;
	{
		si32 maxDuration = 0;

		for(const std::shared_ptr<Bonus> & b : bonus)
		{
			Bonus nb(*b);

			//use configured duration if present
			if(nb.turnsRemain == 0)
				nb.turnsRemain = duration;
			vstd::amax(maxDuration, nb.turnsRemain);

			nb.sid = m->owner->id; //for all
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

	std::shared_ptr<Bonus> bonus = nullptr;
	auto casterHero = dynamic_cast<const CGHeroInstance *>(m->caster);
	if(casterHero)
		bonus = casterHero->getBonusLocalFirst(Selector::typeSubtype(Bonus::SPECIAL_PECULIAR_ENCHANT, m->owner->id));
	//TODO does hero specialty should affects his stack casting spells?

	for(auto & t : target)
	{
		const CStack * affected = t.stackValue;
		if(!affected)
		{
			server->complain("[Internal error] Invalid target for timed effect");
			continue;
		}

		if(!affected->alive())
			continue;

		si32 power = 0;
		sse.stacks.push_back(affected->ID);

		//Apply hero specials - peculiar enchants
		const ui8 tier = std::max((ui8)1, affected->getCreature()->level); //don't divide by 0 for certain creatures (commanders, war machines)
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
					addUniqueBonus(std::move(specialBonus), affected);
				}
				break;
			case 1: //only Coronius as yet
				power = std::max(5 - tier, 0);
				Bonus specialBonus(Bonus::N_TURNS, Bonus::PRIMARY_SKILL, Bonus::SPELL_EFFECT, power, m->owner->id, PrimarySkill::ATTACK);
				specialBonus.turnsRemain = duration;
				addUniqueBonus(std::move(specialBonus), affected);
				break;
			}
		}
		if(casterHero && casterHero->hasBonusOfType(Bonus::SPECIAL_BLESS_DAMAGE, m->owner->id)) //TODO: better handling of bonus percentages
		{
			int damagePercent = casterHero->level * casterHero->valOfBonuses(Bonus::SPECIAL_BLESS_DAMAGE, m->owner->id.toEnum()) / tier;
			Bonus specialBonus(Bonus::N_TURNS, Bonus::CREATURE_DAMAGE, Bonus::SPELL_EFFECT, damagePercent, m->owner->id, 0, Bonus::PERCENT_TO_ALL);
			specialBonus.turnsRemain = duration;
			addUniqueBonus(std::move(specialBonus), affected);
		}
	}

	if(!sse.stacks.empty())
		server->sendAndApply(&sse);
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
