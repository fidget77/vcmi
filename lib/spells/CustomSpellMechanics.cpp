/*
 * CustomSpellMechanics.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CustomSpellMechanics.h"

CustomSpellMechanics::CustomSpellMechanics(const CSpell * s, const CBattleInfoCallback * Cb, std::shared_ptr<spells::effects::Effects> e)
	: DefaultSpellMechanics(s, Cb), effects(e)
{}

CustomSpellMechanics::~CustomSpellMechanics()
{}
