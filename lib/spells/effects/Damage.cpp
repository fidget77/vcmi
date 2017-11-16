/*
 * Damage.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "Damage.h"
#include "Registry.h"
#include "../ISpellMechanics.h"

#include "../../NetPacks.h"
#include "../../CStack.h"
#include "../../battle/IBattleState.h"
#include "../../battle/CBattleInfoCallback.h"


static const std::string EFFECT_NAME = "core:damage";

namespace spells
{
namespace effects
{

VCMI_REGISTER_SPELL_EFFECT(Damage, EFFECT_NAME);

Damage::Damage(const int level)
	: UnitEffect(level)
{
}

Damage::~Damage() = default;

void Damage::apply(const PacketSender * server, RNG & rng, const Mechanics * m, const EffectTarget & target) const
{
	StacksInjured stacksInjured;
	prepareEffects(stacksInjured, rng, m, target, true);
	if(!stacksInjured.stacks.empty())
		server->sendAndApply(&stacksInjured);
}

void Damage::apply(IBattleState * battleState, RNG & rng, const Mechanics * m, const EffectTarget & target) const
{
	StacksInjured stacksInjured;
	prepareEffects(stacksInjured, rng, m, target, false);
	stacksInjured.applyBattle(battleState);
}

void Damage::serializeJsonUnitEffect(JsonSerializeFormat & handler)
{
	serializeJsonDamageEffect(handler);
}

void Damage::serializeJsonDamageEffect(JsonSerializeFormat & handler)
{
	UNUSED(handler);
}

int64_t Damage::damageForTarget(size_t targetIndex, const Mechanics * m, const battle::Unit * target) const
{
	UNUSED(targetIndex);
	return m->owner->adjustRawDamage(m->caster, target, m->getEffectValue());
}

void Damage::describeEffect(std::vector<MetaString> & log, const Mechanics * m, const battle::Unit * firstTarget, uint32_t kills, int64_t damage, bool multiple) const
{
	{
		MetaString line;

		line.addTxt(MetaString::GENERAL_TXT, 376);
		line.addReplacement(MetaString::SPELL_NAME, m->getSpellIndex());
		line.addReplacement(damage);

		log.push_back(line);
	}

	{
		MetaString line;
		const int textId = (kills > 1) ? 379 : 378;
		line.addTxt(MetaString::GENERAL_TXT, textId);

		if(kills > 1)
			line.addReplacement(kills);

		if(kills > 1)
			if(multiple || !firstTarget)
				line.addReplacement(MetaString::GENERAL_TXT, 43);
			else
				firstTarget->addNameReplacement(line, true);
		else
			if(multiple || !firstTarget)
				line.addReplacement(MetaString::GENERAL_TXT, 42);
			else
				firstTarget->addNameReplacement(line, false);

		log.push_back(line);
	}
}

void Damage::prepareEffects(StacksInjured & stacksInjured, RNG & rng, const Mechanics * m, const EffectTarget & target, bool describe) const
{
	size_t targetIndex = 0;
	const battle::Unit * firstTarget = nullptr;

	int64_t damageToDisplay = 0;
	uint32_t killed = 0;
	bool multiple = false;

	for(auto & t : target)
	{
		const battle::Unit * unit = t.unitValue;
		if(unit && unit->alive())
		{
			BattleStackAttacked bsa;
			bsa.damageAmount = damageForTarget(targetIndex, m, unit);
			bsa.stackAttacked = unit->unitId();
			bsa.attackerID = -1;
			auto newState = unit->asquire();
			CStack::prepareAttacked(bsa, rng, newState);

			if(describe)
			{
				if(!firstTarget)
					firstTarget = unit;
				else
					multiple = true;
				damageToDisplay += bsa.damageAmount;
				killed += bsa.killedAmount;
			}

			stacksInjured.stacks.push_back(bsa);
		}
		targetIndex++;
	}

	if(describe && firstTarget && damageToDisplay > 0)
		describeEffect(stacksInjured.battleLog, m, firstTarget, killed, damageToDisplay, multiple);
}

} // namespace effects
} // namespace spells
