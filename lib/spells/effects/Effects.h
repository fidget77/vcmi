/*
 * Effects.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "Effect.h"
#include "../../GameConstants.h"

namespace spells
{
namespace effects
{

class Effects
{
public:
	using TargetFilter = std::function<bool(const Destination & dest)>;
	using EffectsToApply = std::vector<std::pair<const Effect *, EffectTarget>>;

	using EffectVector = std::array<std::vector<std::shared_ptr<Effect>>, GameConstants::SPELL_SCHOOL_LEVELS>;

	EffectVector data;

	Effects();
	virtual ~Effects();

	void add(std::shared_ptr<Effect> effect, const int level);

	bool applicable(Problem & problem, const Mechanics * m, const int level) const;
	bool applicable(Problem & problem, const Mechanics * m, const int level, const Target & aimPoint, const Target & spellTarget) const;

	void forEachEffect(const int level, const std::function<void(const Effect *, bool &)> & callback) const;

	EffectsToApply prepare(const Mechanics * m, const BattleCast & p, const Target & spellTarget) const;

	void serializeJson(JsonSerializeFormat & handler, const int level);
};


} // namespace effects
} // namespace spells
