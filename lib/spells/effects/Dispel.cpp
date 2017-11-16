/*
 * Dispel.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "Dispel.h"
#include "Registry.h"
#include "../ISpellMechanics.h"
#include "../CSpellHandler.h"

#include "../../NetPacks.h"
#include "../../CStack.h"
#include "../../battle/IBattleState.h"
#include "../../serializer/JsonSerializeFormat.h"

static const std::string EFFECT_NAME = "core:dispel";

namespace spells
{
namespace effects
{

VCMI_REGISTER_SPELL_EFFECT(Dispel, EFFECT_NAME);

Dispel::Dispel(const int level)
	: StackEffect(level)
{

}

Dispel::~Dispel() = default;

void Dispel::apply(const PacketSender * server, RNG & rng, const Mechanics * m, const EffectTarget & target) const
{
	SetStackEffect sse;
	prepareEffects(sse, rng, m, target);
	if(!sse.toRemove.empty())
		server->sendAndApply(&sse);
}

void Dispel::apply(IBattleState * battleState, RNG & rng, const Mechanics * m, const EffectTarget & target) const
{
	SetStackEffect sse;
	prepareEffects(sse, rng, m, target);

	for(const auto & idAndBonus : sse.toRemove)
		battleState->removeUnitBonus(idAndBonus.first, idAndBonus.second);
}

bool Dispel::isReceptive(const Mechanics * m, const battle::Unit * unit) const
{
	//ignore all immunities, except specific absolute immunity(VCMI addition)

	//SPELL_IMMUNITY absolute case
	std::stringstream cachingStr;
	cachingStr << "type_" << Bonus::SPELL_IMMUNITY << "subtype_" << m->getSpellIndex() << "addInfo_1";
	if(unit->hasBonus(Selector::typeSubtypeInfo(Bonus::SPELL_IMMUNITY, m->getSpellIndex(), 1), cachingStr.str()))
		return false;

	return true;
}

bool Dispel::isValidTarget(const Mechanics * m, const battle::Unit * unit) const
{
	if(!unit->isValidTarget(false))
		return false;

	if(getBonuses(m, unit)->empty())
		return false;

	return true;
}

void Dispel::serializeJsonEffect(JsonSerializeFormat & handler)
{
	handler.serializeBool("positive", positive);
	handler.serializeBool("negative", negative);
	handler.serializeBool("neutral", neutral);
}

std::shared_ptr<BonusList> Dispel::getBonuses(const Mechanics * m, const battle::Unit * unit) const
{
	auto addSelector = [=](const Bonus * bonus)
	{
		if(bonus->source == Bonus::SPELL_EFFECT)
		{
			const CSpell * sourceSpell = SpellID(bonus->sid).toSpell();
			if(!sourceSpell)
				return false;//error
			if(bonus->sid == m->getSpellIndex())
				return false;

			if(positive && sourceSpell->isPositive())
				return true;
			if(negative && sourceSpell->isNegative())
				return true;
			if(neutral && sourceSpell->isNeutral())
				return true;

		}
		return false;
	};
	CSelector selector = CSelector(mainSelector).And(CSelector(addSelector));

	return unit->getBonuses(selector);
}

bool Dispel::mainSelector(const Bonus * bonus)
{
	if(bonus->source == Bonus::SPELL_EFFECT)
	{
		const CSpell * sourceSpell = SpellID(bonus->sid).toSpell();
		if(!sourceSpell)
			return false;//error
		//Special case: DISRUPTING_RAY is "immune" to dispell
		//Other even PERMANENT effects can be removed (f.e. BIND)
		if(sourceSpell->id == SpellID::DISRUPTING_RAY)
			return false;
		//Special case:do not remove lifetime marker
		if(sourceSpell->id == SpellID::CLONE)
			return false;
		//stack may have inherited effects
		if(sourceSpell->isAdventureSpell())
			return false;
		return true;
	}
	//not spell effect
	return false;
}

void Dispel::prepareEffects(SetStackEffect & pack, RNG & rng, const Mechanics * m, const EffectTarget & target) const
{
	for(auto & t : target)
	{
		const battle::Unit * unit = t.unitValue;
		if(unit)
		{
			std::vector<Bonus> buffer;
			auto bl = getBonuses(m, unit);

			for(auto item : *bl)
				buffer.emplace_back(*item);

			if(!buffer.empty())
				pack.toRemove.push_back(std::make_pair(unit->unitId(), buffer));
		}
	}
}


} // namespace effects
} // namespace spells
