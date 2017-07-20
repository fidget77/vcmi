/*
 * StackEffect.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "StackEffect.h"
#include "Effects.h"

namespace spells
{
namespace effects
{

StackEffect::StackEffect()
{

}

StackEffect::~StackEffect()
{

}

void StackEffect::addTo(Effects * where, const int level)
{
	where->creature.at(level).push_back(this->shared_from_this());
}


} // namespace effects
} // namespace spells
