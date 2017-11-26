/*
 * BattleAttackInfo.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

namespace battle
{
	class Unit;
	class CUnitState;
}

struct DLL_LINKAGE BattleAttackInfo
{
	std::shared_ptr<battle::CUnitState> attacker;
	std::shared_ptr<battle::CUnitState> defender;

	bool shooting;
	int chargedFields;

	bool luckyHit;
	bool unluckyHit;
	bool deathBlow;
	bool ballistaDoubleDamage;

	BattleAttackInfo(const battle::Unit * Attacker, const battle::Unit * Defender, bool Shooting = false);
	BattleAttackInfo reverse() const;
};
