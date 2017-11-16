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

#include "UnitEffect.h"

struct StacksInjured;

namespace spells
{
namespace effects
{

///Direct (if automatic) or indirect damage
class Damage : public UnitEffect
{
public:
	Damage(const int level);
	virtual ~Damage();

	void apply(const PacketSender * server, RNG & rng, const Mechanics * m, const EffectTarget & target) const override;
	void apply(IBattleState * battleState, RNG & rng, const Mechanics * m, const EffectTarget & target) const override;

protected:
	void serializeJsonUnitEffect(JsonSerializeFormat & handler) override final;
	virtual void serializeJsonDamageEffect(JsonSerializeFormat & handler);

	virtual int64_t damageForTarget(size_t targetIndex, const Mechanics * m, const battle::Unit * target) const;

	virtual void describeEffect(std::vector<MetaString> & log, const Mechanics * m, const battle::Unit * firstTarget, uint32_t kills, int64_t damage, bool multiple) const;
private:
	void prepareEffects(StacksInjured & stacksInjured, RNG & rng, const Mechanics * m, const EffectTarget & target, bool describe) const;
};

} // namespace effects
} // namespace spells
