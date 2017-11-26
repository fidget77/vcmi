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

AttackPossibility::AttackPossibility(std::shared_ptr<battle::CUnitState> enemy_, BattleHex tile_, const BattleAttackInfo & attack_)
	: enemy(enemy_),
	tile(tile_),
	attack(attack_)
{
}


int64_t AttackPossibility::damageDiff() const
{
	//TODO: use target priority from HypotheticBattle
	const auto dealtDmgValue = damageDealt;
	const auto receivedDmgValue = damageReceived;

	int64_t diff = 0;

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

int64_t AttackPossibility::attackValue() const
{
	return damageDiff() + tacticImpact;
}

AttackPossibility AttackPossibility::evaluate(const BattleAttackInfo & AttackInfo, BattleHex hex)
{
	const int remainingCounterAttacks = AttackInfo.defender->counterAttacks.available();
	const bool counterAttacksBlocked = AttackInfo.attacker->hasBonusOfType(Bonus::BLOCKS_RETALIATION);

	const int totalAttacks = AttackInfo.shooting ? AttackInfo.attacker->totalAttacks.getRangedValue() : AttackInfo.attacker->totalAttacks.getMeleeValue();

	AttackPossibility ap(AttackInfo.defender, hex, AttackInfo);

	BattleAttackInfo curBai = AttackInfo; //we'll modify here the stack state

	curBai.attacker = AttackInfo.attacker->asquire();
	curBai.defender = AttackInfo.defender->asquire();

	for(int i = 0; i < totalAttacks; i++)
	{
		TDmgRange retaliation(0,0);
		auto attackDmg = getCbc()->battleEstimateDamage(curBai, &retaliation);

		vstd::amin(attackDmg.first, curBai.defender->health.available());
		vstd::amin(attackDmg.second, curBai.defender->health.available());

		vstd::amin(retaliation.first, curBai.attacker->health.available());
		vstd::amin(retaliation.second, curBai.attacker->health.available());

		ap.damageDealt += (attackDmg.first + attackDmg.second) / 2;

		if(remainingCounterAttacks > i && !counterAttacksBlocked)
			ap.damageReceived += (retaliation.first + retaliation.second) / 2;

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
