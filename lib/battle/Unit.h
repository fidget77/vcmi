/*
 * {file}.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "../HeroBonus.h"

#include "IUnitInfo.h"
#include "BattleHex.h"

class MetaString;

namespace battle
{
class CUnitState;

class DLL_LINKAGE Unit : public IUnitInfo, public virtual IBonusBearer
{
public:
	virtual bool ableToRetaliate() const = 0;
	virtual bool alive() const = 0;
	virtual bool isGhost() const = 0;

	bool isDead() const;
	bool isTurret() const;
	bool isValidTarget(bool allowDead = false) const; //non-turret non-ghost stacks (can be attacked or be object of magic effect)

	virtual bool isClone() const = 0;
	virtual bool hasClone() const = 0;

	virtual bool canCast() const = 0;
	virtual bool isCaster() const = 0;
	virtual bool canShoot() const = 0;
	virtual bool isShooter() const = 0;

	virtual int32_t getCount() const = 0;
	virtual int32_t getFirstHPleft() const = 0;
	virtual int32_t getKilled() const = 0;
	virtual int64_t getAvailableHealth() const = 0;
	virtual int64_t getTotalHealth() const = 0;

	virtual BattleHex getPosition() const = 0;

	virtual int32_t getInitiative(int turn = 0) const = 0;

	virtual bool canMove(int turn = 0) const = 0; //if stack can move
	virtual bool defended(int turn = 0) const = 0;
	virtual bool moved(int turn = 0) const = 0; //if stack was already moved this turn
	virtual bool willMove(int turn = 0) const = 0; //if stack has remaining move this turn
	virtual bool waited(int turn = 0) const = 0;

	virtual std::shared_ptr<CUnitState> asquire() const = 0;

	virtual int battleQueuePhase(int turn) const = 0;

	virtual std::string getDescription() const;
	std::vector<BattleHex> getSurroundingHexes(BattleHex assumedPosition = BattleHex::INVALID) const; // get six or 8 surrounding hexes depending on creature size

	bool coversPos(BattleHex position) const; //checks also if unit is double-wide

	static std::vector<BattleHex> getHexes(BattleHex assumedPos, bool twoHex, ui8 side);

	///MetaStrings
	void addText(MetaString & text, ui8 type, int32_t serial, const boost::logic::tribool & plural = boost::logic::indeterminate) const;
	void addNameReplacement(MetaString & text, const boost::logic::tribool & plural = boost::logic::indeterminate) const;
	std::string formatGeneralMessage(const int32_t baseTextId) const;
};

} // namespace battle
