/*
 * Sacrifice.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "Heal.h"

struct BattleStacksRemoved;

namespace spells
{
namespace effects
{

class Sacrifice : public Heal
{
public:
	Sacrifice(const int level);
	virtual ~Sacrifice();

	bool applicable(Problem & problem, const Mechanics * m) const override;
	bool applicable(Problem & problem, const Mechanics * m, const Target & aimPoint, const EffectTarget & target) const override;

	void apply(const PacketSender * server, RNG & rng, const Mechanics * m, const EffectTarget & target) const override;

	EffectTarget filterTarget(const Mechanics * m, const EffectTarget & target) const override;

	EffectTarget transformTarget(const Mechanics * m, const Target & aimPoint, const Target & spellTarget) const override;

protected:
	bool isValidTarget(const Mechanics * m, const battle::Unit * unit) const override;
	void serializeJsonHealEffect(JsonSerializeFormat & handler);

private:

};

} // namespace effects
} // namespace spells
