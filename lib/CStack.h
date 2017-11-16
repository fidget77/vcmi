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

#include "battle/CUnitState.h"

struct BattleStackAttacked;
class BattleInfo;

class DLL_LINKAGE CStack : public CBonusSystemNode, public spells::Caster, public battle::Unit, public battle::IUnitEnvironment
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

	battle::CUnitState stackState;

	CStack(const CStackInstance * base, PlayerColor O, int I, ui8 Side, SlotID S);
	CStack(const CStackBasicDescriptor * stack, PlayerColor O, int I, ui8 Side, SlotID S = SlotID(255));
	CStack();
	~CStack();

	const CCreature * getCreature() const; //deprecated

	std::string nodeName() const override;

	void localInit(BattleInfo * battleInfo);
	std::string getName() const; //plural or singular

	bool canMove(int turn = 0) const override;
	bool defended(int turn = 0) const override;
	bool moved(int turn = 0) const override;
	bool willMove(int turn = 0) const override;
	bool waited(int turn = 0) const override;

	bool canBeHealed() const; //for first aid tent - only harmed stacks that are not war machines

	ui32 level() const;
	si32 magicResistance() const override; //include aura of resistance
	std::vector<si32> activeSpells() const; //returns vector of active spell IDs sorted by time of cast
	const CGHeroInstance * getMyHero() const; //if stack belongs to hero (directly or was by him summoned) returns hero, nullptr otherwise

	static bool isMeleeAttackPossible(const battle::Unit * attacker, const battle::Unit * defender, BattleHex attackerPos = BattleHex::INVALID, BattleHex defenderPos = BattleHex::INVALID);

	BattleHex occupiedHex() const; //returns number of occupied hex (not the position) if stack is double wide; otherwise -1
	BattleHex occupiedHex(BattleHex assumedPos) const; //returns number of occupied hex (not the position) if stack is double wide and would stand on assumedPos; otherwise -1
	std::vector<BattleHex> getHexes() const; //up to two occupied hexes, starting from front
	std::vector<BattleHex> getHexes(BattleHex assumedPos) const; //up to two occupied hexes, starting from front

	BattleHex::EDir destShiftDir() const;

	void prepareAttacked(BattleStackAttacked & bsa, vstd::RNG & rand) const; //requires bsa.damageAmout filled
	static void prepareAttacked(BattleStackAttacked & bsa, vstd::RNG & rand, std::shared_ptr<battle::CUnitState> customState); //requires bsa.damageAmout filled

	///spells::Caster

	ui8 getSpellSchoolLevel(const spells::Mode mode, const CSpell * spell, int * outSelectedSchool = nullptr) const override;
	///default spell school level for effect calculation
	int getEffectLevel(const spells::Mode mode, const CSpell * spell) const override;

	int64_t getSpellBonus(const CSpell * spell, int64_t base, const battle::Unit * affectedStack) const override;
	int64_t getSpecificSpellBonus(const CSpell * spell, int64_t base) const override;

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

	const CCreature * creatureType() const override;

	int32_t unitMaxHealth() const override;
	int32_t unitBaseAmount() const override;

	bool unitHasAmmoCart() const override;

	uint32_t unitId() const override;
	ui8 unitSide() const override;
	PlayerColor unitOwner() const override;
	SlotID unitSlot() const override;

	///battle::Unit

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
	std::shared_ptr<battle::CUnitState> asquire() const	override;

	int battleQueuePhase(int turn) const override;
	std::string getDescription() const override;
	int32_t getInitiative(int turn = 0) const override;


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
