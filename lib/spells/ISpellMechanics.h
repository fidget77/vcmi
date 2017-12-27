/*
 * ISpellMechanics.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "CSpellHandler.h"
#include "../battle/BattleHex.h"

struct Query;
class IBattleState;

namespace spells
{
class TargetCondition;
}

///callback to be provided by server
class DLL_LINKAGE SpellCastEnvironment : public spells::PacketSender
{
public:
	virtual ~SpellCastEnvironment(){};

	virtual CRandomGenerator & getRandomGenerator() const = 0;

	virtual const CMap * getMap() const = 0;
	virtual const CGameInfoCallback * getCb() const = 0;

	virtual bool moveHero(ObjectInstanceID hid, int3 dst, bool teleporting) const = 0;	//TODO: remove

	virtual void genericQuery(Query * request, PlayerColor color, std::function<void(const JsonNode &)> callback) const = 0;//TODO: type safety on query, use generic query packet when implemented
};

namespace spells
{

///Single spell destination.
class DLL_LINKAGE Destination
{
public:
	Destination();
	~Destination() = default;
	explicit Destination(const CStack * destination);
	explicit Destination(const BattleHex & destination);

	Destination(const Destination & other);

	Destination & operator=(const Destination & other);

	const CStack * stackValue;
	BattleHex hexValue;
};

///all parameters of particular cast event
//TODO:
class DLL_LINKAGE IBattleCast
{
public:
	virtual ~IBattleCast();
};

class DLL_LINKAGE BattleCast : public IBattleCast
{
public:
	//normal constructor
	BattleCast(const CBattleInfoCallback * cb, const Caster * caster_, const Mode mode_, const CSpell * spell_);

	//magic mirror constructor
	BattleCast(const BattleCast & orig, const Caster * caster_);

	virtual ~BattleCast();

	void aimToHex(const BattleHex & destination);
	void aimToStack(const CStack * destination);

	///only apply effects to specified targets
	void applyEffects(const SpellCastEnvironment * env) const;

	///only apply effects to specified targets and ignore immunity
	void applyEffectsForced(const SpellCastEnvironment * env) const;

	///normal cast
	void cast(const SpellCastEnvironment * env);

	///cast evaluation
	void cast(IBattleState * battleState);

	///cast with silent check for permitted cast
	bool castIfPossible(const SpellCastEnvironment * env);

	BattleHex getFirstDestinationHex() const;

	int getEffectValue() const;

	const CSpell * spell;
	const CBattleInfoCallback * cb;
	const Caster * caster;

	Target target;
	Mode mode;

	///spell school level
	int spellLvl;
	///spell school level to use for effects
	int effectLevel;
	///actual spell-power affecting effect values
	int effectPower;
	///actual spell-power affecting effect duration
	int effectDuration;

private:
	///for Archangel-like casting
	int effectValue;
};

class DLL_LINKAGE ISpellMechanicsFactory
{
public:
	virtual ~ISpellMechanicsFactory();

	virtual std::unique_ptr<Mechanics> create(const CBattleInfoCallback * cb, Mode mode, const Caster * caster) const = 0;

	static std::unique_ptr<ISpellMechanicsFactory> get(const CSpell * s);

protected:
	const CSpell * spell;

	ISpellMechanicsFactory(const CSpell * s);
};

class SpellCastContext;

class DLL_LINKAGE Mechanics
{
public:
	Mechanics(const CSpell * s, const CBattleInfoCallback * Cb, const Caster * caster_);
	virtual ~Mechanics();

	virtual bool adaptProblem(ESpellCastProblem::ESpellCastProblem source, Problem & target) const = 0;
	virtual bool adaptGenericProblem(Problem & target) const = 0;

	virtual std::vector<BattleHex> rangeInHexes(BattleHex centralHex, ui8 schoolLvl, bool * outDroppedHexes = nullptr) const = 0;
	virtual std::vector<const CStack *> getAffectedStacks(int spellLvl, BattleHex destination) const = 0;

	virtual bool canBeCast(Problem & problem) const = 0;
	virtual bool canBeCastAt(BattleHex destination) const = 0;

	virtual void applyEffects(const SpellCastEnvironment * env, const BattleCast & parameters) const = 0;
	virtual void applyEffectsForced(const SpellCastEnvironment * env, const BattleCast & parameters) const = 0;

	virtual void cast(const SpellCastEnvironment * env, const BattleCast & parameters, SpellCastContext & ctx, std::vector <const CStack*> & reflected) const = 0;
	virtual void cast(IBattleState * battleState, const BattleCast & parameters) const = 0;

	virtual bool isReceptive(const IStackState * target) const = 0;

	Mode mode;
	const CSpell * owner;
	const CBattleInfoCallback * cb;
	const Caster * caster;

	const CStack * casterStack; //deprecated

	ui8 casterSide;
};

class DLL_LINKAGE BaseMechanics : public Mechanics
{
public:
	std::shared_ptr<TargetCondition> targetCondition;

	BaseMechanics(const CSpell * s, const CBattleInfoCallback * Cb, const Caster * caster_);
	virtual ~BaseMechanics();

	bool adaptProblem(ESpellCastProblem::ESpellCastProblem source, Problem & target) const override;
	bool adaptGenericProblem(Problem & target) const override;
	bool isReceptive(const IStackState * target) const override;
};

}// namespace spells

class DLL_LINKAGE AdventureSpellCastParameters
{
public:
	const CGHeroInstance * caster;
	int3 pos;
};

class DLL_LINKAGE IAdventureSpellMechanics
{
public:
	IAdventureSpellMechanics(const CSpell * s);
	virtual ~IAdventureSpellMechanics() = default;

	virtual bool adventureCast(const SpellCastEnvironment * env, const AdventureSpellCastParameters & parameters) const = 0;

	static std::unique_ptr<IAdventureSpellMechanics> createMechanics(const CSpell * s);
protected:
	const CSpell * owner;
};
