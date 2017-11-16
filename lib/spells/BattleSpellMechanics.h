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
	HealingSpellMechanics(const IBattleCast * event);
protected:
	void applyBattleEffects(const SpellCastEnvironment * env, const BattleCast & parameters, SpellCastContext & ctx) const override;
	virtual int calculateHealedHP(const SpellCastEnvironment * env, const BattleCast & parameters, SpellCastContext & ctx) const;
	virtual EHealLevel getHealLevel(int effectLevel) const = 0;
	virtual EHealPower getHealPower(int effectLevel) const = 0;
private:
	static bool cureSelector(const Bonus * b);
};

class DLL_LINKAGE ChainLightningMechanics : public RegularSpellMechanics
{
public:
	ChainLightningMechanics(const IBattleCast * event);
protected:
	void applyBattleEffects(const SpellCastEnvironment * env, const BattleCast & parameters, SpellCastContext & ctx) const override;
	std::vector<const CStack *> calculateAffectedStacks(BattleHex destination) const override;
};

class DLL_LINKAGE DispellMechanics : public RegularSpellMechanics
{
public:
	DispellMechanics(const IBattleCast * event);
	bool isImmuneByStack(const battle::Unit * obj) const override;
protected:
	void applyBattleEffects(const SpellCastEnvironment * env, const BattleCast & parameters, SpellCastContext & ctx) const override;
};

class DLL_LINKAGE EarthquakeMechanics : public SpecialSpellMechanics
{
public:
	EarthquakeMechanics(const IBattleCast * event);
	bool canBeCast(Problem & problem) const override;
	bool requiresCreatureTarget() const	override;
protected:
	void applyBattleEffects(const SpellCastEnvironment * env, const BattleCast & parameters, SpellCastContext & ctx) const override;
};

class DLL_LINKAGE ObstacleMechanics : public SpecialSpellMechanics
{
public:
	ObstacleMechanics(const IBattleCast * event);
	bool canBeCastAt(BattleHex destination) const override;
protected:
	static bool isHexAviable(const CBattleInfoCallback * cb, const BattleHex & hex, const bool mustBeClear);
	void placeObstacle(const SpellCastEnvironment * env, const BattleCast & parameters, const BattleHex & pos) const;
	virtual void setupObstacle(SpellCreatedObstacle * obstacle) const = 0;
};

class DLL_LINKAGE PatchObstacleMechanics : public ObstacleMechanics
{
public:
	PatchObstacleMechanics(const IBattleCast * event);
protected:
	void applyBattleEffects(const SpellCastEnvironment * env, const BattleCast & parameters, SpellCastContext & ctx) const override;
};

class DLL_LINKAGE LandMineMechanics : public PatchObstacleMechanics
{
public:
	LandMineMechanics(const IBattleCast * event);
	bool canBeCast(Problem & problem) const override;
	bool requiresCreatureTarget() const	override;
protected:
	int defaultDamageEffect(const SpellCastEnvironment * env, const BattleCast & parameters, StacksInjured & si, const std::vector<const battle::Unit *> & target) const override;
	void setupObstacle(SpellCreatedObstacle * obstacle) const override;
};

class DLL_LINKAGE QuicksandMechanics : public PatchObstacleMechanics
{
public:
	QuicksandMechanics(const IBattleCast * event);
	bool requiresCreatureTarget() const	override;
protected:
	void setupObstacle(SpellCreatedObstacle * obstacle) const override;
};

class DLL_LINKAGE WallMechanics : public ObstacleMechanics
{
public:
	WallMechanics(const IBattleCast * event);
	std::vector<BattleHex> rangeInHexes(BattleHex centralHex, bool *outDroppedHexes = nullptr) const override;
};

class DLL_LINKAGE FireWallMechanics : public WallMechanics
{
public:
	FireWallMechanics(const IBattleCast * event);
	bool requiresCreatureTarget() const	override;
protected:
	void applyBattleEffects(const SpellCastEnvironment * env, const BattleCast & parameters, SpellCastContext & ctx) const override;
	void setupObstacle(SpellCreatedObstacle * obstacle) const override;
};

class DLL_LINKAGE ForceFieldMechanics : public WallMechanics
{
public:
	ForceFieldMechanics(const IBattleCast * event);
	bool requiresCreatureTarget() const	override;
protected:
	void applyBattleEffects(const SpellCastEnvironment * env, const BattleCast & parameters, SpellCastContext & ctx) const override;
	void setupObstacle(SpellCreatedObstacle * obstacle) const override;
};

class DLL_LINKAGE RemoveObstacleMechanics : public SpecialSpellMechanics
{
public:
	RemoveObstacleMechanics(const IBattleCast * event);
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
	RisingSpellMechanics(const IBattleCast * event);
	EHealLevel getHealLevel(int effectLevel) const override final;
	EHealPower getHealPower(int effectLevel) const override final;
};

class DLL_LINKAGE SacrificeMechanics : public RisingSpellMechanics
{
public:
	SacrificeMechanics(const IBattleCast * event);
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
	SpecialRisingSpellMechanics(const IBattleCast * event);
	bool isImmuneByStack(const battle::Unit * obj) const override;
	bool canBeCastAt(BattleHex destination) const override;
};

} // namespace spells

