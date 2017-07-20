/*
 * CustomSpellMechanics.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "CDefaultSpellMechanics.h"

#include "effects/Effects.h"

class CustomSpellMechanics : public DefaultSpellMechanics
{
public:
	CustomSpellMechanics(const CSpell * s, const CBattleInfoCallback * Cb, std::shared_ptr<spells::effects::Effects> e);
	virtual ~CustomSpellMechanics();

protected:

private:
	std::shared_ptr<spells::effects::Effects> effects;
};
