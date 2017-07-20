/*
 * Effect.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "Effect.h"

#include "../../serializer/JsonDeserializer.h"

namespace spells
{
namespace effects
{

bool IEffect::applicable(Problem & problem, const Mechanics * m) const
{
	return true;
}

} // namespace effects
} // namespace spells
