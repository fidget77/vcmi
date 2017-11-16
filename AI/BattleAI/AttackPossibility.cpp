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
	const auto dealtDmgValue = priorities->stackEvaluator(enemy.get()) * damageDealt;
	const auto receivedDmgValue = priorities->stackEvaluator(attack.attacker.get()) * damageReceived;

	int diff = 0;

	//friendly fire or not
	if(attack.attacker->unitSide() == enemy->unitSide())
		diff = -dealtDmgValue - receivedDmgValue;
	else
		diff = dealtDmgValue - receivedDmgValue;

	//mind control
	auto actualSide = getCbc()->playerToSide(getCbc()->battleGetOwner(attack.attacker.get()));
	if(actualSide && actualSide.get() != attack.attacker->unitSide())
		diff = -diff;
	return diff;
}

int AttackPossibility::attackValue() const
{
	return damageDiff() + tacticImpact;
}

AttackPossibility AttackPossibility::evaluate(const BattleAttackInfo & AttackInfo, BattleHex hex)
{
	const int remainingCounterAttacks = AttackInfo.defender->counterAttacks.available();
	const bool counterAttacksBlocked = AttackInfo.attacker->hasBonusOfType(Bonus::BLOCKS_RETALIATION) || AttackInfo.defender->hasBonusOfType(Bonus::NO_RETALIATION);

	const int totalAttacks = 1 + AttackInfo.attacker->getBonuses(Selector::type(Bonus::ADDITIONAL_ATTACK), (Selector::effectRange (Bonus::NO_LIMIT).Or(Selector::effectRange(Bonus::ONLY_MELEE_FIGHT))))->totalValue();

	AttackPossibility ap = {AttackInfo.defender, hex, AttackInfo, 0, 0, 0};

	BattleAttackInfo curBai = AttackInfo; //we'll modify here the stack state

	curBai.attacker.reset(new battle::CUnitState(*AttackInfo.attacker.get()));
	curBai.defender.reset(new battle::CUnitState(*AttackInfo.defender.get()));

	for(int i = 0; i < totalAttacks; i++)
	{
		TDmgRange retaliation(0,0);
		auto attackDmg = getCbc()->battleEstimateDamage(curBai, &retaliation);

		vstd::amin(attackDmg.first, curBai.defender->health.available());
		vstd::amin(attackDmg.second, curBai.defender->health.available());

		vstd::amin(retaliation.first, curBai.attacker->health.available());
		vstd::amin(retaliation.second, curBai.attacker->health.available());

		ap.damageDealt = (attackDmg.first + attackDmg.second) / 2;
		ap.damageReceived = (retaliation.first + retaliation.second) / 2;

		if(remainingCounterAttacks <= i || counterAttacksBlocked)
			ap.damageReceived = 0;

		curBai.attacker->damage(ap.damageReceived);
		curBai.defender->damage(ap.damageDealt);
		if(!curBai.attacker->alive())
			break;
		if(!curBai.defender->alive())
			break;
	}

	ap.attack = curBai;

	//TODO other damage related to attack (eg. fire shield and other abilities)

	return ap;
}


Priorities* AttackPossibility::priorities = nullptr;
