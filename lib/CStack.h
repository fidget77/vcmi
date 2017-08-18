/*
 * CStack.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once
#include "JsonNode.h"
#include "HeroBonus.h"
#include "CCreatureHandler.h" //todo: remove
#include "battle/BattleHex.h"
#include "mapObjects/CGHeroInstance.h" // for commander serialization

struct BattleStackAttacked;
class BattleInfo;
class CStack;
class CStackState;
class CStackStateInfo;
class JsonSerializeFormat;

class DLL_LINKAGE IUnitBonus
{
public:
	virtual const IBonusBearer * unitAsBearer() const = 0;
	virtual bool unitHasAmmoCart() const = 0; //todo: handle ammo cart with bonus system
};

class DLL_LINKAGE IUnitHealthInfo
{
public:
	virtual int32_t unitMaxHealth() const = 0;
	virtual int32_t unitBaseAmount() const = 0;
};

///Stack interface for Stack state
class DLL_LINKAGE IUnitInfo : public IUnitHealthInfo
{
public:
	virtual bool doubleWide() const = 0;
	virtual uint32_t unitId() const = 0;
	virtual ui8 unitSide() const = 0;
	virtual int32_t creatureIndex() const = 0;
	virtual CreatureID creatureId() const = 0;
	virtual int32_t creatureLevel() const = 0;
	virtual SlotID unitSlot() const = 0;
};

class DLL_LINKAGE CAmmo
{
public:
	explicit CAmmo(const IUnitBonus * Owner, CSelector totalSelector);
	explicit CAmmo(const CAmmo & other, CSelector totalSelector);

	//bonus-related stuff if not copyable
	CAmmo(const CAmmo & other) = delete;

	//ammo itself if not movable
	CAmmo(CAmmo && other) = delete;

	int32_t available() const;
	bool canUse(int32_t amount = 1) const;
	virtual bool isLimited() const;
	virtual void reset();
	virtual int32_t total() const;
	virtual void use(int32_t amount = 1);

	virtual void serializeJson(JsonSerializeFormat & handler);
protected:
	int32_t used;
	const IUnitBonus * owner;
	CBonusProxy totalProxy;
};

class DLL_LINKAGE CShots : public CAmmo
{
public:
	explicit CShots(const IUnitBonus * Owner);
	CShots(const CShots & other);
	CShots & operator=(const CShots & other);
	bool isLimited() const override;
};

class DLL_LINKAGE CCasts : public CAmmo
{
public:
	explicit CCasts(const IUnitBonus * Owner);
	CCasts(const CCasts & other);
	CCasts & operator=(const CCasts & other);
};

class DLL_LINKAGE CRetaliations : public CAmmo
{
public:
	explicit CRetaliations(const IUnitBonus * Owner);
	CRetaliations(const CRetaliations & other);
	CRetaliations & operator=(const CRetaliations & other);
	bool isLimited() const override;
	int32_t total() const override;
	void reset() override;

	void serializeJson(JsonSerializeFormat & handler) override;
private:
	mutable int32_t totalCache;
};

class DLL_LINKAGE CHealth
{
public:
	explicit CHealth(const IUnitHealthInfo * Owner);
	CHealth(const CHealth & other);

	CHealth & operator=(const CHealth & other);

	void init();
	void reset();

	void damage(int32_t & amount);
	void heal(int32_t & amount, EHealLevel level, EHealPower power);

	int32_t getCount() const;
	int32_t getFirstHPleft() const;
	int32_t getResurrected() const;

	int64_t available() const;
	int64_t total() const;

	void takeResurrected();

	void serializeJson(JsonSerializeFormat & handler);
private:
	void addResurrected(int32_t amount);
	void setFromTotal(const int64_t totalHealth);
	const IUnitHealthInfo * owner;

	int32_t firstHPleft;
	int32_t fullUnits;
	int32_t resurrected;
};


/// Stack state interface for outside
/// to be used instead of CStack where possible
class DLL_LINKAGE IStackState : public IUnitInfo, public IUnitBonus
{
public:
	virtual bool ableToRetaliate() const = 0;
	virtual bool alive() const = 0;
	virtual bool isGhost() const = 0;

	bool isDead() const;
	bool isTurret() const;
	bool isValidTarget(bool allowDead = false) const; //non-turret non-ghost stacks (can be attacked or be object of magic effect)

	virtual bool isClone() const = 0;
	virtual bool hasClone() const = 0;

	virtual bool canCast() const = 0;
	virtual bool isCaster() const = 0;
	virtual bool canShoot() const = 0;
	virtual bool isShooter() const = 0;

	virtual int32_t getCount() const = 0;
	virtual int32_t getFirstHPleft() const = 0;
	virtual int32_t getKilled() const = 0;
	virtual int64_t getAvailableHealth() const = 0;
	virtual int64_t getTotalHealth() const = 0;

	virtual BattleHex getPosition() const = 0;

	virtual bool canMove(int turn = 0) const = 0; //if stack can move
	virtual bool moved(int turn = 0) const = 0; //if stack was already moved this turn
	virtual bool willMove(int turn = 0) const = 0; //if stack has remaining move this turn
	virtual bool waited(int turn = 0) const = 0;

	virtual CStackState asquire() const = 0;
};

///mutable part of CStack
class DLL_LINKAGE CStackState : public IStackState
{
public:
	bool cloned;
	bool defending;
	bool defendingAnim;
	bool drainedMana;
	bool fear;
	bool hadMorale;
	bool ghost;
	bool ghostPending;
	bool movedThisTurn;
	bool summoned;
	bool waiting;

	CCasts casts;
	CRetaliations counterAttacks;
	CHealth health;
	CShots shots;

	///id of alive clone of this stack clone if any
	si32 cloneID;

	///position on battlefield; -2 - keep, -3 - lower tower, -4 - upper tower
	BattleHex position;

	explicit CStackState(const IUnitInfo * unit_, const IUnitBonus * bonus_);
	CStackState(const CStackState & other);

	CStackState & operator=(const CStackState & other);

	bool ableToRetaliate() const override;
	bool alive() const override;
	bool isGhost() const override;

	bool isClone() const override;
	bool hasClone() const override;

	bool canCast() const override;
	bool isCaster() const override;
	bool canShoot() const override;
	bool isShooter() const override;

	int32_t getKilled() const override;
	int32_t getCount() const override;
	int32_t getFirstHPleft() const override;
	int64_t getAvailableHealth() const override;
	int64_t getTotalHealth() const override;

	BattleHex getPosition() const override;

	bool canMove(int turn = 0) const override;
	bool moved(int turn = 0) const override;
	bool willMove(int turn = 0) const override;
	bool waited(int turn = 0) const override;

	bool doubleWide() const override;
	uint32_t unitId() const override;
	ui8 unitSide() const override;
	int32_t creatureIndex() const override;
	CreatureID creatureId() const override;
	int32_t creatureLevel() const override;
	SlotID unitSlot() const override;

	int32_t unitMaxHealth() const override;
	int32_t unitBaseAmount() const override;

	const IBonusBearer * unitAsBearer() const override;
	bool unitHasAmmoCart() const override;

	CStackState asquire() const	override;

	void damage(int32_t & amount);
	void heal(int32_t & amount, EHealLevel level, EHealPower power);

	void localInit();
	void serializeJson(JsonSerializeFormat & handler);
	void swap(CStackState & other);

	void toInfo(CStackStateInfo & info);
	void fromInfo(const CStackStateInfo & info);

	const IUnitInfo * getUnitInfo() const;

private:
	const IUnitInfo * unit;
	const IUnitBonus * bonus;

	void reset();
};

class DLL_LINKAGE CStack : public CBonusSystemNode, public spells::Caster, public IStackState
{
public:
	const CStackInstance * base; //garrison slot from which stack originates (nullptr for war machines, summoned cres, etc)

	ui32 ID; //unique ID of stack
	const CCreature * type;
	ui32 baseAmount;

	PlayerColor owner; //owner - player color (255 for neutrals)
	SlotID slot;  //slot - position in garrison (may be 255 for neutrals/called creatures)
	ui8 side;
	BattleHex initialPosition; //position on battlefield; -2 - keep, -3 - lower tower, -4 - upper tower

	CStackState stackState;

	CStack(const CStackInstance * base, PlayerColor O, int I, ui8 Side, SlotID S);
	CStack(const CStackBasicDescriptor * stack, PlayerColor O, int I, ui8 Side, SlotID S = SlotID(255));
	CStack();
	~CStack();

	const CCreature * getCreature() const;

	std::string nodeName() const override;

	void localInit(BattleInfo * battleInfo);
	std::string getName() const; //plural or singular

	bool canMove(int turn = 0) const override;
	bool moved(int turn = 0) const override;
	bool willMove(int turn = 0) const override;
	bool waited(int turn = 0) const override;

	bool canBeHealed() const; //for first aid tent - only harmed stacks that are not war machines

	ui32 level() const;
	si32 magicResistance() const override; //include aura of resistance
	std::vector<si32> activeSpells() const; //returns vector of active spell IDs sorted by time of cast
	const CGHeroInstance * getMyHero() const; //if stack belongs to hero (directly or was by him summoned) returns hero, nullptr otherwise

	static bool isMeleeAttackPossible(const CStack * attacker, const CStack * defender, BattleHex attackerPos = BattleHex::INVALID, BattleHex defenderPos = BattleHex::INVALID);

	BattleHex occupiedHex() const; //returns number of occupied hex (not the position) if stack is double wide; otherwise -1
	BattleHex occupiedHex(BattleHex assumedPos) const; //returns number of occupied hex (not the position) if stack is double wide and would stand on assumedPos; otherwise -1
	std::vector<BattleHex> getHexes() const; //up to two occupied hexes, starting from front
	std::vector<BattleHex> getHexes(BattleHex assumedPos) const; //up to two occupied hexes, starting from front
	static std::vector<BattleHex> getHexes(BattleHex assumedPos, bool twoHex, ui8 side); //up to two occupied hexes, starting from front
	bool coversPos(BattleHex position) const; //checks also if unit is double-wide
	std::vector<BattleHex> getSurroundingHexes(BattleHex attackerPos = BattleHex::INVALID) const; // get six or 8 surrounding hexes depending on creature size

	BattleHex::EDir destShiftDir() const;

	void prepareAttacked(BattleStackAttacked & bsa, CRandomGenerator & rand) const; //requires bsa.damageAmout filled
	static void prepareAttacked(BattleStackAttacked & bsa, CRandomGenerator & rand, const CStackState & customState); //requires bsa.damageAmout filled

	///spells::Caster

	ui8 getSpellSchoolLevel(const spells::Mode mode, const CSpell * spell, int * outSelectedSchool = nullptr) const override;
	///default spell school level for effect calculation
	int getEffectLevel(const spells::Mode mode, const CSpell * spell) const override;

	ui32 getSpellBonus(const CSpell * spell, ui32 base, const IStackState * affectedStack) const override;
	ui32 getSpecificSpellBonus(const CSpell * spell, ui32 base) const override;

	///default spell-power for damage/heal calculation
	int getEffectPower(const spells::Mode mode, const CSpell * spell) const override;

	///default spell-power for timed effects duration
	int getEnchantPower(const spells::Mode mode, const CSpell * spell) const override;

	///damage/heal override(ignores spell configuration, effect level and effect power)
	int getEffectValue(const spells::Mode mode, const CSpell * spell) const override;

	const PlayerColor getOwner() const override;
	void getCasterName(MetaString & text) const override;
	void getCastDescription(const CSpell * spell, MetaString & text) const override;
	void getCastDescription(const CSpell * spell, const std::vector<const CStack *> & attacked, MetaString & text) const override;
	void spendMana(const spells::Mode mode, const CSpell * spell, const spells::PacketSender * server, const int spellCost) const override;

	///IUnitInfo

	int32_t creatureIndex() const override;
	CreatureID creatureId() const override;
	int32_t creatureLevel() const override;

	int32_t unitMaxHealth() const override;
	int32_t unitBaseAmount() const override;

	const IBonusBearer * unitAsBearer() const override;
	bool unitHasAmmoCart() const override;

	bool doubleWide() const override;
	uint32_t unitId() const override;
	ui8 unitSide() const override;
	SlotID unitSlot() const override;

	///IStackState

	bool ableToRetaliate() const override;
	bool alive() const override;
	bool isGhost() const override;

	bool isClone() const override;
	bool hasClone() const override;

	bool canCast() const override;
	bool isCaster() const override;

	bool canShoot() const override;
	bool isShooter() const override;

	int32_t getKilled() const override;
	int32_t getCount() const override;
	int32_t getFirstHPleft() const override;
	int64_t getAvailableHealth() const override;
	int64_t getTotalHealth() const override;

	BattleHex getPosition() const override;
	CStackState asquire() const	override;

	///MetaStrings

	void addText(MetaString & text, ui8 type, int32_t serial, const boost::logic::tribool & plural = boost::logic::indeterminate) const;
	void addNameReplacement(MetaString & text, const boost::logic::tribool & plural = boost::logic::indeterminate) const;
	std::string formatGeneralMessage(const int32_t baseTextId) const;

	///Non const API for NetPacks

	///stack will be ghost in next battle state update
	void makeGhost();

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		//this assumes that stack objects is newly created
		//stackState is not serialized here
		assert(isIndependentNode());
		h & static_cast<CBonusSystemNode&>(*this);
		h & type;
		h & ID;
		h & baseAmount;
		h & owner;
		h & slot;
		h & side;
		h & initialPosition;

		const CArmedInstance * army = (base ? base->armyObj : nullptr);
		SlotID extSlot = (base ? base->armyObj->findStack(base) : SlotID());

		if(h.saving)
		{
			h & army;
			h & extSlot;
		}
		else
		{
			h & army;
			h & extSlot;

			if(extSlot == SlotID::COMMANDER_SLOT_PLACEHOLDER)
			{
				auto hero = dynamic_cast<const CGHeroInstance *>(army);
				assert(hero);
				base = hero->commander;
			}
			else if(slot == SlotID::SUMMONED_SLOT_PLACEHOLDER || slot == SlotID::ARROW_TOWERS_SLOT || slot == SlotID::WAR_MACHINES_SLOT)
			{
				//no external slot possible, so no base stack
				base = nullptr;
			}
			else if(!army || extSlot == SlotID() || !army->hasStackAtSlot(extSlot))
			{
				base = nullptr;
				logGlobal->warn("%s doesn't have a base stack!", type->nameSing);
			}
			else
			{
				base = &army->getStack(extSlot);
			}
		}
	}

private:
	const BattleInfo * battle; //do not serialize
};
