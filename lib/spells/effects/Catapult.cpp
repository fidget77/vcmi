/*
 * Catapult.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "Catapult.h"

#include "Registry.h"
#include "../ISpellMechanics.h"

#include "../../NetPacks.h"
#include "../../battle/IBattleState.h"
#include "../../battle/CBattleInfoCallback.h"
#include "../../mapObjects/CGTownInstance.h"

static const std::string EFFECT_NAME = "core:catapult";

namespace spells
{
namespace effects
{

VCMI_REGISTER_SPELL_EFFECT(Catapult, EFFECT_NAME);

Catapult::Catapult(const int level)
	: LocationEffect(level)
{
}

Catapult::~Catapult() = default;

bool Catapult::applicable(Problem & problem, const Mechanics * m) const
{
	auto mode = m->mode;

	if(mode == Mode::AFTER_ATTACK || mode == Mode::BEFORE_ATTACK || mode == Mode::SPELL_LIKE_ATTACK || mode == Mode::MAGIC_MIRROR)
	{
		logGlobal->warn("Invalid spell cast attempt: spell %s, mode %d", m->getSpellName(), (int)mode); //should not even try to do it
		return m->adaptProblem(ESpellCastProblem::INVALID, problem);
	}

	auto town = m->cb->battleGetDefendedTown();

	if(nullptr == town)
	{
		return m->adaptProblem(ESpellCastProblem::NO_APPROPRIATE_TARGET, problem);
	}

	if(CGTownInstance::NONE == town->fortLevel())
	{
		return m->adaptProblem(ESpellCastProblem::NO_APPROPRIATE_TARGET, problem);
	}

	if(m->isSmart())
	{
		const auto side = m->cb->playerToSide(m->caster->getOwner());
		if(!side)
			return m->adaptProblem(ESpellCastProblem::INVALID, problem);
		//if spell targeting is smart, then only attacker can use it
		if(side.get() != BattleSide::ATTACKER)
			return m->adaptProblem(ESpellCastProblem::NO_APPROPRIATE_TARGET, problem);
	}

	const auto attackableBattleHexes = m->cb->getAttackableBattleHexes();

	if(attackableBattleHexes.empty())
		return m->adaptProblem(ESpellCastProblem::NO_APPROPRIATE_TARGET, problem);

	return true;
}

void Catapult::apply(const PacketSender * server, RNG & rng, const Mechanics * m, const EffectTarget & target) const
{
	//start with all destructible parts
	static const std::set<EWallPart::EWallPart> possibleTargets =
	{
		EWallPart::KEEP,
		EWallPart::BOTTOM_TOWER,
		EWallPart::BOTTOM_WALL,
		EWallPart::BELOW_GATE,
		EWallPart::OVER_GATE,
		EWallPart::UPPER_WALL,
		EWallPart::UPPER_TOWER,
		EWallPart::GATE
	};

	assert(possibleTargets.size() == EWallPart::PARTS_COUNT);

	const int targetsToAttack = 2 + std::max<int>(spellLevel - 1, 0);

	CatapultAttack ca;
	ca.attacker = -1;

	for(int i = 0; i < targetsToAttack; i++)
	{
		//Any destructible part can be hit regardless of its HP. Multiple hit on same target is allowed.
		EWallPart::EWallPart target = *RandomGeneratorUtil::nextItem(possibleTargets, rng);

		auto state = m->cb->battleGetWallState(target);

		if(state == EWallState::DESTROYED || state == EWallState::NONE)
			continue;

		CatapultAttack::AttackInfo attackInfo;

		attackInfo.damageDealt = 1;
		attackInfo.attackedPart = target;
		attackInfo.destinationTile = m->cb->wallPartToBattleHex(target);

		ca.attackedParts.push_back(attackInfo);

		//removing creatures in turrets / keep if one is destroyed
		BattleHex posRemove;

		switch(target)
		{
		case EWallPart::KEEP:
			posRemove = -2;
			break;
		case EWallPart::BOTTOM_TOWER:
			posRemove = -3;
			break;
		case EWallPart::UPPER_TOWER:
			posRemove = -4;
			break;
		}

		if(posRemove != BattleHex::INVALID)
		{
			BattleStacksRemoved bsr;
			auto all = m->cb->battleGetUnitsIf([=](const battle::Unit * unit)
			{
				return !unit->isGhost();
			});

			for(auto & elem : all)
			{
				if(elem->getPosition() == posRemove)
				{
					bsr.stackIDs.insert(elem->unitId());
					break;
				}
			}
			if(bsr.stackIDs.size() > 0)
				server->sendAndApply(&bsr);
		}
	}

	server->sendAndApply(&ca);
}


void Catapult::serializeJsonEffect(JsonSerializeFormat & handler)
{
	//TODO: add configuration unifying with Catapult ability
}


} // namespace effects
} // namespace spells
