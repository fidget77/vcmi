/*
 * TargetCondition.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "TargetCondition.h"

#include "ISpellMechanics.h"

#include "../GameConstants.h"
#include "../CBonusTypeHandler.h"
#include "../CStack.h"

#include "../serializer/JsonSerializeFormat.h"


namespace spells
{

class BonusCondition : public TargetCondition::Item
{
public:
	BonusTypeID type;

	BonusCondition();

protected:

	bool check(const CSpell * spell, const IStackState * target) const override;
};

class CreatureCondition : public TargetCondition::Item
{
public:
	CreatureID type;

	CreatureCondition();

	bool check(const CSpell * spell, const IStackState * target) const override;
};


}

namespace spells
{

TargetCondition::Item::Item()
{

}

TargetCondition::Item::~Item() = default;

bool TargetCondition::Item::isReceptive(const CSpell * spell, const IStackState * target) const
{
	bool result = check(spell, target);

	if(inverted)
		return !result;
	else
		return result;
}

TargetCondition::TargetCondition()
{

}

TargetCondition::~TargetCondition()
{

}

bool TargetCondition::isReceptive(const CSpell * spell, const IStackState * target) const
{
	//TODO
	return true;
}

void TargetCondition::serializeJson(JsonSerializeFormat & handler)
{
	//TODO
}

bool TargetCondition::check(const ItemVector & condition, const CSpell * spell, const IStackState * target) const
{
    bool nonExclusiveCheck = true;
    bool nonExclusiveExits = false;

    for(auto & item : condition)
	{
        if(item->exclusive)
		{
			if(!item->isReceptive(spell, target))
				return false;
		}
		else
		{
			if(!item->isReceptive(spell, target))
				nonExclusiveCheck = false;
			nonExclusiveExits = true;
		}
	}

	if(nonExclusiveExits)
		return nonExclusiveCheck;
	else
		return true;
}

///BonusCondition
BonusCondition::BonusCondition()
{

}

bool BonusCondition::check(const CSpell * spell, const IStackState * target) const
{
	const IBonusBearer * bearer = target->unitAsBearer();

	return bearer->hasBonus(Selector::type(type));
}

///CreatureCondition
CreatureCondition::CreatureCondition()
{

}

bool CreatureCondition::check(const CSpell * spell, const IStackState * target) const
{
	return target->creatureId() == type;
}


} // namespace spells
