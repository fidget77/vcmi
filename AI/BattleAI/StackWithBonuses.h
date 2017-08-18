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
#include "../../lib/battle/BattleProxy.h"

class HypotheticBattle;

class StackWithBonuses : public IBonusBearer, public IUnitBonus
{
public:
	CStackState state;

	mutable std::vector<Bonus> bonusesToAdd;
	mutable std::vector<Bonus> bonusesToUpdate;

	StackWithBonuses(const CStackState * Stack);

	///IBonusBearer
	const TBonusListPtr getAllBonuses(const CSelector & selector, const CSelector & limit,
		const CBonusSystemNode * root = nullptr, const std::string & cachingStr = "") const override;

	const IBonusBearer * unitAsBearer() const override;
	bool unitHasAmmoCart() const override;
};


class HypotheticBattle : public BattleProxy
{
public:
	HypotheticBattle(Subject realBattle);

	std::map<uint32_t, std::shared_ptr<StackWithBonuses>> stackStates;
};
