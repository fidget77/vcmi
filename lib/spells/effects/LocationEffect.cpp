/*
 * LocationEffect.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "LocationEffect.h"
#include "../ISpellMechanics.h"

namespace spells
{
namespace effects
{

LocationEffect::LocationEffect(const int level)
	: Effect(level)
{
}

LocationEffect::~LocationEffect() = default;

EffectTarget LocationEffect::filterTarget(const Mechanics * m, const BattleCast & p, const EffectTarget & target) const
{
	EffectTarget res;
	vstd::copy_if(target, std::back_inserter(res), [](const Destination & d)
	{
		return !d.stackValue && (d.hexValue.isValid());
	});
	return res;
}

EffectTarget LocationEffect::transformTarget(const Mechanics * m, const Target & aimPoint, const Target & spellTarget) const
{
	//by def ault effect covers exactly spell range
	return EffectTarget(spellTarget);
}

} // namespace effects
} // namespace spells
