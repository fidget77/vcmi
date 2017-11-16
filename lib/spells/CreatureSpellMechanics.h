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
	AcidBreathDamageMechanics(const IBattleCast * event);
	bool isImmuneByStack(const battle::Unit * obj) const override;
protected:
	void applyBattleEffects(const SpellCastEnvironment * env, const BattleCast & parameters, SpellCastContext & ctx) const override;
};

class DLL_LINKAGE DeathStareMechanics : public RegularSpellMechanics
{
public:
	DeathStareMechanics(const IBattleCast * event);
protected:
	void applyBattleEffects(const SpellCastEnvironment * env, const BattleCast & parameters, SpellCastContext & ctx) const override;
};

class DLL_LINKAGE DispellHelpfulMechanics : public RegularSpellMechanics
{
public:
	DispellHelpfulMechanics(const IBattleCast * event);
	bool isImmuneByStack(const battle::Unit * obj) const override;
protected:
	void applyBattleEffects(const SpellCastEnvironment * env, const BattleCast & parameters, SpellCastContext & ctx) const override;
private:
	static bool positiveSpellEffects(const Bonus * b);
};

} //namespace spells
