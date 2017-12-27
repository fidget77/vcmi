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
#include "../../lib/battle/BattleProxy.h"
#include "../../lib/battle/CUnitState.h"

class HypotheticBattle;

class StackWithBonuses : public virtual IBonusBearer, public battle::IUnitEnvironment
{
public:
	battle::CUnitState state;

	std::vector<Bonus> bonusesToAdd;
	std::vector<Bonus> bonusesToUpdate;
	std::set<std::shared_ptr<Bonus>> bonusesToRemove;

	StackWithBonuses(const HypotheticBattle * Owner, const battle::CUnitState * Stack);
	virtual ~StackWithBonuses();

	///IBonusBearer
	const TBonusListPtr getAllBonuses(const CSelector & selector, const CSelector & limit,
		const CBonusSystemNode * root = nullptr, const std::string & cachingStr = "") const override;

	int64_t getTreeVersion() const override;

	bool unitHasAmmoCart() const override;

	void addUnitBonus(const std::vector<Bonus> & bonus);
	void updateUnitBonus(const std::vector<Bonus> & bonus);
	void removeUnitBonus(const std::vector<Bonus> & bonus);

	void removeUnitBonus(const CSelector & selector);

private:
	const IBonusBearer * origBearer;
	const HypotheticBattle * owner;
};

class HypotheticBattle : public BattleProxy
{
public:
	std::map<uint32_t, std::shared_ptr<StackWithBonuses>> stackStates;

	HypotheticBattle(Subject realBattle);

	std::shared_ptr<StackWithBonuses> getForUpdate(uint32_t id);

	battle::Units getUnitsIf(battle::UnitFilter predicate) const override;

	void nextRound(int32_t roundNr) override;
	void nextTurn(uint32_t unitId) override;

	void updateUnit(const CStackStateInfo & changes) override;

	void addUnitBonus(uint32_t id, const std::vector<Bonus> & bonus) override;
	void updateUnitBonus(uint32_t id, const std::vector<Bonus> & bonus) override;
	void removeUnitBonus(uint32_t id, const std::vector<Bonus> & bonus) override;

	uint32_t nextUnitId() const override;

	int64_t getActualDamage(const TDmgRange & damage, int32_t attackerCount, vstd::RNG & rng) const override;

	int64_t getTreeVersion() const;

private:
	int32_t bonusTreeVersion;
};
