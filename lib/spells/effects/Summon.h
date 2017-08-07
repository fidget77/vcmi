/*
 * Summon.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "GlobalEffect.h"
#include "../../GameConstants.h"

namespace spells
{
namespace effects
{

class Summon : public GlobalEffect
{
public:
	CreatureID creature;

	Summon(const int level);
	virtual ~Summon();

	bool applicable(Problem & problem, const Mechanics * m) const override;

	void apply(const PacketSender * server, RNG & rng, const Mechanics * m, const BattleCast & p, const EffectTarget & target) const override;

	EffectTarget transformTarget(const Mechanics * m, const Target & aimPoint, const Target & spellTarget) const override;

protected:
	void serializeJsonEffect(JsonSerializeFormat & handler) override final;

private:
	bool permanent;
	bool exclusive;
};

} // namespace effects
} // namespace spells
