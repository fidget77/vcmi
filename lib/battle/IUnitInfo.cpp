/*
 * IUnitInfo.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "IUnitInfo.h"

#include "../CCreatureHandler.h"

namespace battle
{

int32_t IUnitInfo::creatureIndex() const
{
	return static_cast<int32_t>(creatureId().toEnum());
}

CreatureID IUnitInfo::creatureId() const
{
	return creatureType()->idNumber;
}

int32_t IUnitInfo::creatureLevel() const
{
	return static_cast<int32_t>(creatureType()->level);
}

bool IUnitInfo::doubleWide() const
{
	return creatureType()->doubleWide;
}

} // namespace battle
