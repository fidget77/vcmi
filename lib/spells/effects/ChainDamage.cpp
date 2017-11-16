/*
 * ChainDamage.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "ChainDamage.h"
#include "Registry.h"
#include "../ISpellMechanics.h"

#include "../../NetPacks.h"
#include "../../CStack.h"
#include "../../battle/IBattleState.h"
#include "../../battle/CBattleInfoCallback.h"
#include "../../serializer/JsonSerializeFormat.h"

static const std::string EFFECT_NAME = "core:chainDamage";
static const int32_t DEFAULT_LENGTH = 2; //minimal chain should be at least 2 long
static const double DEFAULT_FACTOR = .5f; //original chain lightning

namespace spells
{
namespace effects
{

VCMI_REGISTER_SPELL_EFFECT(ChainDamage, EFFECT_NAME);

ChainDamage::ChainDamage(const int level)
	: Damage(level),
	length(DEFAULT_LENGTH),
	factor(DEFAULT_FACTOR)
{
}

ChainDamage::~ChainDamage() = default;

EffectTarget ChainDamage::transformTarget(const Mechanics * m, const Target & aimPoint, const Target & spellTarget) const
{
	if(aimPoint.empty())
	{
		logGlobal->error("Chain damage effect with no target");
		return EffectTarget();
	}

	const Destination & mainDestination = aimPoint.front();

	if(!mainDestination.hexValue.isValid())
	{
		logGlobal->error("Chain damage effect with invalid target");
		return EffectTarget();
	}

	std::set<BattleHex> possibleHexes;

	auto possibleTargets = m->cb->battleGetUnitsIf([&](const battle::Unit * unit) -> bool
	{
		return unit->isValidTarget(true);
	});

	for(auto unit : possibleTargets)
	{
		for(auto hex : battle::Unit::getHexes(unit->getPosition(), unit->doubleWide(), unit->unitSide()))
			possibleHexes.insert(hex);
	}

	BattleHex lightningHex = mainDestination.hexValue;
	EffectTarget res;

	for(int32_t targetIndex = 0; targetIndex < length; ++targetIndex)
	{
		auto unit = m->cb->battleGetUnitByPos(lightningHex, true);

		if(!unit)
			break;
		if(m->mode == Mode::SPELL_LIKE_ATTACK && targetIndex == 0)
			res.emplace_back(unit);
		else if(isReceptive(m, unit) && isValidTarget(m, unit))
			res.emplace_back(unit);
		else
			res.emplace_back();

		for(auto hex : battle::Unit::getHexes(unit->getPosition(), unit->doubleWide(), unit->unitSide()))
			possibleHexes.erase(hex);

		lightningHex = BattleHex::getClosestTile(unit->unitSide(), lightningHex, possibleHexes);
	}

	return res;
}

int64_t ChainDamage::damageForTarget(size_t targetIndex, const Mechanics * m, const battle::Unit * target) const
{
	int64_t damage = Damage::damageForTarget(targetIndex, m, target);

	if(targetIndex == 0)
		return damage;

	double indexedFactor = std::pow(factor, (double) targetIndex);

	return (int64_t) (indexedFactor * damage);
}

void ChainDamage::serializeJsonDamageEffect(JsonSerializeFormat & handler)
{
	handler.serializeInt("length", length, DEFAULT_LENGTH);
	handler.serializeFloat("factor", factor, DEFAULT_FACTOR);

	if(!handler.saving)
	{
		vstd::amax(length, 1);
	}
}

} // namespace effects
} // namespace spells
