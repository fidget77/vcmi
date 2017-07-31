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

namespace spells
{
namespace effects
{

///Direct (if automatic) or indirect damage
class Damage : public StackEffect
{
public:
	Damage();
	virtual ~Damage() = default;

	void apply(const PacketSender * server, RNG & rng, const Mechanics * m, const BattleCast & p, const EffectTarget & target) const override;

protected:
	void serializeJsonEffect(JsonSerializeFormat & handler) override final;

private:
};

} // namespace effects
} // namespace spells
