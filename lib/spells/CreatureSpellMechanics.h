/*
 * CreatureSpellMechanics.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "ISpellMechanics.h"
#include "CDefaultSpellMechanics.h"

namespace spells
{

class DLL_LINKAGE AcidBreathDamageMechanics : public RegularSpellMechanics
{
public:
	AcidBreathDamageMechanics(const CSpell * s, const CBattleInfoCallback * Cb, const Caster * caster_);
	bool isImmuneByStack(const CStack * obj) const override;
protected:
	void applyBattleEffects(const SpellCastEnvironment * env, const BattleCast & parameters, SpellCastContext & ctx) const override;
};

class DLL_LINKAGE DeathStareMechanics : public RegularSpellMechanics
{
public:
	DeathStareMechanics(const CSpell * s, const CBattleInfoCallback * Cb, const Caster * caster_);
protected:
	void applyBattleEffects(const SpellCastEnvironment * env, const BattleCast & parameters, SpellCastContext & ctx) const override;
};

class DLL_LINKAGE DispellHelpfulMechanics : public RegularSpellMechanics
{
public:
	DispellHelpfulMechanics(const CSpell * s, const CBattleInfoCallback * Cb, const Caster * caster_);
	bool isImmuneByStack(const CStack * obj) const override;
protected:
	void applyBattleEffects(const SpellCastEnvironment * env, const BattleCast & parameters, SpellCastContext & ctx) const override;
private:
	static bool positiveSpellEffects(const Bonus * b);
};

} //namespace spells
