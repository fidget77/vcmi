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
	Effects() = default;
	virtual ~Effects() = default;

	template <TargetType TType>
	using EffectVector = std::array<std::vector<std::shared_ptr<Effect<TType>>>, GameConstants::SPELL_LEVELS>;

	EffectVector<TargetType::NO_TARGET> global;
	EffectVector<TargetType::LOCATION> location;
	EffectVector<TargetType::CREATURE> creature;

	void serializeJson(JsonSerializeFormat & handler, const int level);
};


} // namespace effects
} // namespace spells
