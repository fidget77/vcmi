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
#include "Problem.h"

#include "../CStack.h"

namespace spells
{

CustomSpellMechanics::CustomSpellMechanics(const CSpell * s, const CBattleInfoCallback * Cb, const Caster * caster_, std::shared_ptr<effects::Effects> e)
	: DefaultSpellMechanics(s, Cb, caster_), effects(e)
{}

CustomSpellMechanics::~CustomSpellMechanics()
{}

void CustomSpellMechanics::applyEffects(const SpellCastEnvironment * env, const BattleCast & parameters) const
{
	auto callback = [&](const effects::IEffect * e, bool & stop)
	{
		EffectTarget target = e->filterTarget(this, parameters, parameters.target);
		e->apply(env, env->getRandomGenerator(), this, parameters, target);
	};

	effects->forEachEffect(parameters.spellLvl, callback);
}

void CustomSpellMechanics::applyEffectsForced(const SpellCastEnvironment * env, const BattleCast & parameters) const
{
	auto callback = [&](const effects::IEffect * e, bool & stop)
	{
		e->apply(env, env->getRandomGenerator(), this, parameters, parameters.target);
	};

	effects->forEachEffect(parameters.spellLvl, callback);
}

bool CustomSpellMechanics::canBeCast(Problem & problem) const
{
	const int level = caster->getSpellSchoolLevel(mode, owner);
	return effects->applicable(problem, this, level);
}

bool CustomSpellMechanics::canBeCastAt(BattleHex destination) const
{
	const int level = caster->getSpellSchoolLevel(mode, owner);

	detail::ProblemImpl problem;

	//TODO: add support for secondary targets
	//TODO: send problem to caller (for battle log message in BattleInterface)
	Target tmp;
	tmp.push_back(Destination(destination));

	Target spellTarget = transformSpellTarget(tmp, level);

    return effects->applicable(problem, this, level, tmp, spellTarget);
}

std::vector<const CStack *> CustomSpellMechanics::getAffectedStacks(int spellLvl, BattleHex destination) const
{
	Target tmp;
	tmp.push_back(Destination(destination));
	Target spellTarget = transformSpellTarget(tmp, spellLvl);

	EffectTarget all;

	effects->forEachEffect(spellLvl, [&all, &tmp, &spellTarget, this](const effects::IEffect * e, bool & stop)
	{
		EffectTarget one = e->transformTarget(this, tmp, spellTarget);
		vstd::concatenate(all, one);
	});

	std::set<const CStack *> stacks;

	for(const Destination & dest : all)
	{
		if(dest.stackValue)
			stacks.insert(dest.stackValue);
	}

	std::vector<const CStack *> res;
	std::copy(stacks.begin(), stacks.end(), std::back_inserter(res));
	return res;
}

void CustomSpellMechanics::cast(const SpellCastEnvironment * env, const BattleCast & parameters, SpellCastContext & ctx, std::vector<const CStack*> & reflected) const
{
	reflected.clear();

	Target spellTarget = transformSpellTarget(parameters.target, parameters.spellLvl);

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

	auto filterStack = [&](const CStack * s)
	{
		if(stackResisted(s))
			resisted.push_back(s);
		else if(stackReflected(s))
			reflected.push_back(s);
		else
			affected.push_back(s);
	};

	//prepare targets
	auto toApply = effects->prepare(this, parameters, spellTarget);

	//collect stacks from targets of all effects
	std::set<const CStack *> stacks;

	for(const auto & p : toApply)
	{
		for(const Destination & d : p.second)
			if(d.stackValue)
				stacks.insert(d.stackValue);
	}

	//process them
	for(auto s : stacks)
		filterStack(s);

	//and update targets
	for(auto & p : toApply)
	{
		vstd::erase_if(p.second, [&](const Destination & d)
		{
			if(!d.stackValue)
				return false;
			return vstd::contains(resisted, d.stackValue) || vstd::contains(reflected, d.stackValue);
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
		p.first->apply(env, env->getRandomGenerator(), this, parameters, p.second);
}

Target CustomSpellMechanics::transformSpellTarget(const Target & aimPoint, const int spellLevel) const
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
			auto spellRange = rangeInHexes(aimPoint, spellLevel);
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

