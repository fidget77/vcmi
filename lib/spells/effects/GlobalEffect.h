/*
 * GlobalEffect.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "Effect.h"

namespace spells
{
namespace effects
{

class GlobalEffect : public Effect
{
public:
	GlobalEffect(const int level);
	virtual ~GlobalEffect();

	EffectTarget filterTarget(const Mechanics * m, const BattleCast & p, const EffectTarget & target) const override;

	EffectTarget transformTarget(const Mechanics * m,  const Target & aimPoint, const Target & spellTarget) const override;
protected:

private:
};

} // namespace effects
} // namespace spells
