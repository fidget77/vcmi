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

#include "GlobalEffect.h"
#include "../../GameConstants.h"

namespace spells
{
namespace effects
{

class DLL_LINKAGE Summon : public GlobalEffect
{
public:
	Summon();
	virtual ~Summon();

	bool applicable(Problem & problem, const Mechanics * m, const CastParameters & p) const override;

	void apply(const PacketSender * server, const Mechanics * m, const CastParameters & p) const override;

	void serializeJson(JsonSerializeFormat & handler) override;
protected:

private:
	CreatureID creature;
	bool permanent;
	bool exclusive;
};

} // namespace effects
} // namespace spells
