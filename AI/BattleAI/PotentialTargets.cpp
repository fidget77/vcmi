/*
 * PotentialTargets.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "PotentialTargets.h"

PotentialTargets::PotentialTargets(const battle::Unit * attacker, const HypotheticBattle * state)
{
	auto attIter = state->stackStates.find(attacker->unitId());
	const battle::Unit * attackerInfo = (attIter == state->stackStates.end()) ? attacker : (battle::Unit *)&attIter->second->state;

	auto dists = state->battleGetDistances(attackerInfo, attackerInfo->getPosition());
	auto avHexes = state->battleGetAvailableHexes(attackerInfo, attackerInfo->getPosition());

	//FIXME: this should part of battleGetAvailableHexes
	bool forceTarget = false;
	const battle::Unit * forcedTarget = nullptr;
	BattleHex forcedHex;

	if(attackerInfo->hasBonusOfType(Bonus::ATTACKS_NEAREST_CREATURE))
	{
		forceTarget = true;
		auto nearest = state->getNearestStack(attackerInfo);

		if(nearest.first != nullptr)
		{
			forcedTarget = nearest.first;
			forcedHex = nearest.second;
		}
	}

	auto aliveUnits = state->battleGetUnitsIf([=](const battle::Unit * unit)
	{
		return unit->isValidTarget() && unit->unitId() != attackerInfo->unitId();
	});

	for(auto defender : aliveUnits)
	{
		if(!forceTarget && !state->battleMatchOwner(attackerInfo, defender))
			continue;

		auto GenerateAttackInfo = [&](bool shooting, BattleHex hex) -> AttackPossibility
		{
			auto bai = BattleAttackInfo(attackerInfo, defender, shooting);

			if(hex.isValid() && !shooting)
				bai.chargedFields = dists[hex];

			return AttackPossibility::evaluate(bai, hex);
		};

		if(forceTarget)
		{
			if(forcedTarget && defender->unitId() == forcedTarget->unitId())
				possibleAttacks.push_back(GenerateAttackInfo(false, forcedHex));
			else
				unreachableEnemies.push_back(defender);
		}
		else if(state->battleCanShoot(attackerInfo, defender->getPosition()))
		{
			possibleAttacks.push_back(GenerateAttackInfo(true, BattleHex::INVALID));
		}
		else
		{
			for(BattleHex hex : avHexes)
				if(CStack::isMeleeAttackPossible(attackerInfo, defender, hex))
					possibleAttacks.push_back(GenerateAttackInfo(false, hex));

			if(!vstd::contains_if(possibleAttacks, [=](const AttackPossibility & pa) { return pa.enemy->unitId() == defender->unitId(); }))
				unreachableEnemies.push_back(defender);
		}
	}
}

int PotentialTargets::bestActionValue() const
{
	if(possibleAttacks.empty())
		return 0;

	return bestAction().attackValue();
}

AttackPossibility PotentialTargets::bestAction() const
{
	if(possibleAttacks.empty())
		throw std::runtime_error("No best action, since we don't have any actions");

	return *vstd::maxElementByFun(possibleAttacks, [](const AttackPossibility &ap) { return ap.attackValue(); } );
}
