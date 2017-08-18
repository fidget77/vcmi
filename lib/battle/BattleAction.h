/*
 * BattleAction.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once
#include "BattleHex.h"
#include "../GameConstants.h"

class IStackState;

/// A struct which handles battle actions like defending, walking,... - represents a creature stack in a battle
struct DLL_LINKAGE BattleAction
{
	ui8 side; //who made this action: false - left, true - right player
	ui32 stackNumber; //stack ID, -1 left hero, -2 right hero,
	Battle::ActionType actionType; //use ActionType enum for values
	BattleHex destinationTile;
	si32 additionalInfo; // e.g. spell number if type is 1 || 10; tile to attack if type is 6
	si32 selectedStack; //spell subject for teleport / sacrifice

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & side;
		h & stackNumber;
		h & actionType;
		h & destinationTile;
		h & additionalInfo;
		h & selectedStack;
	}

	BattleAction();

	static BattleAction makeHeal(const IStackState * healer, const IStackState * healed);
	static BattleAction makeDefend(const IStackState * stack);
	static BattleAction makeWait(const IStackState * stack);
	static BattleAction makeMeleeAttack(const IStackState * stack, const IStackState * attacked, BattleHex attackFrom = BattleHex::INVALID);
	static BattleAction makeShotAttack(const IStackState * shooter, const IStackState * target);
	static BattleAction makeMove(const IStackState * stack, BattleHex dest);
	static BattleAction makeEndOFTacticPhase(ui8 side);

	std::string toString() const;
};

DLL_EXPORT std::ostream & operator<<(std::ostream & os, const BattleAction & ba); //todo: remove
