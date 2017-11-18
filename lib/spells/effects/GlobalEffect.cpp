/*
 * GlobalEffect.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"

#include "GlobalEffect.h"

#include "../ISpellMechanics.h"

namespace spells
{
namespace effects
{

GlobalEffect::GlobalEffect(const int level)
	: Effect(level)
{
}

GlobalEffect::~GlobalEffect() = default;

void GlobalEffect::adjustTargetTypes(std::vector<TargetType> & types) const
{
	//any target type allowed
}

EffectTarget GlobalEffect::filterTarget(const Mechanics * m, const EffectTarget & target) const
{
	return target;
}

EffectTarget GlobalEffect::transformTarget(const Mechanics * m, const Target & aimPoint, const Target & spellTarget) const
{
	//ignore spellTarget and reuse initial target
	return aimPoint;
}

} // namespace effects
} // namespace spells
