/*
 * CDefaultSpellMechanics.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "ISpellMechanics.h"
#include "../NetPacks.h"
#include "../battle/CBattleInfoEssentials.h"

namespace spells
{

class DLL_LINKAGE SpellCastContext
{
public:
	const Mechanics * mechanics;
	const SpellCastEnvironment * env;
	std::vector<const CStack *> attackedCres;//must be vector, as in Chain Lightning order matters
	StacksInjured si;
	const BattleCast & parameters;

	SpellCastContext(const Mechanics * mechanics_, const SpellCastEnvironment * env_, const BattleCast & parameters_);
	virtual ~SpellCastContext();

	void addDamageToDisplay(const si32 value);
	void setDamageToDisplay(const si32 value);
	si32 getDamageToDisplay() const;

	void addBattleLog(MetaString && line);
	void addCustomEffect(const CStack * target, ui32 effect);

	void beforeCast();
	void cast();
	void afterCast();
private:
	BattleSpellCast sc;
	const CGHeroInstance * otherHero;
	int spellCost;
	si32 damageToDisplay;

	bool counteringSelector(const Bonus * bonus) const;
};

///all combat spells
class DLL_LINKAGE DefaultSpellMechanics : public Mechanics
{
public:
	DefaultSpellMechanics(const CSpell * s, const CBattleInfoCallback * Cb, const Caster * caster_);

	void applyEffects(const SpellCastEnvironment * env, const BattleCast & parameters) const override;
	void applyEffectsForced(const SpellCastEnvironment * env, const BattleCast & parameters) const override;

	std::vector<BattleHex> rangeInHexes(BattleHex centralHex, ui8 schoolLvl, bool * outDroppedHexes = nullptr) const override;

	bool canBeCast(Problem & problem) const override;

	virtual bool isImmuneByStack(const CStack * obj) const;

	virtual bool requiresCreatureTarget() const;

protected:
	virtual void applyEffectsForced(const SpellCastEnvironment * env, const BattleCast & parameters, const Target & targets) const;

	bool canDispell(const IBonusBearer * obj, const CSelector & selector, const std::string & cachingStr = "") const;
	void doDispell(const SpellCastEnvironment * env, SpellCastContext & ctx, const CSelector & selector) const;

	virtual int defaultDamageEffect(const SpellCastEnvironment * env, const BattleCast & parameters, StacksInjured & si, const TStacks & target) const;
	virtual void defaultTimedEffect(const SpellCastEnvironment * env, const BattleCast & parameters, SetStackEffect & sse, const TStacks & target) const;

	void handleResistance(const SpellCastEnvironment * env, SpellCastContext & ctx) const;
	void handleMagicMirror(const SpellCastEnvironment * env, SpellCastContext & ctx, std::vector <const CStack*> & reflected) const;

private:
	static void doRemoveEffects(const SpellCastEnvironment * env, SpellCastContext & ctx, const CSelector & selector);
	static bool dispellSelector(const Bonus * bonus);
	friend class SpellCastContext;
};

///affecting creatures directly
class DLL_LINKAGE RegularSpellMechanics : public DefaultSpellMechanics
{
public:
	RegularSpellMechanics(const CSpell * s, const CBattleInfoCallback * Cb, const Caster * caster_);
	bool canBeCastAt(BattleHex destination) const override;
	void cast(const SpellCastEnvironment * env, const BattleCast & parameters, SpellCastContext & ctx, std::vector <const CStack*> & reflected) const override;
	std::vector<const CStack *> getAffectedStacks(int spellLvl, BattleHex destination) const override final;

protected:
	virtual void applyBattleEffects(const SpellCastEnvironment * env, const BattleCast & parameters, SpellCastContext & ctx) const;
	virtual std::vector<const CStack *> calculateAffectedStacks(int spellLvl, BattleHex destination) const;
private:
	void prepareBattleLog(SpellCastContext & ctx) const;
};

///not affecting creatures directly
class DLL_LINKAGE SpecialSpellMechanics : public DefaultSpellMechanics
{
public:
	SpecialSpellMechanics(const CSpell * s, const CBattleInfoCallback * Cb, const Caster * caster_);
	bool canBeCastAt(BattleHex destination) const override;
	void cast(const SpellCastEnvironment * env, const BattleCast & parameters, SpellCastContext & ctx, std::vector <const CStack*> & reflected) const override;

protected:
	virtual void applyBattleEffects(const SpellCastEnvironment * env, const BattleCast & parameters, SpellCastContext & ctx) const = 0;
	std::vector<const CStack *> getAffectedStacks(int spellLvl, BattleHex destination) const override final;
};

} //namespace spells

