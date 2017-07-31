/*
 * BattleSpellMechanics.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "CDefaultSpellMechanics.h"

class CObstacleInstance;
class SpellCreatedObstacle;

namespace spells
{

class DLL_LINKAGE HealingSpellMechanics : public RegularSpellMechanics
{
public:
	HealingSpellMechanics(const CSpell * s, const CBattleInfoCallback * Cb, const Caster * caster_);
protected:
	void applyBattleEffects(const SpellCastEnvironment * env, const BattleCast & parameters, SpellCastContext & ctx) const override;
	virtual int calculateHealedHP(const SpellCastEnvironment * env, const BattleCast & parameters, SpellCastContext & ctx) const;
	virtual EHealLevel getHealLevel(int effectLevel) const = 0;
	virtual EHealPower getHealPower(int effectLevel) const = 0;
private:
	static bool cureSelector(const Bonus * b);
};

class DLL_LINKAGE AntimagicMechanics : public RegularSpellMechanics
{
public:
	AntimagicMechanics(const CSpell * s, const CBattleInfoCallback * Cb, const Caster * caster_);
protected:
	void applyBattleEffects(const SpellCastEnvironment * env, const BattleCast & parameters, SpellCastContext & ctx) const override;
};

class DLL_LINKAGE ChainLightningMechanics : public RegularSpellMechanics
{
public:
	ChainLightningMechanics(const CSpell * s, const CBattleInfoCallback * Cb, const Caster * caster_);
protected:
	void applyBattleEffects(const SpellCastEnvironment * env, const BattleCast & parameters, SpellCastContext & ctx) const override;
	std::vector<const CStack *> calculateAffectedStacks(int spellLvl, BattleHex destination) const override;
};

class DLL_LINKAGE CloneMechanics : public RegularSpellMechanics
{
public:
	CloneMechanics(const CSpell * s, const CBattleInfoCallback * Cb, const Caster * caster_);
	bool isImmuneByStack(const CStack * obj) const override;
protected:
	void applyBattleEffects(const SpellCastEnvironment * env, const BattleCast & parameters, SpellCastContext & ctx) const override;
};

class DLL_LINKAGE CureMechanics : public HealingSpellMechanics
{
public:
	CureMechanics(const CSpell * s, const CBattleInfoCallback * Cb, const Caster * caster_);
	bool isImmuneByStack(const CStack * obj) const override;
	EHealLevel getHealLevel(int effectLevel) const override final;
	EHealPower getHealPower(int effectLevel) const override final;
protected:
	void applyBattleEffects(const SpellCastEnvironment * env, const BattleCast & parameters, SpellCastContext & ctx) const override;
private:
    static bool dispellSelector(const Bonus * b);
};

class DLL_LINKAGE DispellMechanics : public RegularSpellMechanics
{
public:
	DispellMechanics(const CSpell * s, const CBattleInfoCallback * Cb, const Caster * caster_);
	bool isImmuneByStack(const CStack * obj) const override;
protected:
	void applyBattleEffects(const SpellCastEnvironment * env, const BattleCast & parameters, SpellCastContext & ctx) const override;
};

class DLL_LINKAGE EarthquakeMechanics : public SpecialSpellMechanics
{
public:
	EarthquakeMechanics(const CSpell * s, const CBattleInfoCallback * Cb, const Caster * caster_);
	bool canBeCast(Problem & problem) const override;
	bool requiresCreatureTarget() const	override;
protected:
	void applyBattleEffects(const SpellCastEnvironment * env, const BattleCast & parameters, SpellCastContext & ctx) const override;
};

class DLL_LINKAGE HypnotizeMechanics : public RegularSpellMechanics
{
public:
	HypnotizeMechanics(const CSpell * s, const CBattleInfoCallback * Cb, const Caster * caster_);
	bool isImmuneByStack(const CStack * obj) const override;
};

class DLL_LINKAGE ObstacleMechanics : public SpecialSpellMechanics
{
public:
	ObstacleMechanics(const CSpell * s, const CBattleInfoCallback * Cb, const Caster * caster_);
	bool canBeCastAt(BattleHex destination) const override;
protected:
	static bool isHexAviable(const CBattleInfoCallback * cb, const BattleHex & hex, const bool mustBeClear);
	void placeObstacle(const SpellCastEnvironment * env, const BattleCast & parameters, const BattleHex & pos) const;
	virtual void setupObstacle(SpellCreatedObstacle * obstacle) const = 0;
};

class DLL_LINKAGE PatchObstacleMechanics : public ObstacleMechanics
{
public:
	PatchObstacleMechanics(const CSpell * s, const CBattleInfoCallback * Cb, const Caster * caster_);
protected:
	void applyBattleEffects(const SpellCastEnvironment * env, const BattleCast & parameters, SpellCastContext & ctx) const override;
};

class DLL_LINKAGE LandMineMechanics : public PatchObstacleMechanics
{
public:
	LandMineMechanics(const CSpell * s, const CBattleInfoCallback * Cb, const Caster * caster_);
	bool canBeCast(Problem & problem) const override;
	bool requiresCreatureTarget() const	override;
protected:
	int defaultDamageEffect(const SpellCastEnvironment * env, const BattleCast & parameters, StacksInjured & si, const TStacks & target) const override;
	void setupObstacle(SpellCreatedObstacle * obstacle) const override;
};

class DLL_LINKAGE QuicksandMechanics : public PatchObstacleMechanics
{
public:
	QuicksandMechanics(const CSpell * s, const CBattleInfoCallback * Cb, const Caster * caster_);
	bool requiresCreatureTarget() const	override;
protected:
	void setupObstacle(SpellCreatedObstacle * obstacle) const override;
};

class DLL_LINKAGE WallMechanics : public ObstacleMechanics
{
public:
	WallMechanics(const CSpell * s, const CBattleInfoCallback * Cb, const Caster * caster_);
	std::vector<BattleHex> rangeInHexes(BattleHex centralHex, ui8 schoolLvl, bool *outDroppedHexes = nullptr) const override;
};

class DLL_LINKAGE FireWallMechanics : public WallMechanics
{
public:
	FireWallMechanics(const CSpell * s, const CBattleInfoCallback * Cb, const Caster * caster_);
	bool requiresCreatureTarget() const	override;
protected:
	void applyBattleEffects(const SpellCastEnvironment * env, const BattleCast & parameters, SpellCastContext & ctx) const override;
	void setupObstacle(SpellCreatedObstacle * obstacle) const override;
};

class DLL_LINKAGE ForceFieldMechanics : public WallMechanics
{
public:
	ForceFieldMechanics(const CSpell * s, const CBattleInfoCallback * Cb, const Caster * caster_);
	bool requiresCreatureTarget() const	override;
protected:
	void applyBattleEffects(const SpellCastEnvironment * env, const BattleCast & parameters, SpellCastContext & ctx) const override;
	void setupObstacle(SpellCreatedObstacle * obstacle) const override;
};

class DLL_LINKAGE RemoveObstacleMechanics : public SpecialSpellMechanics
{
public:
	RemoveObstacleMechanics(const CSpell * s, const CBattleInfoCallback * Cb, const Caster * caster_);
	bool canBeCast(Problem & problem) const override;
	bool canBeCastAt(BattleHex destination) const override;
	bool requiresCreatureTarget() const	override;
protected:
	void applyBattleEffects(const SpellCastEnvironment * env, const BattleCast & parameters, SpellCastContext & ctx) const override;
private:
    bool canRemove(const CObstacleInstance * obstacle, const int spellLevel) const;
};

///all rising spells
class DLL_LINKAGE RisingSpellMechanics : public HealingSpellMechanics
{
public:
	RisingSpellMechanics(const CSpell * s, const CBattleInfoCallback * Cb, const Caster * caster_);
	EHealLevel getHealLevel(int effectLevel) const override final;
	EHealPower getHealPower(int effectLevel) const override final;
};

class DLL_LINKAGE SacrificeMechanics : public RisingSpellMechanics
{
public:
	SacrificeMechanics(const CSpell * s, const CBattleInfoCallback * Cb, const Caster * caster_);
	bool canBeCast(Problem & problem) const override;
	bool requiresCreatureTarget() const	override;
protected:
	void applyBattleEffects(const SpellCastEnvironment * env, const BattleCast & parameters, SpellCastContext & ctx) const override;
	int calculateHealedHP(const SpellCastEnvironment * env, const BattleCast & parameters, SpellCastContext & ctx) const override;
};

///ANIMATE_DEAD and RESURRECTION
class DLL_LINKAGE SpecialRisingSpellMechanics : public RisingSpellMechanics
{
public:
	SpecialRisingSpellMechanics(const CSpell * s, const CBattleInfoCallback * Cb, const Caster * caster_);
	bool isImmuneByStack(const CStack * obj) const override;
	bool canBeCastAt(BattleHex destination) const override;
};

class DLL_LINKAGE TeleportMechanics : public RegularSpellMechanics
{
public:
	TeleportMechanics(const CSpell * s, const CBattleInfoCallback * Cb, const Caster * caster_);
	bool canBeCast(Problem & problem) const override;
protected:
	void applyBattleEffects(const SpellCastEnvironment * env, const BattleCast & parameters, SpellCastContext & ctx) const override;
};

} // namespace spells

