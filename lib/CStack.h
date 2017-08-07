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
struct BattleInfo;
class CStack;
class CHealthInfo;
class JsonSerializeFormat;

class DLL_LINKAGE IUnitBonusInfo
{
public:
	virtual const IBonusBearer * unitAsBearer() const = 0;
	virtual bool unitHasAmmoCart() const = 0; //todo: handle ammo cart with bonus system
};

class DLL_LINKAGE CAmmo
{
public:
	explicit CAmmo(const IUnitBonusInfo * Owner, CSelector totalSelector);
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
	const IUnitBonusInfo * owner;
	CBonusProxy totalProxy;
};

class DLL_LINKAGE CShots : public CAmmo
{
public:
	explicit CShots(const IUnitBonusInfo * Owner);
	CShots(const CShots & other);
	CShots & operator=(const CShots & other);
	bool isLimited() const override;
};

class DLL_LINKAGE CCasts : public CAmmo
{
public:
	explicit CCasts(const IUnitBonusInfo * Owner);
	CCasts(const CCasts & other);
	CCasts & operator=(const CCasts & other);
};

class DLL_LINKAGE CRetaliations : public CAmmo
{
public:
	explicit CRetaliations(const IUnitBonusInfo * Owner);
	CRetaliations(const CRetaliations & other);
	CRetaliations & operator=(const CRetaliations & other);
	bool isLimited() const override;
	int32_t total() const override;
	void reset() override;

	void serializeJson(JsonSerializeFormat & handler) override;
private:
	mutable int32_t totalCache;
};

class DLL_LINKAGE IUnitHealthInfo
{
public:
	virtual int32_t unitMaxHealth() const = 0;
	virtual int32_t unitBaseAmount() const = 0;
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

	void toInfo(CHealthInfo & info) const;
	void fromInfo(const CHealthInfo & info);

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

///Stack interface for Stack state
class DLL_LINKAGE IUnitInfo : public IUnitHealthInfo, public IUnitBonusInfo
{
public:
	virtual bool doubleWide() const = 0;
	virtual uint32_t unitId() const = 0;
	virtual ui8 unitSide() const = 0;
	virtual int32_t creatureIndex() const = 0;
};

/// Stack state interface for outside
/// to be used instead of CStack where possible
class DLL_LINKAGE IStackState
{
public:
	virtual const IUnitInfo * getUnitInfo() const = 0;

	virtual bool ableToRetaliate() const = 0;
	virtual bool alive() const = 0;

	virtual bool canCast() const = 0;
	virtual bool isCaster() const = 0;
	virtual bool canShoot() const = 0;
	virtual bool isShooter() const = 0;

	virtual int32_t getCount() const = 0;
	virtual int32_t getFirstHPleft() const = 0;
	virtual int32_t getKilled() const = 0;
};

///mutable part of CStack
class DLL_LINKAGE CStackState : public IStackState
{
public:
	CCasts casts;
	CRetaliations counterAttacks;
	CHealth health;
	CShots shots;

	explicit CStackState(const IUnitInfo * Owner);
	CStackState(const CStackState & other);
	CStackState(CStackState && other) = delete;

	CStackState & operator=(const CStackState & other);
	CStackState & operator=(CStackState && other) = delete;

	const IUnitInfo * getUnitInfo() const override;
	bool ableToRetaliate() const override;
	bool alive() const override;

	bool canCast() const override;
	bool isCaster() const override;
	bool canShoot() const override;
	bool isShooter() const override;

	int32_t getKilled() const override;
	int32_t getCount() const override;
	int32_t getFirstHPleft() const override;

	void localInit();
	void serializeJson(JsonSerializeFormat & handler);
	void swap(CStackState & other);

private:
	const IUnitInfo * owner;

	void reset();
};

class DLL_LINKAGE CStackStateTransfer
{
public:
	CStackStateTransfer();
	~CStackStateTransfer();

	void pack(uint32_t id, CStackState & state);
	void unpack(BattleInfo * battle);

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & stackId;
		h & data;
	}
private:
	uint32_t stackId;
	JsonNode data;
};

class DLL_LINKAGE CStack : public CBonusSystemNode, public spells::Caster, public IUnitInfo, public IStackState
{
public:
	const CStackInstance * base; //garrison slot from which stack originates (nullptr for war machines, summoned cres, etc)

	ui32 ID; //unique ID of stack
	const CCreature * type;
	ui32 baseAmount;

	PlayerColor owner; //owner - player color (255 for neutrals)
	SlotID slot;  //slot - position in garrison (may be 255 for neutrals/called creatures)
	ui8 side;

	CStackState stackState;

	BattleHex position; //position on battlefield; -2 - keep, -3 - lower tower, -4 - upper tower

	///id of alive clone of this stack clone if any
	si32 cloneID;
	std::set<EBattleStackState::EBattleStackState> state;

	CStack(const CStackInstance * base, PlayerColor O, int I, ui8 Side, SlotID S);
	CStack(const CStackBasicDescriptor * stack, PlayerColor O, int I, ui8 Side, SlotID S = SlotID(255));
	CStack();
	~CStack();

	const CCreature * getCreature() const;

	std::string nodeName() const override;

	void localInit(BattleInfo * battleInfo);
	std::string getName() const; //plural or singular
	bool willMove(int turn = 0) const; //if stack has remaining move this turn

	bool moved(int turn = 0) const; //if stack was already moved this turn
	bool waited(int turn = 0) const;

	bool canMove(int turn = 0) const; //if stack can move

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

	CHealth healthAfterAttacked(int32_t & damage, const CHealth & customHealth) const;

	CHealth healthAfterHealed(int32_t & toHeal, EHealLevel level, EHealPower power) const;

	void prepareAttacked(BattleStackAttacked & bsa, CRandomGenerator & rand) const; //requires bsa.damageAmout filled
	void prepareAttacked(BattleStackAttacked & bsa, CRandomGenerator & rand, const CHealth & customHealth) const; //requires bsa.damageAmout filled

	///spells::Caster

	ui8 getSpellSchoolLevel(const spells::Mode mode, const CSpell * spell, int * outSelectedSchool = nullptr) const override;
	///default spell school level for effect calculation
	int getEffectLevel(const spells::Mode mode, const CSpell * spell) const override;

	ui32 getSpellBonus(const CSpell * spell, ui32 base, const CStack * affectedStack) const override;
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

	virtual int32_t creatureIndex() const override;

	int32_t unitMaxHealth() const override;
	int32_t unitBaseAmount() const override;

	const IBonusBearer * unitAsBearer() const override;
	bool unitHasAmmoCart() const override;

	bool doubleWide() const override;
	uint32_t unitId() const override;
	ui8 unitSide() const override;

	///IStackState

	const IUnitInfo * getUnitInfo() const override;

	bool ableToRetaliate() const override;
	bool alive() const override;

	bool canCast() const override;
	bool isCaster() const override;

	bool canShoot() const override;
	bool isShooter() const override;

	int32_t getKilled() const override;
	int32_t getCount() const override;
	int32_t getFirstHPleft() const override;

	///MetaStrings

	void addText(MetaString & text, ui8 type, int32_t serial, const boost::logic::tribool & plural = boost::logic::indeterminate) const;
	void addNameReplacement(MetaString & text, const boost::logic::tribool & plural = boost::logic::indeterminate) const;
	std::string formatGeneralMessage(const int32_t baseTextId) const;

	///Non const API for NetPacks

	///stack will be ghost in next battle state update
	void makeGhost();
	void setHealth(const CHealthInfo & value);
	void setHealth(const CHealth & value);

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
		h & position;
		h & state;

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

	bool isClone() const;
	bool isDead() const;
	bool isGhost() const; //determines if stack was removed
	bool isValidTarget(bool allowDead = false) const; //non-turret non-ghost stacks (can be attacked or be object of magic effect)
	bool isTurret() const;

private:
	const BattleInfo * battle; //do not serialize
};
