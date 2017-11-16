/*
 * CreatureSpellMechanics.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"

#include "CreatureSpellMechanics.h"

#include "../NetPacks.h"
#include "../CStack.h"
#include "../battle/BattleInfo.h"

namespace spells
{

AcidBreathDamageMechanics::AcidBreathDamageMechanics(const IBattleCast * event):
	RegularSpellMechanics(event)
{
}

void AcidBreathDamageMechanics::applyBattleEffects(const SpellCastEnvironment * env, const BattleCast & parameters, SpellCastContext & ctx) const
{
	//todo: this should be effectValue
	ctx.setDamageToDisplay(getEffectPower());

	for(auto & attackedCre : ctx.attackedCres)
	{
		BattleStackAttacked bsa;
		bsa.flags |= BattleStackAttacked::SPELL_EFFECT;
		bsa.spellID = getSpellId();
		bsa.damageAmount = getEffectPower(); //damage times the number of attackers
		bsa.stackAttacked = (attackedCre)->ID;
		bsa.attackerID = -1;
		(attackedCre)->prepareAttacked(bsa, env->getRandomGenerator());
		ctx.si.stacks.push_back(bsa);
	}
}

bool AcidBreathDamageMechanics::isImmuneByStack(const battle::Unit * obj) const
{
	//just in case
	if(!obj->alive())
		return true;

	//FIXME: code duplication with Dispell
	//there should be no immunities by design
	//but make it a bit configurable
	//ignore all immunities, except specific absolute immunity
	{
		//SPELL_IMMUNITY absolute case
		std::stringstream cachingStr;
		cachingStr << "type_" << Bonus::SPELL_IMMUNITY << "subtype_" << getSpellIndex() << "addInfo_1";
		if(obj->hasBonus(Selector::typeSubtypeInfo(Bonus::SPELL_IMMUNITY, getSpellIndex(), 1), cachingStr.str()))
			return true;
	}
	return false;
}

///DeathStareMechanics
DeathStareMechanics::DeathStareMechanics(const IBattleCast * event)
	: RegularSpellMechanics(event)
{
}

void DeathStareMechanics::applyBattleEffects(const SpellCastEnvironment * env, const BattleCast & parameters, SpellCastContext & ctx) const
{
	//calculating dmg to display
	si32 damageToDisplay = getEffectPower();

	if(!ctx.attackedCres.empty())
		vstd::amin(damageToDisplay, (*ctx.attackedCres.begin())->getCount()); //stack is already reduced after attack

	ctx.setDamageToDisplay(damageToDisplay);

	for(auto & attackedCre : ctx.attackedCres)
	{
		BattleStackAttacked bsa;
		bsa.flags |= BattleStackAttacked::SPELL_EFFECT;
		bsa.spellID = getSpellId();
		bsa.damageAmount = getEffectPower() * attackedCre->MaxHealth();//todo: move here all DeathStare calculation
		bsa.stackAttacked = attackedCre->ID;
		bsa.attackerID = -1;
		attackedCre->prepareAttacked(bsa, env->getRandomGenerator());
		ctx.si.stacks.push_back(bsa);
	}

	if(damageToDisplay > 0 && !ctx.attackedCres.empty())
	{
		auto attackedStack = ctx.attackedCres.front();
		MetaString line;
		if(damageToDisplay > 1)
		{
			line.addTxt(MetaString::GENERAL_TXT, 119); //%d %s die under the terrible gaze of the %s.
			line.addReplacement(damageToDisplay);
			attackedStack->addNameReplacement(line, true);
		}
		else
		{
			line.addTxt(MetaString::GENERAL_TXT, 118); //One %s dies under the terrible gaze of the %s.
			attackedStack->addNameReplacement(line, false);
		}
		caster->getCasterName(line);
		ctx.si.battleLog.push_back(line);
	}
}

///DispellHelpfulMechanics
DispellHelpfulMechanics::DispellHelpfulMechanics(const IBattleCast * event)
	: RegularSpellMechanics(event)
{
}

void DispellHelpfulMechanics::applyBattleEffects(const SpellCastEnvironment * env, const BattleCast & parameters, SpellCastContext & ctx) const
{
	RegularSpellMechanics::applyBattleEffects(env, parameters, ctx);
	doDispell(env, ctx, positiveSpellEffects);
}

bool DispellHelpfulMechanics::isImmuneByStack(const battle::Unit * obj) const
{
	if(!canDispell(obj, positiveSpellEffects, "DispellHelpfulMechanics::positiveSpellEffects"))
		return true;

	//use default algorithm only if there is no mechanics-related problem
	return RegularSpellMechanics::isImmuneByStack(obj);
}

bool DispellHelpfulMechanics::positiveSpellEffects(const Bonus * b)
{
	if(b->source == Bonus::SPELL_EFFECT)
	{
		const CSpell * sp = SpellID(b->sid).toSpell();
		return sp && sp->isPositive();
	}
	return false; //not a spell effect
}

}//namespace spells
