/*
 * Sacrifice.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "Sacrifice.h"
#include "Registry.h"
#include "../ISpellMechanics.h"

#include "../../NetPacks.h"
#include "../../battle/IBattleState.h"
#include "../../battle/CBattleInfoCallback.h"
#include "../../battle/Unit.h"
#include "../../serializer/JsonSerializeFormat.h"


static const std::string EFFECT_NAME = "core:sacrifice";

namespace spells
{
namespace effects
{

VCMI_REGISTER_SPELL_EFFECT(Sacrifice, EFFECT_NAME);

Sacrifice::Sacrifice(const int level)
	: Heal(level)
{

}

Sacrifice::~Sacrifice() = default;

void Sacrifice::adjustTargetTypes(std::vector<TargetType> & types) const
{
	if(!types.empty())
	{
		if(types[0] != AimType::CREATURE)
		{
			types.clear();
			return;
		}

		if(types.size() == 1)
		{
			types.push_back(AimType::CREATURE);
		}
		else if(types.size() > 1)
		{
			if(types[1] != AimType::CREATURE)
				types.clear();
		}
	}
}

bool Sacrifice::applicable(Problem & problem, const Mechanics * m) const
{
	auto mode = m->mode;

	switch(mode)
	{
	case Mode::HERO:
	case Mode::CREATURE_ACTIVE:
		{
			auto mainFilter = std::bind(&UnitEffect::getStackFilter, this, m, true, _1);
			auto predicate = std::bind(&UnitEffect::eraseByImmunityFilter, this, m, _1);

			auto targets = m->cb->battleGetUnitsIf(mainFilter);
			vstd::erase_if(targets, predicate);

			bool targetExists = false;
			bool targetToSacrificeExists = false;

			for(auto & target : targets)
			{
				if(target->alive())
					targetToSacrificeExists = true;
				else if(target->isDead())
					targetExists = true;

				if(targetExists && targetToSacrificeExists)
					break;
			}

			if(!(targetExists && targetToSacrificeExists))
			{
				MetaString text;
				text.addTxt(MetaString::GENERAL_TXT, 185);
				problem.add(std::move(text), Problem::NORMAL);
				return false;
			}
		}
		break;
	default:
		logGlobal->warn("Invalid mode for Sacrifice effect: spell %s, mode %d", m->getSpellName(), (int)mode); //should not even try to do it
		return m->adaptProblem(ESpellCastProblem::INVALID, problem);
	}

	return true;
}

bool Sacrifice::applicable(Problem & problem, const Mechanics * m, const Target & aimPoint, const EffectTarget & target) const
{
	//TODO: proper support for multiple targets

	if(target.empty())
		return false;

	EffectTarget healTarget;
	healTarget.emplace_back(target.front());

	if(!Heal::applicable(problem, m, aimPoint, healTarget))
		return false;

    if(target.size() == 2)
	{
		auto victim = target.at(1).unitValue;
		if(!victim->alive() || getStackFilter(m ,false, victim))
			return false;
	}

	return true;
}

void Sacrifice::apply(const PacketSender * server, RNG & rng, const Mechanics * m, const EffectTarget & target) const
{
	if(target.size() != 2)
	{
		logGlobal->error("Sacrifice effect requires 2 targets");
		return;
	}

    const battle::Unit * victim = target.back().unitValue;

    if(!victim)
	{
		logGlobal->error("No unit to Sacrifice");
		return;
	}

	EffectTarget healTarget;
	healTarget.emplace_back(target.front());

	int64_t effectValue = (m->getEffectPower() + victim->unitMaxHealth() + m->calculateRawEffectValue(0, 1)) * victim->getCount();

	Heal::apply(effectValue, server, rng, m, healTarget);

	BattleStacksRemoved bsr;
	bsr.stackIDs.insert(victim->unitId());
	server->sendAndApply(&bsr);
}

EffectTarget Sacrifice::filterTarget(const Mechanics * m, const EffectTarget & target) const
{
	return target;
}

bool Sacrifice::isValidTarget(const Mechanics * m, const battle::Unit * unit) const
{
	return unit->isValidTarget(true);
}

EffectTarget Sacrifice::transformTarget(const Mechanics * m, const Target & aimPoint, const Target & spellTarget) const
{
	EffectTarget res = Heal::transformTarget(m, aimPoint, spellTarget);

	//ignore spell range for now, arbitrary range support requires redesign
	res.resize(1);

	//add victim
	if(aimPoint.size() >= 2)
	{
		auto victim = aimPoint.at(1).unitValue;
		if(getStackFilter(m, false, victim))
			res.emplace_back(victim);
	}

	return res;
}


void Sacrifice::serializeJsonHealEffect(JsonSerializeFormat & handler)
{

}

} // namespace effects
} // namespace spells
