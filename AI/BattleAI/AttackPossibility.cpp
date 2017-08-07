/*
 * AttackPossibility.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "AttackPossibility.h"

int AttackPossibility::damageDiff() const
{
	if (!priorities)
		priorities = new Priorities();
	const auto dealtDmgValue = priorities->stackEvaluator(enemy) * damageDealt;
	const auto receivedDmgValue = priorities->stackEvaluator(attack.attacker) * damageReceived;

	int diff = 0;

	//friendly fire or not
	if(attack.attacker->unitSide() == enemy->unitSide())
		diff = -dealtDmgValue - receivedDmgValue;
	else
		diff = dealtDmgValue - receivedDmgValue;

	//mind control
	if(getCbc()->battleGetOwner(attack.attackerState.getUnitInfo()) != attack.attacker->owner)
		diff = -diff;
	return diff;
}

int AttackPossibility::attackValue() const
{
	return damageDiff() + tacticImpact;
}

AttackPossibility AttackPossibility::evaluate(const BattleAttackInfo & AttackInfo, BattleHex hex)
{
	auto attacker = AttackInfo.attacker;
	auto defender = AttackInfo.defender;

	const int remainingCounterAttacks = AttackInfo.defenderState.counterAttacks.available();
	const bool counterAttacksBlocked = AttackInfo.attackerBonuses->hasBonusOfType(Bonus::BLOCKS_RETALIATION) || AttackInfo.defenderBonuses->hasBonusOfType(Bonus::NO_RETALIATION);

	const int totalAttacks = 1 + AttackInfo.attackerBonuses->getBonuses(Selector::type(Bonus::ADDITIONAL_ATTACK), (Selector::effectRange (Bonus::NO_LIMIT).Or(Selector::effectRange(Bonus::ONLY_MELEE_FIGHT))))->totalValue();

	AttackPossibility ap = {defender, hex, AttackInfo, 0, 0, 0};

	BattleAttackInfo curBai = AttackInfo; //we'll modify here the stack state
	for(int i = 0; i < totalAttacks; i++)
	{
		std::pair<ui32, ui32> retaliation(0,0);
		auto attackDmg = getCbc()->battleEstimateDamage(CRandomGenerator::getDefault(), curBai, &retaliation);

		vstd::amin(attackDmg.first, curBai.defenderState.health.available());
		vstd::amin(attackDmg.second, curBai.defenderState.health.available());

		vstd::amin(retaliation.first, curBai.attackerState.health.available());
		vstd::amin(retaliation.second, curBai.attackerState.health.available());

		ap.damageDealt = (attackDmg.first + attackDmg.second) / 2;
		ap.damageReceived = (retaliation.first + retaliation.second) / 2;

		if(remainingCounterAttacks <= i || counterAttacksBlocked)
			ap.damageReceived = 0;

		curBai.attackerState.health = attacker->healthAfterAttacked(ap.damageReceived, curBai.attackerState.health);
		curBai.defenderState.health = defender->healthAfterAttacked(ap.damageDealt, curBai.defenderState.health);
		if(!curBai.attackerState.alive())
			break;
		if(!curBai.defenderState.alive())
			break;
	}

	ap.attack = curBai;

	//TODO other damage related to attack (eg. fire shield and other abilities)

	return ap;
}


Priorities* AttackPossibility::priorities = nullptr;
