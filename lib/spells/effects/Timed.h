/*
 * Timed.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "StackEffect.h"

struct Bonus;
struct SetStackEffect;

namespace spells
{
namespace effects
{

class Timed : public StackEffect
{
public:
	bool cumulative;
	std::vector<std::shared_ptr<Bonus>> bonus;

	Timed(const int level);
	virtual ~Timed();

	void apply(const PacketSender * server, RNG & rng, const Mechanics * m, const EffectTarget & target) const override;
	void apply(IBattleState * battleState, RNG & rng, const Mechanics * m, const EffectTarget & target) const override;

protected:
	void serializeJsonEffect(JsonSerializeFormat & handler) override final;

private:
	void convertBonus(const Mechanics * m, int32_t & duration, std::vector<Bonus> & converted) const;
	void prepareEffects(SetStackEffect & sse, const Mechanics * m, const EffectTarget & target) const;
};

} // namespace effects
} // namespace spells
