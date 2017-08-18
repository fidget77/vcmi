/*
 * AttackPossibility.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once
#include "../../lib/CStack.h"
#include "../../CCallback.h"
#include "common.h"
#include "StackWithBonuses.h"

class Priorities
{
public:
	std::vector<double> resourceTypeBaseValues;
	std::function<double(const IStackState *)> stackEvaluator;
	Priorities()
	{
		//        range::copy(VLC->objh->resVals, std::back_inserter(resourceTypeBaseValues));
		stackEvaluator = [](const IStackState *){ return 1.0; };
	}
};

class AttackPossibility
{
public:
	CStackState enemy; //redundant (to attack.defender) but looks nice
	BattleHex tile; //tile from which we attack
	BattleAttackInfo attack;

	int damageDealt;
	int damageReceived; //usually by counter-attack
	int tacticImpact;

	int damageDiff() const;
	int attackValue() const;

	static AttackPossibility evaluate(const BattleAttackInfo &AttackInfo, BattleHex hex);
	static Priorities * priorities;
};
