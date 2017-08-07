/*
 * StackWithBonuses.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "StackWithBonuses.h"
#include "../../lib/CStack.h"

void actualizeEffect(TBonusListPtr target, const Bonus & ef)
{
	for(auto bonus : *target) //TODO: optimize
	{
		if(bonus->source == Bonus::SPELL_EFFECT && bonus->type == ef.type && bonus->subtype == ef.subtype)
		{
			bonus->turnsRemain = std::max(bonus->turnsRemain, ef.turnsRemain);
		}
	}
}

StackWithBonuses::StackWithBonuses(const CStack * Stack)
	: stack(Stack),
	state(this)
{
	state = stack->stackState;
	position = stack->position;
}

const TBonusListPtr StackWithBonuses::getAllBonuses(const CSelector & selector, const CSelector & limit,
	const CBonusSystemNode * root, const std::string & cachingStr) const
{
	TBonusListPtr ret = std::make_shared<BonusList>();
	const TBonusListPtr originalList = stack->getAllBonuses(selector, limit, root, cachingStr);
	range::copy(*originalList, std::back_inserter(*ret));

	for(const Bonus & bonus : bonusesToUpdate)
	{
		if(selector(&bonus) && (!limit || !limit(&bonus)))
		{
			if(ret->getFirst(Selector::source(Bonus::SPELL_EFFECT, bonus.sid).And(Selector::typeSubtype(bonus.type, bonus.subtype))))
			{
				actualizeEffect(ret, bonus);
			}
			else
			{
				auto b = std::make_shared<Bonus>(bonus);
				ret->push_back(b);
			}
		}
	}

	for(auto & bonus : bonusesToAdd)
	{
		auto b = std::make_shared<Bonus>(bonus);
		if(selector(b.get()) && (!limit || !limit(b.get())))
			ret->push_back(b);
	}
	//TODO limiters?
	return ret;
}

int32_t StackWithBonuses::creatureIndex() const
{
	return stack->creatureIndex();
}

int32_t StackWithBonuses::unitMaxHealth() const
{
	return stack->unitMaxHealth();
}

int32_t StackWithBonuses::unitBaseAmount() const
{
	return stack->unitBaseAmount();
}

const IBonusBearer * StackWithBonuses::unitAsBearer() const
{
	return this;
}

bool StackWithBonuses::unitHasAmmoCart() const
{
	//todo: check ammocart alive state here
	return stack->unitHasAmmoCart();
}

bool StackWithBonuses::doubleWide() const
{
	return stack->doubleWide();
}

uint32_t StackWithBonuses::unitId() const
{
	return stack->unitId();
}

ui8 StackWithBonuses::unitSide() const
{
	return stack->unitSide();
}
