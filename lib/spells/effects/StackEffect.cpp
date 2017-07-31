/*
 * StackEffect.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "StackEffect.h"

#include "Effects.h"

#include "../ISpellMechanics.h"

#include "../../NetPacksBase.h"
#include "../../battle/CBattleInfoCallback.h"

namespace spells
{
namespace effects
{

StackEffect::StackEffect()
{

}

void StackEffect::addTo(Effects * where, const int level)
{
	spellLevel = level;
	where->creature.at(level).push_back(this->shared_from_this());
}

bool StackEffect::applicable(Problem & problem, const Mechanics * m) const
{
	//stack effect is applicable in general if there is at least one smart target

	switch(m->mode)
	{
	case Mode::HERO:
	case Mode::CREATURE_ACTIVE:
	case Mode::ENCHANTER:
	case Mode::PASSIVE:
		{
			auto mainFilter = std::bind(&StackEffect::getStackFilter, this, m, true, _1);
			auto predicate = std::bind(&StackEffect::eraseByImmunityFilter, this, m, _1);

			TStacks targets = m->cb->battleGetStacksIf(mainFilter);
			vstd::erase_if(targets, predicate);
			if(targets.empty())
			{
				MetaString text;
				text.addTxt(MetaString::GENERAL_TXT, 185);
				problem.add(std::move(text), Problem::NORMAL);
				return false;
			}
		}
		break;
	}

	return true;
}

bool StackEffect::applicable(Problem & problem, const Mechanics * m, const Target & aimPoint, const EffectTarget & target) const
{
	//stack effect is applicable if it affects at least one smart target
	//assume target correctly transformed, just reapply smart filter

	for(auto & item : target)
		if(item.stackValue)
			if(getStackFilter(m, true, item.stackValue))
				return true;


	return false;
}

bool StackEffect::getStackFilter(const Mechanics * m, bool alwaysSmart, const CStack * s) const
{
	const CSpell::TargetInfo targetInfo(m->owner, spellLevel, m->mode);

	const bool ownerMatches = m->cb->battleMatchOwner(m->caster->getOwner(), s, m->owner->getPositiveness());
	const bool validTarget = s->isValidTarget(!targetInfo.onlyAlive); //todo: this should be handled by spell class
	const bool positivenessFlag = !(targetInfo.smart || alwaysSmart) || ownerMatches;

	return positivenessFlag && validTarget;
}

bool StackEffect::eraseByImmunityFilter(const Mechanics * m, const CStack * s) const
{
	return m->owner->internalIsImmune(m->caster, s);
}

EffectTarget StackEffect::filterTarget(const Mechanics * m, const BattleCast & p, const EffectTarget & target) const
{
	EffectTarget res;
	vstd::copy_if(target, std::back_inserter(res), [this, m](const Destination & d)
	{
		if(!d.stackValue)
			return false;
		if(eraseByImmunityFilter(m, d.stackValue))
			return false;
		return true;
	});
	return res;
}

EffectTarget StackEffect::transformTarget(const Mechanics * m, const Target & aimPoint, const Target & spellTarget) const
{
	auto mainFilter = std::bind(&StackEffect::getStackFilter, this, m, false, _1);

	const CSpell::TargetInfo targetInfo(m->owner, spellLevel, m->mode);

	Target spellTargetCopy(spellTarget);

	//make sure that we have valid target with valid aim, even if spell have invalid range configured
	//TODO: check than spell range is actually not valid
	//also hackfix for banned creature massive spells
	if(!aimPoint.empty())
		spellTargetCopy.insert(spellTargetCopy.begin(), Destination(aimPoint.front()));

	Destination mainDestination = spellTargetCopy.front();

	std::set<const CStack *> targets;

	if(targetInfo.massive)
	{
		//ignore spellTarget and add all stacks
		TStacks stacks = m->cb->battleGetStacksIf(mainFilter);
		for(auto stack : stacks)
			targets.insert(stack);
	}
	else
	{
		//process each tile
		for(const Destination & dest : spellTargetCopy)
		{
			if(dest.stackValue)
			{
				if(mainFilter(dest.stackValue))
					targets.insert(dest.stackValue);
			}
			else if(dest.hexValue.isValid())
			{
				//select one stack on tile, prefer alive one
				const CStack * targetOnTile = nullptr;

				auto predicate = [&](const CStack * s)
				{
					return s->coversPos(dest.hexValue) && mainFilter(s);
				};

				TStacks stacks = m->cb->battleGetStacksIf(predicate);

				for(auto s : stacks)
				{
					if(s->alive())
					{
						targetOnTile = s;
						break;
					}
				}

				if(targetOnTile == nullptr && !stacks.empty())
					targetOnTile = stacks.front();

				if(targetOnTile)
					targets.insert(targetOnTile);
			}
			else
			{
				logGlobal->debug("Invalid destination in spell Target");
			}
		}
	}

	auto predicate = std::bind(&StackEffect::eraseByImmunityFilter, this, m, _1);

	vstd::erase_if(targets, predicate);

	if(m->mode == Mode::SPELL_LIKE_ATTACK)
	{
		if(!aimPoint.empty() && aimPoint.front().stackValue)
			targets.insert(aimPoint.front().stackValue);
		else
			logGlobal->error("Spell-like attack with no primary target.");
	}

	EffectTarget effectTarget;

	for(auto s : targets)
		effectTarget.push_back(Destination(s));

	return effectTarget;
}

} // namespace effects
} // namespace spells
