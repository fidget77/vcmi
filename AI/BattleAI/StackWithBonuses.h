/*
 * StackWithBonuses.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once
#include "../../lib/HeroBonus.h"
#include "../../lib/CStack.h"


class StackWithBonuses : public IBonusBearer, public IUnitInfo
{
public:
	const CStack * stack;

	BattleHex position; //TODO: move to state

	CStackState state;

	mutable std::vector<Bonus> bonusesToAdd;
	mutable std::vector<Bonus> bonusesToUpdate;

	StackWithBonuses(const CStack * Stack);

	///IBonusBearer
	const TBonusListPtr getAllBonuses(const CSelector &selector, const CSelector &limit,
		const CBonusSystemNode *root = nullptr, const std::string &cachingStr = "") const override;

	///IUnitInfo
	virtual int32_t creatureIndex() const override;
	int32_t unitMaxHealth() const override;
	int32_t unitBaseAmount() const override;
	const IBonusBearer * unitAsBearer() const override;
	bool unitHasAmmoCart() const override;
	bool doubleWide() const override;
	uint32_t unitId() const override;
	ui8 unitSide() const override;
};
