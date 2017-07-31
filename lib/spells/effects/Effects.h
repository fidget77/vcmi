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

class DLL_LINKAGE Effects
{
public:
	using TargetFilter = std::function<bool(const Destination & dest)>;
	using EffectsToApply = std::vector<std::pair<const IEffect *, EffectTarget>>;

	template <TargetType TType>
	using EffectVector = std::array<std::vector<std::shared_ptr<Effect<TType>>>, GameConstants::SPELL_SCHOOL_LEVELS>;

	EffectVector<TargetType::NO_TARGET> global;
	EffectVector<TargetType::LOCATION> location;
	EffectVector<TargetType::CREATURE> creature;

	Effects() = default;
	virtual ~Effects() = default;

	bool applicable(Problem & problem, const Mechanics * m, const int level) const;
	bool applicable(Problem & problem, const Mechanics * m, const int level, const Target & aimPoint, const Target & spellTarget) const;

	void forEachEffect(const int level, const std::function<void(const IEffect *, bool &)> & callback) const;

	EffectsToApply prepare(const Mechanics * m, const BattleCast & p, const Target & spellTarget) const;

	void serializeJson(JsonSerializeFormat & handler, const int level);
};


} // namespace effects
} // namespace spells
