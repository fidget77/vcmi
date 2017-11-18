/*
 * IBattleState.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once
#include "CBattleInfoEssentials.h"

class CStackStateInfo;
struct Bonus;

class DLL_LINKAGE IBattleInfo
{
public:
	using ObstacleCList = std::vector<std::shared_ptr<const CObstacleInstance>>;

	virtual ~IBattleInfo() = default;

	virtual int32_t getActiveStackID() const = 0;

	virtual TStacks getStacksIf(TStackFilter predicate) const = 0;

	virtual battle::Units getUnitsIf(battle::UnitFilter predicate) const = 0;

	virtual BFieldType getBattlefieldType() const = 0;
	virtual ETerrainType getTerrainType() const = 0;

	virtual ObstacleCList getAllObstacles() const = 0;

	virtual const CGTownInstance * getDefendedTown() const = 0;
	virtual si8 getWallState(int partOfWall) const = 0;
	virtual EGateState getGateState() const = 0;

	virtual PlayerColor getSidePlayer(ui8 side) const = 0;
	virtual const CArmedInstance * getSideArmy(ui8 side) const = 0;
	virtual const CGHeroInstance * getSideHero(ui8 side) const = 0;

	virtual uint32_t getCastSpells(ui8 side) const = 0;
	virtual int32_t getEnchanterCounter(ui8 side) const = 0;

	virtual ui8 getTacticDist() const = 0;
	virtual ui8 getTacticsSide() const = 0;

	virtual const IBonusBearer * asBearer() const = 0;

	virtual uint32_t nextUnitId() const = 0;
};


class DLL_LINKAGE IBattleState : public IBattleInfo
{
public:
	//TODO: add non-const API and use in game state

	virtual void updateUnit(const CStackStateInfo & changes) = 0;

	virtual void addUnitBonus(uint32_t id, const std::vector<Bonus> & bonus) = 0;
	virtual void updateUnitBonus(uint32_t id, const std::vector<Bonus> & bonus) = 0;
	virtual void removeUnitBonus(uint32_t id, const std::vector<Bonus> & bonus) = 0;
};
