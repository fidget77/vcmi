/*
 * StackEffect.h, part of VCMI engine
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

class StackEffect : public Effect<TargetType::CREATURE>, public std::enable_shared_from_this<StackEffect>
{
public:
	StackEffect();
	virtual ~StackEffect() = default;

	void addTo(Effects * where, const int level) override;

	bool applicable(Problem & problem, const Mechanics * m) const override;
	bool applicable(Problem & problem, const Mechanics * m, const Target & aimPoint, const EffectTarget & target) const override;

	EffectTarget filterTarget(const Mechanics * m, const BattleCast & p, const EffectTarget & target) const override;

	virtual EffectTarget transformTarget(const Mechanics * m, const Target & aimPoint, const Target & spellTarget) const;

    bool getStackFilter(const Mechanics * m, bool alwaysSmart, const CStack * s) const;

    virtual bool eraseByImmunityFilter(const Mechanics * m, const CStack * s) const;
protected:

private:
};

} // namespace effects
} // namespace spells
