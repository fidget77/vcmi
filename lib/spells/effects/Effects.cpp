/*
 * Effects.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "Effects.h"
#include "Registry.h"

#include "../../serializer/JsonSerializeFormat.h"

namespace spells
{
namespace effects
{

void Effects::serializeJson(JsonSerializeFormat & handler, const int level)
{
	assert(!handler.saving);

	const JsonNode & effectMap = handler.getCurrent();

	for(auto & p : effectMap.Struct())
	{
		const std::string & name = p.first;

		auto guard = handler.enterStruct(name);

		std::string type;
		handler.serializeString("effect", type);

		const IEffectFactory * factory = Registry::get()->find(type);
		if(!factory)
		{
			logGlobal->error("Unknown effect type '%s'", type);
			continue;
		}

		auto effect = factory->create();
		effect->serializeJson(handler);
		effect->addTo(this, level);
	}
}

} // namespace effects
} // namespace spells
