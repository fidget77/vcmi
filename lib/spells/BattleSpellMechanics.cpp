/*
 * BattleSpellMechanics.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "BattleSpellMechanics.h"

#include "../NetPacks.h"
#include "../CStack.h"
#include "../battle/BattleInfo.h"
#include "../mapObjects/CGHeroInstance.h"
#include "../mapObjects/CGTownInstance.h"

namespace spells
{


///ObstacleMechanics
ObstacleMechanics::ObstacleMechanics(const IBattleCast * event)
	: SpecialSpellMechanics(event)
{
}

bool ObstacleMechanics::canBeCastAt(BattleHex destination) const
{
	bool hexesOutsideBattlefield = false;

	auto tilesThatMustBeClear = rangeInHexes(destination, &hexesOutsideBattlefield);
	const CSpell::TargetInfo ti(owner, getRangeLevel(), mode);
	for(const BattleHex & hex : tilesThatMustBeClear)
		if(!isHexAviable(cb, hex, ti.clearAffected))
			return false;

	if(hexesOutsideBattlefield)
		return false;

	return true;
}

bool ObstacleMechanics::isHexAviable(const CBattleInfoCallback * cb, const BattleHex & hex, const bool mustBeClear)
{
	if(!hex.isAvailable())
		return false;

	if(!mustBeClear)
		return true;

	if(cb->battleGetStackByPos(hex, true))
		return false;

	auto obst = cb->battleGetAllObstaclesOnPos(hex, false);

	for(auto & i : obst)
		if(i->obstacleType != CObstacleInstance::MOAT)
			return false;

	if(cb->battleGetDefendedTown() != nullptr && cb->battleGetDefendedTown()->fortLevel() != CGTownInstance::NONE)
	{
		EWallPart::EWallPart part = cb->battleHexToWallPart(hex);

		if(part == EWallPart::INVALID || part == EWallPart::INDESTRUCTIBLE_PART_OF_GATE)
			return true;//no fortification here
		else if(static_cast<int>(part) < 0)
			return false;//indestuctible part (cant be checked by battleGetWallState)
		else if(part == EWallPart::BOTTOM_TOWER || part == EWallPart::UPPER_TOWER)
			return false;//destructible, but should not be available
		else if(cb->battleGetWallState(part) != EWallState::DESTROYED && cb->battleGetWallState(part) != EWallState::NONE)
			return false;
	}

	return true;
}

void ObstacleMechanics::placeObstacle(const SpellCastEnvironment * env, const BattleCast & parameters, const BattleHex & pos) const
{
	//do not trust obstacle count, some of them may be removed
	auto all = cb->battleGetAllObstacles(BattlePerspective::ALL_KNOWING);

	int obstacleIdToGive = 1;
	for(auto & one : all)
		if(one->uniqueID >= obstacleIdToGive)
			obstacleIdToGive = one->uniqueID + 1;

	auto obstacle = std::make_shared<SpellCreatedObstacle>();
	obstacle->pos = pos;
	obstacle->casterSide = casterSide;
	obstacle->ID = getSpellIndex();
	obstacle->spellLevel = getEffectLevel();
	obstacle->casterSpellPower = getEffectPower();
	obstacle->uniqueID = obstacleIdToGive;
	obstacle->customSize = std::vector<BattleHex>(1, pos);
	setupObstacle(obstacle.get());

	BattleObstaclePlaced bop;
	bop.obstacle = obstacle;
	env->sendAndApply(&bop);
}

///PatchObstacleMechanics
PatchObstacleMechanics::PatchObstacleMechanics(const IBattleCast * event)
	: ObstacleMechanics(event)
{
}

void PatchObstacleMechanics::applyBattleEffects(const SpellCastEnvironment * env, const BattleCast & parameters, SpellCastContext & ctx) const
{
	std::vector<BattleHex> availableTiles;
	for(int i = 0; i < GameConstants::BFIELD_SIZE; i += 1)
	{
		BattleHex hex = i;
		if(isHexAviable(cb, hex, true))
			availableTiles.push_back(hex);
	}
	RandomGeneratorUtil::randomShuffle(availableTiles, env->getRandomGenerator());
	static const std::array<int, 4> patchesForSkill = {4, 4, 6, 8};
	const int patchesToPut = std::min<int>(patchesForSkill.at(getRangeLevel()), availableTiles.size());

	//land mines or quicksand patches are handled as spell created obstacles
	for (int i = 0; i < patchesToPut; i++)
		placeObstacle(env, parameters, availableTiles.at(i));
}

///LandMineMechanics
LandMineMechanics::LandMineMechanics(const IBattleCast * event)
	: PatchObstacleMechanics(event)
{
}

bool LandMineMechanics::canBeCast(Problem & problem) const
{
	//LandMine are useless if enemy has native stack and can see mines, check for LandMine damage immunity is done in general way by CSpell
	const auto side = cb->playerToSide(caster->getOwner());
	if(!side)
		return adaptProblem(ESpellCastProblem::INVALID, problem);

	const ui8 otherSide = cb->otherSide(side.get());

	if(cb->battleHasNativeStack(otherSide))
		return adaptProblem(ESpellCastProblem::NO_APPROPRIATE_TARGET, problem);

	return SpecialSpellMechanics::canBeCast(problem);
}

int LandMineMechanics::defaultDamageEffect(const SpellCastEnvironment * env, const BattleCast & parameters, StacksInjured & si, const std::vector<const battle::Unit *> & target) const
{
	auto res = PatchObstacleMechanics::defaultDamageEffect(env, parameters, si, target);

	for(BattleStackAttacked & bsa : si.stacks)
	{
		bsa.effect = 82;
		bsa.flags |= BattleStackAttacked::EFFECT;
	}

	return res;
}

bool LandMineMechanics::requiresCreatureTarget() const
{
	return true;
}

void LandMineMechanics::setupObstacle(SpellCreatedObstacle * obstacle) const
{
	obstacle->obstacleType = CObstacleInstance::LAND_MINE;
	obstacle->turnsRemaining = -1;
	obstacle->visibleForAnotherSide = false;
}

///QuicksandMechanics
QuicksandMechanics::QuicksandMechanics(const IBattleCast * event)
	: PatchObstacleMechanics(event)
{
}

bool QuicksandMechanics::requiresCreatureTarget() const
{
	return false;
}

void QuicksandMechanics::setupObstacle(SpellCreatedObstacle * obstacle) const
{
	obstacle->obstacleType = CObstacleInstance::QUICKSAND;
	obstacle->turnsRemaining = -1;
	obstacle->visibleForAnotherSide = false;
}

///WallMechanics
WallMechanics::WallMechanics(const IBattleCast * event)
	: ObstacleMechanics(event)
{
}

std::vector<BattleHex> WallMechanics::rangeInHexes(BattleHex centralHex, bool * outDroppedHexes) const
{
	std::vector<BattleHex> ret;

	//Special case - shape of obstacle depends on caster's side
	//TODO make it possible through spell config

	BattleHex::EDir firstStep, secondStep;
	if(casterSide)
	{
		firstStep = BattleHex::TOP_LEFT;
		secondStep = BattleHex::TOP_RIGHT;
	}
	else
	{
		firstStep = BattleHex::TOP_RIGHT;
		secondStep = BattleHex::TOP_LEFT;
	}

	//Adds hex to the ret if it's valid. Otherwise sets output arg flag if given.
	auto addIfValid = [&](BattleHex hex)
	{
		if(hex.isValid())
			ret.push_back(hex);
		else if(outDroppedHexes)
			*outDroppedHexes = true;
	};

	ret.push_back(centralHex);
	addIfValid(centralHex.moveInDirection(firstStep, false));
	if(getRangeLevel() >= 2) //advanced versions of fire wall / force field cotnains of 3 hexes
		addIfValid(centralHex.moveInDirection(secondStep, false)); //moveInDir function modifies subject hex

	return ret;
}

///FireWallMechanics
FireWallMechanics::FireWallMechanics(const IBattleCast * event)
	: WallMechanics(event)
{
}

bool FireWallMechanics::requiresCreatureTarget() const
{
	return true;
}

void FireWallMechanics::applyBattleEffects(const SpellCastEnvironment * env, const BattleCast & parameters, SpellCastContext & ctx) const
{
	const BattleHex destination = parameters.getFirstDestinationHex();

	if(!destination.isValid())
	{
		env->complain("Invalid destination for FIRE_WALL");
		return;
	}
	//firewall is build from multiple obstacles - one fire piece for each affected hex
	auto affectedHexes = rangeInHexes(destination);
	for(BattleHex hex : affectedHexes)
		placeObstacle(env, parameters, hex);
}

void FireWallMechanics::setupObstacle(SpellCreatedObstacle * obstacle) const
{
	obstacle->obstacleType = CObstacleInstance::FIRE_WALL;
	obstacle->turnsRemaining = 2;
	obstacle->visibleForAnotherSide = true;
}

///ForceFieldMechanics
ForceFieldMechanics::ForceFieldMechanics(const IBattleCast * event)
	: WallMechanics(event)
{
}

bool ForceFieldMechanics::requiresCreatureTarget() const
{
	return false;
}

void ForceFieldMechanics::applyBattleEffects(const SpellCastEnvironment * env, const BattleCast & parameters, SpellCastContext & ctx) const
{
	const BattleHex destination = parameters.getFirstDestinationHex();

	if(!destination.isValid())
	{
		env->complain("Invalid destination for FORCE_FIELD");
		return;
	}
	placeObstacle(env, parameters, destination);
}

void ForceFieldMechanics::setupObstacle(SpellCreatedObstacle * obstacle) const
{
	obstacle->obstacleType = CObstacleInstance::FORCE_FIELD;
	obstacle->turnsRemaining = 2;
	obstacle->visibleForAnotherSide = true;
	obstacle->customSize = rangeInHexes(obstacle->pos);
}




} // namespace spells

