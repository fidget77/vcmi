/*
 * Teleport.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "UnitEffect.h"

namespace spells
{
namespace effects
{

class Teleport : public UnitEffect
{
public:
	Teleport(const int level);
	virtual ~Teleport();

	bool applicable(Problem & problem, const Mechanics * m) const override;

	void apply(const PacketSender * server, RNG & rng, const Mechanics * m, const EffectTarget & target) const override;

	EffectTarget transformTarget(const Mechanics * m,  const Target & aimPoint, const Target & spellTarget) const override;
protected:
	void serializeJsonUnitEffect(JsonSerializeFormat & handler) override;
private:
};

} // namespace effects
} // namespace spells
