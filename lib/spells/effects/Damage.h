/*
 * Damage.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "StackEffect.h"

struct StacksInjured;

namespace spells
{
namespace effects
{

///Direct (if automatic) or indirect damage
class Damage : public StackEffect
{
public:
	Damage(const int level);
	virtual ~Damage();

	void apply(const PacketSender * server, RNG & rng, const Mechanics * m, const EffectTarget & target) const override;
	void apply(IBattleState * battleState, RNG & rng, const Mechanics * m, const EffectTarget & target) const override;

protected:
	void serializeJsonEffect(JsonSerializeFormat & handler) override final;

private:
	void prepareEffects(StacksInjured & stacksInjured, RNG & rng, const Mechanics * m, const EffectTarget & target) const;
};

} // namespace effects
} // namespace spells
