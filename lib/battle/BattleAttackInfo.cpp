/*
 * BattleAttackInfo.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "BattleAttackInfo.h"


BattleAttackInfo::BattleAttackInfo(const CStackState & Attacker, const CStackState & Defender, bool Shooting)
	: attacker(Attacker),
	defender(Defender)
{
	shooting = Shooting;
	chargedFields = 0;

	luckyHit = false;
	unluckyHit = false;
	deathBlow = false;
	ballistaDoubleDamage = false;
}

BattleAttackInfo BattleAttackInfo::reverse() const
{
	BattleAttackInfo ret = *this;

	ret.attacker.swap(ret.defender);

	ret.shooting = false;
	ret.chargedFields = 0;
	ret.luckyHit = ret.ballistaDoubleDamage = ret.deathBlow = false;

	return ret;
}
