/*
 * IUnitInfo.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "../GameConstants.h"

class CCreature;

namespace battle
{

class DLL_LINKAGE IUnitEnvironment
{
public:
	virtual bool unitHasAmmoCart() const = 0; //todo: handle ammo cart with bonus system
};

class DLL_LINKAGE IUnitHealthInfo
{
public:
	virtual int32_t unitMaxHealth() const = 0;
	virtual int32_t unitBaseAmount() const = 0;
};

class DLL_LINKAGE IUnitInfo : public IUnitHealthInfo
{
public:
	bool doubleWide() const;

	virtual uint32_t unitId() const = 0;
	virtual ui8 unitSide() const = 0;
	virtual PlayerColor unitOwner() const = 0;
	virtual SlotID unitSlot() const = 0;

	int32_t creatureIndex() const;
	CreatureID creatureId() const;
	int32_t creatureLevel() const;
	int32_t creatureCost() const;

	virtual const CCreature * creatureType() const = 0;
};

} // namespace battle
