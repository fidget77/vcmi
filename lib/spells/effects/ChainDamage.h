/*
 * ChainDamage.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "Damage.h"

namespace spells
{
namespace effects
{

class ChainDamage : public Damage
{
public:
	ChainDamage(const int level);
	virtual ~ChainDamage();

	EffectTarget transformTarget(const Mechanics * m, const Target & aimPoint, const Target & spellTarget) const override;
protected:
	int64_t damageForTarget(size_t targetIndex, const Mechanics * m, const battle::Unit * target) const override;

	void serializeJsonDamageEffect(JsonSerializeFormat & handler) override;
private:
	int32_t length;
	double factor;
};

} // namespace effects
} // namespace spells
