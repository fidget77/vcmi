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
#include "Effects.h"

#include "../ISpellMechanics.h"

namespace spells
{
namespace effects
{

GlobalEffect::GlobalEffect()
{
}

GlobalEffect::~GlobalEffect()
{

}

void GlobalEffect::addTo(Effects * where, const int level)
{
	spellLevel = level;
	where->global.at(level).push_back(this->shared_from_this());
}

EffectTarget GlobalEffect::filterTarget(const Mechanics * m, const BattleCast & p, const EffectTarget & target) const
{
	EffectTarget res;
	vstd::copy_if(target, std::back_inserter(res), [](const Destination & d)
	{
		//we can apply only to default target, but not only once
		return !d.stackValue && (d.hexValue == BattleHex::INVALID);
	});
	return res;
}

EffectTarget GlobalEffect::transformTarget(const Mechanics * m, const Target & aimPoint, const Target & spellTarget) const
{
	//ignore spellTarget and reuse initial target
	return aimPoint;
}

} // namespace effects
} // namespace spells
