/*
 * Heal.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "UnitEffect.h"
#include "../../GameConstants.h"

struct BattleStacksChanged;

namespace spells
{
namespace effects
{

class Heal : public UnitEffect
{
public:
	Heal(const int level);
	virtual ~Heal();

	void apply(const PacketSender * server, RNG & rng, const Mechanics * m, const EffectTarget & target) const override;
	void apply(IBattleState * battleState, RNG & rng, const Mechanics * m, const EffectTarget & target) const override;

protected:
	bool isValidTarget(const Mechanics * m, const battle::Unit * unit) const override;
	void serializeJsonUnitEffect(JsonSerializeFormat & handler) override final;

	void apply(int64_t value, const PacketSender * server, RNG & rng, const Mechanics * m, const EffectTarget & target) const;

private:
    EHealLevel healLevel;
	EHealPower healPower;

	int32_t minFullUnits;

	void prepareHealEffect(int64_t value, BattleStacksChanged & pack, RNG & rng, const Mechanics * m, const EffectTarget & target) const;
};

} // namespace effects
} // namespace spells
