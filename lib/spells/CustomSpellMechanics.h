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

#include "ISpellMechanics.h"
#include "CDefaultSpellMechanics.h"//todo:remove

#include "effects/Effects.h"

namespace spells
{

class CustomSpellMechanics : public DefaultSpellMechanics
{
public:
	CustomSpellMechanics(const CSpell * s, const CBattleInfoCallback * Cb, const Caster * caster_, std::shared_ptr<effects::Effects> e);
	virtual ~CustomSpellMechanics();

	void applyEffects(const SpellCastEnvironment * env, const BattleCast & parameters) const override;
	void applyEffectsForced(const SpellCastEnvironment * env, const BattleCast & parameters) const override;

	bool canBeCast(Problem & problem) const override;
	bool canBeCastAt(BattleHex destination) const override;

	bool requiresCreatureTarget() const	override;

	void cast(const SpellCastEnvironment * env, const BattleCast & parameters, SpellCastContext & ctx, std::vector <const CStack*> & reflected) const override;

	std::vector<const CStack *> getAffectedStacks(int spellLvl, BattleHex destination) const override final;

private:
	std::shared_ptr<effects::Effects> effects;

	Target transformSpellTarget(const Target & aimPoint, const int spellLevel) const;
};

} //namespace spells
