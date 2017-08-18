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
#include "BattleHex.h"
#include "../CStack.h"

class IBonusBearer;

struct DLL_LINKAGE BattleAttackInfo
{
	CStackState attacker;
	CStackState defender;

	bool shooting;
	int chargedFields;

	bool luckyHit;
	bool unluckyHit;
	bool deathBlow;
	bool ballistaDoubleDamage;

	BattleAttackInfo(const CStackState & Attacker, const CStackState & Defender, bool Shooting = false);
	BattleAttackInfo reverse() const;
};
