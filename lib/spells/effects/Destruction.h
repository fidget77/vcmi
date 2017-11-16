/*
 * Destruction.h, part of VCMI engine
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

class Destruction : public Damage
{
public:
	Destruction(const int level);
	virtual ~Destruction();

protected:
	virtual int64_t damageForTarget(size_t targetIndex, const Mechanics * m, const battle::Unit * target) const;
	void describeEffect(std::vector<MetaString> & log, const Mechanics * m, const battle::Unit * firstTarget, uint32_t kills, int64_t damage, bool multiple) const override;

	void serializeJsonDamageEffect(JsonSerializeFormat & handler) override;
private:
	bool killByPercentage;
};

} // namespace effects
} // namespace spells
