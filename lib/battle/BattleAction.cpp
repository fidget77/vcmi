/*
 * BattleAction.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "BattleAction.h"
#include "../CStack.h"

using namespace Battle;

BattleAction::BattleAction():
	side(-1),
	stackNumber(-1),
	actionType(INVALID),
	destinationTile(-1),
	additionalInfo(-1),
	selectedStack(-1)
{
}

BattleAction BattleAction::makeHeal(const IStackState * healer, const IStackState * healed)
{
	BattleAction ba;
	ba.side = healer->unitSide();
	ba.actionType = STACK_HEAL;
	ba.stackNumber = healer->unitId();
	ba.destinationTile = healed->getPosition();
	return ba;
}

BattleAction BattleAction::makeDefend(const IStackState * stack)
{
	BattleAction ba;
	ba.side = stack->unitSide();
	ba.actionType = DEFEND;
	ba.stackNumber = stack->unitId();
	return ba;
}

BattleAction BattleAction::makeMeleeAttack(const IStackState * stack, const IStackState * attacked, BattleHex attackFrom)
{
	BattleAction ba;
	ba.side = stack->unitSide(); //FIXME: will it fail if stack mind controlled?
	ba.actionType = WALK_AND_ATTACK;
	ba.stackNumber = stack->unitId();
	ba.destinationTile = attackFrom;
	ba.additionalInfo = attacked->getPosition();
	return ba;
}

BattleAction BattleAction::makeWait(const IStackState * stack)
{
	BattleAction ba;
	ba.side = stack->unitSide();
	ba.actionType = WAIT;
	ba.stackNumber = stack->unitId();
	return ba;
}

BattleAction BattleAction::makeShotAttack(const IStackState * shooter, const IStackState * target)
{
	BattleAction ba;
	ba.side = shooter->unitSide();
	ba.actionType = SHOOT;
	ba.stackNumber = shooter->unitId();
	ba.destinationTile = target->getPosition();
	return ba;
}

BattleAction BattleAction::makeMove(const IStackState * stack, BattleHex dest)
{
	BattleAction ba;
	ba.side = stack->unitSide();
	ba.actionType = WALK;
	ba.stackNumber = stack->unitId();
	ba.destinationTile = dest;
	return ba;
}

BattleAction BattleAction::makeEndOFTacticPhase(ui8 side)
{
	BattleAction ba;
	ba.side = side;
	ba.actionType = END_TACTIC_PHASE;
	return ba;
}

std::string BattleAction::toString() const
{
	std::stringstream actionTypeStream;
	actionTypeStream << actionType;

	boost::format fmt("{BattleAction: side '%d', stackNumber '%d', actionType '%s', destinationTile '%s', additionalInfo '%d', selectedStack '%d'}");
	fmt % static_cast<int>(side) % stackNumber % actionTypeStream.str() % destinationTile % additionalInfo % selectedStack;
	return fmt.str();
}

std::ostream & operator<<(std::ostream & os, const BattleAction & ba)
{
	os << ba.toString();
	return os;
}
