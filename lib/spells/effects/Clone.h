/*
 * {file}.h, part of VCMI engine
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

class Clone : public UnitEffect
{
public:
	Clone(const int level);
	virtual ~Clone();

	void apply(const PacketSender * server, RNG & rng, const Mechanics * m, const EffectTarget & target) const override;
protected:
	bool isReceptive(const Mechanics * m, const battle::Unit * s) const override;
	bool isValidTarget(const Mechanics * m, const battle::Unit * s) const override;

	void serializeJsonUnitEffect(JsonSerializeFormat & handler) override final;
private:
	int maxTier;
};

} // namespace effects
} // namespace spells
