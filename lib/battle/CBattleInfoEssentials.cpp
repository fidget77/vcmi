/*
 * CBattleInfoEssentials.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CBattleInfoEssentials.h"
#include "../CStack.h"
#include "BattleInfo.h"
#include "../NetPacks.h"
#include "../mapObjects/CGTownInstance.h"

ETerrainType CBattleInfoEssentials::battleTerrainType() const
{
	RETURN_IF_NOT_BATTLE(ETerrainType::WRONG);
	return getBattle()->getTerrainType();
}

BFieldType CBattleInfoEssentials::battleGetBattlefieldType() const
{
	RETURN_IF_NOT_BATTLE(BFieldType::NONE);
	return getBattle()->getBattlefieldType();
}

int32_t CBattleInfoEssentials::battleGetEnchanterCounter(ui8 side) const
{
	RETURN_IF_NOT_BATTLE(0);
	return getBattle()->getEnchanterCounter(side);
}

std::vector<std::shared_ptr<const CObstacleInstance>> CBattleInfoEssentials::battleGetAllObstacles(boost::optional<BattlePerspective::BattlePerspective> perspective) const
{
	std::vector<std::shared_ptr<const CObstacleInstance> > ret;
	RETURN_IF_NOT_BATTLE(ret);

	if(!perspective)
	{
		//if no particular perspective request, use default one
		perspective = boost::make_optional(battleGetMySide());
	}
	else
	{
		if(!!player && *perspective != battleGetMySide())
		{
			logGlobal->error("Unauthorized obstacles access attempt!");
			return ret;
		}
	}

	for(auto obstacle : getBattle()->getAllObstacles())
	{
		if(battleIsObstacleVisibleForSide(*(obstacle.get()), *perspective))
			ret.push_back(obstacle);
	}

	return ret;
}

bool CBattleInfoEssentials::battleIsObstacleVisibleForSide(const CObstacleInstance & coi, BattlePerspective::BattlePerspective side) const
{
	RETURN_IF_NOT_BATTLE(false);
	return side == BattlePerspective::ALL_KNOWING || coi.visibleForSide(side, battleHasNativeStack(side));
}

bool CBattleInfoEssentials::battleHasNativeStack(ui8 side) const
{
	RETURN_IF_NOT_BATTLE(false);

	for(const CStack * s : battleGetAllStacks())
	{
		if(s->side == side && s->getCreature()->isItNativeTerrain(getBattle()->getTerrainType()))
			return true;
	}

	return false;
}

TStacks CBattleInfoEssentials::battleGetAllStacks(bool includeTurrets) const
{
	return battleGetStacksIf([=](const CStack * s)
	{
		return !s->isGhost() && (includeTurrets || !s->isTurret());
	});
}

TStacks CBattleInfoEssentials::battleGetStacksIf(TStackFilter predicate) const
{
	RETURN_IF_NOT_BATTLE(TStacks());
	return getBattle()->getStacksIf(predicate);
}

TStacks CBattleInfoEssentials::battleAliveStacks() const
{
	return battleGetStacksIf([](const CStack * s)
	{
		return s->isValidTarget(false);
	});
}

TStacks CBattleInfoEssentials::battleAliveStacks(ui8 side) const
{
	return battleGetStacksIf([=](const CStack * s)
	{
		return s->isValidTarget(false) && s->side == side;
	});
}

const CGTownInstance * CBattleInfoEssentials::battleGetDefendedTown() const
{
	RETURN_IF_NOT_BATTLE(nullptr);

	return getBattle()->getDefendedTown();
}

BattlePerspective::BattlePerspective CBattleInfoEssentials::battleGetMySide() const
{
	RETURN_IF_NOT_BATTLE(BattlePerspective::INVALID);
	if(!player || player.get().isSpectator())
		return BattlePerspective::ALL_KNOWING;
	if(*player == getBattle()->getSidePlayer(BattleSide::ATTACKER))
		return BattlePerspective::LEFT_SIDE;
	if(*player == getBattle()->getSidePlayer(BattleSide::DEFENDER))
		return BattlePerspective::RIGHT_SIDE;

	logGlobal->error("Cannot find player %s in battle!", player->getStr());
	return BattlePerspective::INVALID;
}

const CStack * CBattleInfoEssentials::battleActiveStack() const
{
	RETURN_IF_NOT_BATTLE(nullptr);
	return battleGetStackByID(getBattle()->getActiveStackID());
}

const CStack* CBattleInfoEssentials::battleGetStackByID(int ID, bool onlyAlive) const
{
	RETURN_IF_NOT_BATTLE(nullptr);

	auto stacks = battleGetStacksIf([=](const CStack * s)
	{
		return s->ID == ID && (!onlyAlive || s->alive());
	});

	if(stacks.empty())
		return nullptr;
	else
		return stacks[0];
}

bool CBattleInfoEssentials::battleDoWeKnowAbout(ui8 side) const
{
	RETURN_IF_NOT_BATTLE(false);
	auto p = battleGetMySide();
	return p == BattlePerspective::ALL_KNOWING || p == side;
}

si8 CBattleInfoEssentials::battleTacticDist() const
{
	RETURN_IF_NOT_BATTLE(0);
	return getBattle()->getTacticDist();
}

si8 CBattleInfoEssentials::battleGetTacticsSide() const
{
	RETURN_IF_NOT_BATTLE(-1);
	return getBattle()->getTacticsSide();
}

const CGHeroInstance * CBattleInfoEssentials::battleGetFightingHero(ui8 side) const
{
	RETURN_IF_NOT_BATTLE(nullptr);
	if(side > 1)
	{
		logGlobal->error("FIXME: %s wrong argument!", __FUNCTION__);
		return nullptr;
	}

	if(!battleDoWeKnowAbout(side))
	{
		logGlobal->error("FIXME: %s access check ", __FUNCTION__);
		return nullptr;
	}

	return getBattle()->getSideHero(side);
}

const CArmedInstance * CBattleInfoEssentials::battleGetArmyObject(ui8 side) const
{
	RETURN_IF_NOT_BATTLE(nullptr);
	if(side > 1)
	{
		logGlobal->error("FIXME: %s wrong argument!", __FUNCTION__);
		return nullptr;
	}
	if(!battleDoWeKnowAbout(side))
	{
		logGlobal->error("FIXME: %s access check!", __FUNCTION__);
		return nullptr;
	}
	return getBattle()->getSideArmy(side);
}

InfoAboutHero CBattleInfoEssentials::battleGetHeroInfo(ui8 side) const
{
	auto hero = getBattle()->getSideHero(side);
	if(!hero)
	{
		logGlobal->warn("%s: side %d does not have hero!", __FUNCTION__, static_cast<int>(side));
		return InfoAboutHero();
	}
	InfoAboutHero::EInfoLevel infoLevel = battleDoWeKnowAbout(side) ? InfoAboutHero::EInfoLevel::DETAILED : InfoAboutHero::EInfoLevel::BASIC;
	return InfoAboutHero(hero, infoLevel);
}

uint32_t CBattleInfoEssentials::battleCastSpells(ui8 side) const
{
	RETURN_IF_NOT_BATTLE(-1);
	return getBattle()->getCastSpells(side);
}

const IBonusBearer * CBattleInfoEssentials::getBattleNode() const
{
	return getBattle()->asBearer();
}

bool CBattleInfoEssentials::battleCanFlee(PlayerColor player) const
{
	RETURN_IF_NOT_BATTLE(false);
	const auto side = playerToSide(player);
	if(!side)
		return false;

	const CGHeroInstance *myHero = battleGetFightingHero(side.get());

	//current player have no hero
	if(!myHero)
		return false;

	//eg. one of heroes is wearing shakles of war
	if(myHero->hasBonusOfType(Bonus::BATTLE_NO_FLEEING))
		return false;

	//we are besieged defender
	if(side.get() == BattleSide::DEFENDER && battleGetSiegeLevel())
	{
		auto town = battleGetDefendedTown();
		if(!town->hasBuilt(BuildingID::ESCAPE_TUNNEL, ETownType::STRONGHOLD))
			return false;
	}

	return true;
}

BattleSideOpt CBattleInfoEssentials::playerToSide(PlayerColor player) const
{
	RETURN_IF_NOT_BATTLE(boost::none);

	if(getBattle()->getSidePlayer(BattleSide::ATTACKER) == player)
		return BattleSideOpt(BattleSide::ATTACKER);

	if(getBattle()->getSidePlayer(BattleSide::DEFENDER) == player)
		return BattleSideOpt(BattleSide::DEFENDER);

	logGlobal->warn("Cannot find side for player %s", player.getStr());

	return boost::none;
}

PlayerColor CBattleInfoEssentials::sideToPlayer(ui8 side) const
{
	RETURN_IF_NOT_BATTLE(PlayerColor::CANNOT_DETERMINE);
    return getBattle()->getSidePlayer(side);
}

ui8 CBattleInfoEssentials::otherSide(ui8 side) const
{
	if(side == BattleSide::ATTACKER)
		return BattleSide::DEFENDER;
	else
		return BattleSide::ATTACKER;
}

PlayerColor CBattleInfoEssentials::otherPlayer(PlayerColor player) const
{
	RETURN_IF_NOT_BATTLE(PlayerColor::CANNOT_DETERMINE);

	auto side = playerToSide(player);
    if(!side)
		return PlayerColor::CANNOT_DETERMINE;

	return getBattle()->getSidePlayer(otherSide(side.get()));
}

bool CBattleInfoEssentials::playerHasAccessToHeroInfo(PlayerColor player, const CGHeroInstance * h) const
{
	RETURN_IF_NOT_BATTLE(false);
	const auto side = playerToSide(player);
	if(side)
	{
		auto opponentSide = otherSide(side.get());
		if(getBattle()->getSideHero(opponentSide) == h)
			return true;
	}
	return false;
}

ui8 CBattleInfoEssentials::battleGetSiegeLevel() const
{
	RETURN_IF_NOT_BATTLE(CGTownInstance::NONE);
	return getBattle()->getDefendedTown() ? getBattle()->getDefendedTown()->fortLevel() : CGTownInstance::NONE;
}

bool CBattleInfoEssentials::battleCanSurrender(PlayerColor player) const
{
	RETURN_IF_NOT_BATTLE(false);
	const auto side = playerToSide(player);
	if(!side)
		return false;
	bool iAmSiegeDefender = (side.get() == BattleSide::DEFENDER && battleGetSiegeLevel());
	//conditions like for fleeing (except escape tunnel presence) + enemy must have a hero
	return battleCanFlee(player) && !iAmSiegeDefender && battleHasHero(otherSide(side.get()));
}

bool CBattleInfoEssentials::battleHasHero(ui8 side) const
{
	RETURN_IF_NOT_BATTLE(false);
	return getBattle()->getSideHero(side) != nullptr;
}

si8 CBattleInfoEssentials::battleGetWallState(int partOfWall) const
{
	RETURN_IF_NOT_BATTLE(EWallState::NONE);
	if(battleGetSiegeLevel() == CGTownInstance::NONE)
		return EWallState::NONE;

	return getBattle()->getWallState(partOfWall);
}

EGateState CBattleInfoEssentials::battleGetGateState() const
{
	RETURN_IF_NOT_BATTLE(EGateState::NONE);
	if(battleGetSiegeLevel() == CGTownInstance::NONE)
		return EGateState::NONE;

	return getBattle()->getGateState();
}

PlayerColor CBattleInfoEssentials::battleGetOwner(const IStackState * stack) const
{
	RETURN_IF_NOT_BATTLE(PlayerColor::CANNOT_DETERMINE);

	PlayerColor initialOwner = getBattle()->getSidePlayer(stack->unitSide());

	if(stack->unitAsBearer()->hasBonusOfType(Bonus::HYPNOTIZED))
		return otherPlayer(initialOwner);
	else
		return initialOwner;
}

const CGHeroInstance * CBattleInfoEssentials::battleGetOwnerHero(const IStackState * stack) const
{
	RETURN_IF_NOT_BATTLE(nullptr);
	const auto side = playerToSide(battleGetOwner(stack));
	if(!side)
		return nullptr;
	return getBattle()->getSideHero(side.get());
}

bool CBattleInfoEssentials::battleMatchOwner(const IStackState * attacker, const IStackState * defender, const boost::logic::tribool positivness) const
{
	RETURN_IF_NOT_BATTLE(false);
	if(boost::logic::indeterminate(positivness))
		return true;
	else if(attacker->unitId() == defender->unitId())
		return positivness;
	else
		return battleMatchOwner(battleGetOwner(attacker), defender, positivness);
}

bool CBattleInfoEssentials::battleMatchOwner(const PlayerColor & attacker, const IStackState * defender, const boost::logic::tribool positivness) const
{
	RETURN_IF_NOT_BATTLE(false);

	PlayerColor initialOwner = getBattle()->getSidePlayer(defender->unitSide());

	if(boost::logic::indeterminate(positivness))
		return true;
	else if(initialOwner != battleGetOwner(defender))
		return true; //mind controlled unit is attackable for both sides
	else
		return (attacker == battleGetOwner(defender)) == positivness;
}
