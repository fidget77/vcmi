/*
 * CObstacleInstance.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CObstacleInstance.h"
#include "../CHeroHandler.h"
#include "../CTownHandler.h"
#include "../VCMI_Lib.h"

CObstacleInstance::CObstacleInstance()
{
	obstacleType = USUAL;
	uniqueID = -1;
	ID = -1;
}

CObstacleInstance::~CObstacleInstance()
{

}

const CObstacleInfo & CObstacleInstance::getInfo() const
{
	switch(obstacleType)
	{
	case ABSOLUTE_OBSTACLE:
		return VLC->heroh->absoluteObstacles[ID];
	case USUAL:
		return VLC->heroh->obstacles[ID];
	default:
		throw std::runtime_error("Unknown obstacle type in CObstacleInstance::getInfo()");
	}
}

std::vector<BattleHex> CObstacleInstance::getBlockedTiles() const
{
	if(blocksTiles())
		return getAffectedTiles();
	return std::vector<BattleHex>();
}

std::vector<BattleHex> CObstacleInstance::getStoppingTile() const
{
	if(stopsMovement())
		return getAffectedTiles();
	return std::vector<BattleHex>();
}

std::vector<BattleHex> CObstacleInstance::getAffectedTiles() const
{
	switch(obstacleType)
	{
	case ABSOLUTE_OBSTACLE:
	case USUAL:
		return getInfo().getBlocked(pos);
	default:
		assert(0);
		return std::vector<BattleHex>();
	}
}

// bool CObstacleInstance::spellGenerated() const
// {
// 	if(obstacleType == USUAL || obstacleType == ABSOLUTE_OBSTACLE)
// 		return false;
//
// 	return true;
// }

bool CObstacleInstance::visibleForSide(ui8 side, bool hasNativeStack) const
{
	//by default obstacle is visible for everyone
	return true;
}

bool CObstacleInstance::stopsMovement() const
{
	return obstacleType == QUICKSAND || obstacleType == MOAT;
}

bool CObstacleInstance::blocksTiles() const
{
	return obstacleType == USUAL || obstacleType == ABSOLUTE_OBSTACLE || obstacleType == FORCE_FIELD;
}

SpellCreatedObstacle::SpellCreatedObstacle()
{
	casterSide = -1;
	spellLevel = -1;
	casterSpellPower = -1;
	turnsRemaining = -1;
	visibleForAnotherSide = true;
}

bool SpellCreatedObstacle::visibleForSide(ui8 side, bool hasNativeStack) const
{
	//we hide mines and not discovered quicksands
	//quicksands are visible to the caster or if owned unit stepped into that particular patch
	//additionally if side has a native unit, mines/quicksands will be visible

	return casterSide == side || visibleForAnotherSide || hasNativeStack;
}

std::vector<BattleHex> SpellCreatedObstacle::getAffectedTiles() const
{
	return customSize;
}

void SpellCreatedObstacle::battleTurnPassed()
{
	if(turnsRemaining > 0)
		turnsRemaining--;
}

std::vector<BattleHex> MoatObstacle::getAffectedTiles() const
{
	return VLC->townh->factions[ID]->town->moatHexes;
}
