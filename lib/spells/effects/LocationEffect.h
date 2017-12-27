/*
 * LocationEffect.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "Effect.h"

namespace spells
{
namespace effects
{

class LocationEffect : public Effect<TargetType::LOCATION>, public std::enable_shared_from_this<LocationEffect>
{
public:
	LocationEffect();
	virtual ~LocationEffect();

	void addTo(Effects * where, const int level) override;
protected:

private:
};

} // namespace effects
} // namespace spells
