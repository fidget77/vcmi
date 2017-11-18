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
class DLL_LINKAGE SpecialSpellMechanics : public DefaultSpellMechanics
{
public:
	SpecialSpellMechanics(const IBattleCast * event);

	void applyEffects(const SpellCastEnvironment * env, const Target & targets) const override;
	void applyEffectsForced(const SpellCastEnvironment * env, const Target & targets) const override;

	bool canBeCast(Problem & problem) const override;
	bool canBeCastAt(BattleHex destination) const override;

	void beforeCast(vstd::RNG & rng, const Target & target, std::vector <const CStack*> & reflected) override;

	void cast(IBattleState * battleState, vstd::RNG & rng, const Target & target) override;

	virtual bool requiresCreatureTarget() const = 0;

	std::vector<Destination> getPossibleDestinations(size_t index, AimType aimType, const Target & current) const override;

protected:
	virtual int defaultDamageEffect(const SpellCastEnvironment * env, StacksInjured & si, const std::vector<const battle::Unit *> & target) const;

	std::vector<const CStack *> getAffectedStacks(BattleHex destination) const override final;
};

class DLL_LINKAGE ObstacleMechanics : public SpecialSpellMechanics
{
public:
	ObstacleMechanics(const IBattleCast * event);
	bool canBeCastAt(BattleHex destination) const override;
protected:
	static bool isHexAviable(const CBattleInfoCallback * cb, const BattleHex & hex, const bool mustBeClear);
	void placeObstacle(const SpellCastEnvironment * env, const BattleHex & pos) const;
	virtual void setupObstacle(SpellCreatedObstacle * obstacle) const = 0;
};

class DLL_LINKAGE PatchObstacleMechanics : public ObstacleMechanics
{
public:
	PatchObstacleMechanics(const IBattleCast * event);
protected:
	void applyCastEffects(const SpellCastEnvironment * env, const Target & target) const override;
};

class DLL_LINKAGE LandMineMechanics : public PatchObstacleMechanics
{
public:
	LandMineMechanics(const IBattleCast * event);
	bool canBeCast(Problem & problem) const override;
	bool requiresCreatureTarget() const	override;
protected:
	int defaultDamageEffect(const SpellCastEnvironment * env, StacksInjured & si, const std::vector<const battle::Unit *> & target) const override;
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
	void applyCastEffects(const SpellCastEnvironment * env, const Target & target) const override;
	void setupObstacle(SpellCreatedObstacle * obstacle) const override;
};

class DLL_LINKAGE ForceFieldMechanics : public WallMechanics
{
public:
	ForceFieldMechanics(const IBattleCast * event);
	bool requiresCreatureTarget() const	override;
protected:
	void applyCastEffects(const SpellCastEnvironment * env, const Target & target) const override;
	void setupObstacle(SpellCreatedObstacle * obstacle) const override;
};

} // namespace spells

