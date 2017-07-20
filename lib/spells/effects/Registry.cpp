/*
 * Registry.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "Registry.h"

namespace spells
{
namespace effects
{

std::unique_ptr<Registry> Instance;

Registry::Registry()
{
}

Registry::~Registry()
{
}

const IEffectFactory * Registry::find(const std::string & name) const
{
    auto iter = data.find(name);
    if(iter == data.end())
		return nullptr;
	else
		return iter->second.get();
}

Registry * Registry::get()
{
	if(!Instance)
	{
		Instance = make_unique<Registry>();
	}
	return Instance.get();
}


} // namespace effects
} // namespace spells
