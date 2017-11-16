/*
 * CustomSpellMechanics.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CustomSpellMechanics.h"
#include "CDefaultSpellMechanics.h"
#include "../battle/IBattleState.h"
#include "../battle/CBattleInfoCallback.h"
#include "Problem.h"

#include "../CStack.h"

namespace spells
{

CustomSpellMechanics::CustomSpellMechanics(const IBattleCast * event, std::shared_ptr<effects::Effects> e)
	: DefaultSpellMechanics(event),
	effects(e)
{}

CustomSpellMechanics::~CustomSpellMechanics() = default;

void CustomSpellMechanics::applyEffects(const SpellCastEnvironment * env, const BattleCast & parameters) const
{
	auto callback = [&](const effects::Effect * e, bool & stop)
	{
		EffectTarget target = e->filterTarget(this, parameters.target);
		e->apply(env, env->getRandomGenerator(), this, target);
	};

	effects->forEachEffect(getEffectLevel(), callback);
}

void CustomSpellMechanics::applyEffectsForced(const SpellCastEnvironment * env, const BattleCast & parameters) const
{
	auto callback = [&](const effects::Effect * e, bool & stop)
	{
		e->apply(env, env->getRandomGenerator(), this, parameters.target);
	};

	effects->forEachEffect(getEffectLevel(), callback);
}

bool CustomSpellMechanics::canBeCast(Problem & problem) const
{
	return effects->applicable(problem, this);
}

bool CustomSpellMechanics::canBeCastAt(BattleHex destination) const
{
	detail::ProblemImpl problem;

	//TODO: add support for secondary targets
	//TODO: send problem to caller (for battle log message in BattleInterface)
	Target tmp;
	tmp.push_back(Destination(destination));

	Target spellTarget = transformSpellTarget(tmp);

    return effects->applicable(problem, this, tmp, spellTarget);
}

std::vector<const CStack *> CustomSpellMechanics::getAffectedStacks(BattleHex destination) const
{
	Target tmp;
	tmp.push_back(Destination(destination));
	Target spellTarget = transformSpellTarget(tmp);

	EffectTarget all;

	effects->forEachEffect(getEffectLevel(), [&all, &tmp, &spellTarget, this](const effects::Effect * e, bool & stop)
	{
		EffectTarget one = e->transformTarget(this, tmp, spellTarget);
		vstd::concatenate(all, one);
	});

	std::set<const CStack *> stacks;

	for(const Destination & dest : all)
	{
		if(dest.unitValue)
		{
			//FIXME: remove and return battle::Unit
			stacks.insert(cb->battleGetStackByID(dest.unitValue->unitId(), false));
		}
	}

	std::vector<const CStack *> res;
	std::copy(stacks.begin(), stacks.end(), std::back_inserter(res));
	return res;
}

void CustomSpellMechanics::cast(const SpellCastEnvironment * env, const BattleCast & parameters, SpellCastContext & ctx, std::vector<const CStack*> & reflected) const
{
	reflected.clear();

	Target spellTarget = transformSpellTarget(parameters.target);

	std::vector <const CStack *> affected;
	std::vector <const CStack *> resisted;

	auto stackReflected = [&, this](const CStack * s) -> bool
	{
		const bool tryMagicMirror = mode != Mode::MAGIC_MIRROR && owner->isNegative() && owner->level && owner->getLevelInfo(0).range == "0";
		if(tryMagicMirror)
		{
			const int mirrorChance = s->valOfBonuses(Bonus::MAGIC_MIRROR);
			if(env->getRandomGenerator().nextInt(99) < mirrorChance)
				return true;
		}
		return false;
	};

	auto stackResisted = [&, this](const CStack * s) -> bool
	{
		if(owner->isNegative())
		{
			//magic resistance
			const int prob = std::min((s)->magicResistance(), 100); //probability of resistance in %
			if(env->getRandomGenerator().nextInt(99) < prob)
				return true;
		}
		return false;
	};

	auto filterStack = [&](const battle::Unit * st)
	{
		const CStack * s = dynamic_cast<const CStack *>(st);

		if(!s)
			s = cb->battleGetStackByID(st->unitId(), false);

		if(stackResisted(s))
			resisted.push_back(s);
		else if(stackReflected(s))
			reflected.push_back(s);
		else
			affected.push_back(s);
	};

	//prepare targets
	auto toApply = effects->prepare(this, parameters.target, spellTarget);

	std::set<const battle::Unit *> stacks = collectTargets(toApply);

	//process them
	for(auto s : stacks)
		filterStack(s);

	//and update targets
	for(auto & p : toApply)
	{
		vstd::erase_if(p.second, [&](const Destination & d)
		{
			if(!d.unitValue)
				return false;
			return vstd::contains(resisted, d.unitValue) || vstd::contains(reflected, d.unitValue);
		});
	}

	for(auto s : reflected)
		ctx.addCustomEffect(s, 3);

	for(auto s : resisted)
		ctx.addCustomEffect(s, 78);

	ctx.attackedCres = affected;

	//TODO: handle special cases
	MetaString line;
	caster->getCastDescription(owner, affected, line);
	ctx.addBattleLog(std::move(line));

	//now we actually cast a spell
	ctx.cast();

	//and see what it does
	for(auto & p : toApply)
		p.first->apply(env, env->getRandomGenerator(), this, p.second);
}

void CustomSpellMechanics::cast(IBattleState * battleState, vstd::RNG & rng, const BattleCast & parameters) const
{
	//TODO: evaluate caster updates (mana usage etc.)
	//TODO: evaluate random values

	Target spellTarget = transformSpellTarget(parameters.target);

	auto toApply = effects->prepare(this, parameters.target, spellTarget);

	std::set<const battle::Unit *> stacks = collectTargets(toApply);

	for(const battle::Unit * one : stacks)
	{
		auto selector = std::bind(&Mechanics::counteringSelector, this, _1);

		std::vector<Bonus> buffer;
		auto bl = one->getBonuses(selector);

		for(auto item : *bl)
			buffer.emplace_back(*item);

		if(!buffer.empty())
			battleState->removeUnitBonus(one->unitId(), buffer);
	}

	for(auto & p : toApply)
		p.first->apply(battleState, rng, this, p.second);
}

std::set<const battle::Unit *> CustomSpellMechanics::collectTargets(const effects::Effects::EffectsToApply & from) const
{
	std::set<const battle::Unit *> result;

	for(const auto & p : from)
	{
		for(const Destination & d : p.second)
			if(d.unitValue)
				result.insert(d.unitValue);
	}

	return result;
}


Target CustomSpellMechanics::transformSpellTarget(const Target & aimPoint) const
{
	Target spellTarget;

	if(aimPoint.size() < 1)
	{
		logGlobal->error("Aimed spell cast with no destination.");
	}
	else
	{
		const Destination & primary = aimPoint.at(0);
		BattleHex aimPoint = primary.hexValue;

		//transform primary spell target with spell range (if it`s valid), leave anything else to effects

		if(aimPoint.isValid())
		{
			auto spellRange = rangeInHexes(aimPoint);
			for(auto & hex : spellRange)
				spellTarget.push_back(Destination(hex));
		}
	}

	if(spellTarget.empty())
		spellTarget.push_back(Destination(BattleHex::INVALID));

	return std::move(spellTarget);
}

bool CustomSpellMechanics::requiresCreatureTarget() const
{
	//TODO: remove
	//effects will do target existence check
	return false;
}


} //namespace spells

