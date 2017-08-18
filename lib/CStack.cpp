/*
 * CStack.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CStack.h"
#include "CGeneralTextHandler.h"
#include "battle/BattleInfo.h"
#include "spells/CSpellHandler.h"
#include "CRandomGenerator.h"
#include "NetPacks.h"

#include "serializer/JsonDeserializer.h"
#include "serializer/JsonSerializer.h"

///CAmmo
CAmmo::CAmmo(const IUnitBonus * Owner, CSelector totalSelector)
	: used(0),
	owner(Owner),
	totalProxy(Owner->unitAsBearer(), totalSelector)
{
	reset();
}

CAmmo::CAmmo(const CAmmo & other, CSelector totalSelector)
	: used(other.used),
	owner(other.owner),
	totalProxy(owner->unitAsBearer(), totalSelector)
{

}

int32_t CAmmo::available() const
{
	return total() - used;
}

bool CAmmo::canUse(int32_t amount) const
{
	return !isLimited() || (available() - amount >= 0);
}

bool CAmmo::isLimited() const
{
	return true;
}

void CAmmo::reset()
{
	used = 0;
}

int32_t CAmmo::total() const
{
	return totalProxy->totalValue();
}

void CAmmo::use(int32_t amount)
{
	if(!isLimited())
		return;

	if(available() - amount < 0)
	{
		logGlobal->error("Stack ammo overuse");
		used += available();
	}
	else
		used += amount;
}

void CAmmo::serializeJson(JsonSerializeFormat & handler)
{
	handler.serializeInt("used", used, 0);
}

///CShots
CShots::CShots(const IUnitBonus * Owner)
	: CAmmo(Owner, Selector::type(Bonus::SHOTS))
{
}

CShots::CShots(const CShots & other)
	: CAmmo(other, Selector::type(Bonus::SHOTS))
{
}

CShots & CShots::operator=(const CShots & other)
{
	//do not change owner
	used = other.used;
	totalProxy = std::move(CBonusProxy(owner->unitAsBearer(), Selector::type(Bonus::SHOTS)));
	return *this;
}

bool CShots::isLimited() const
{
	return !owner->unitHasAmmoCart();
}

///CCasts
CCasts::CCasts(const IUnitBonus * Owner):
	CAmmo(Owner, Selector::type(Bonus::CASTS))
{
}

CCasts::CCasts(const CCasts & other)
	: CAmmo(other, Selector::type(Bonus::CASTS))
{
}

CCasts & CCasts::operator=(const CCasts & other)
{
	//do not change owner
	used = other.used;
	totalProxy = CBonusProxy(owner->unitAsBearer(), Selector::type(Bonus::CASTS));
	return *this;
}

///CRetaliations
CRetaliations::CRetaliations(const IUnitBonus * Owner)
	: CAmmo(Owner, Selector::type(Bonus::ADDITIONAL_RETALIATION)),
	totalCache(0)
{
}

CRetaliations::CRetaliations(const CRetaliations & other)
	: CAmmo(other, Selector::type(Bonus::ADDITIONAL_RETALIATION)),
	totalCache(other.totalCache)
{
}

CRetaliations & CRetaliations::operator=(const CRetaliations & other)
{
	//do not change owner
	used = other.used;
	totalCache = other.totalCache;
	totalProxy = CBonusProxy(owner->unitAsBearer(), Selector::type(Bonus::ADDITIONAL_RETALIATION));
	return *this;
}

bool CRetaliations::isLimited() const
{
	return !owner->unitAsBearer()->hasBonusOfType(Bonus::UNLIMITED_RETALIATIONS);
}

int32_t CRetaliations::total() const
{
	//after dispell bonus should remain during current round
	int32_t val = 1 + totalProxy->totalValue();
	vstd::amax(totalCache, val);
	return totalCache;
}

void CRetaliations::reset()
{
	CAmmo::reset();
	totalCache = 0;
}

void CRetaliations::serializeJson(JsonSerializeFormat & handler)
{
	CAmmo::serializeJson(handler);
	//we may be serialized in the middle of turn
	handler.serializeInt("totalCache", totalCache, 0);
}

///CHealth
CHealth::CHealth(const IUnitHealthInfo * Owner):
	owner(Owner)
{
	reset();
}

CHealth::CHealth(const CHealth & other):
	owner(other.owner),
	firstHPleft(other.firstHPleft),
	fullUnits(other.fullUnits),
	resurrected(other.resurrected)
{

}

CHealth & CHealth::operator=(const CHealth & other)
{
	//do not change owner
	firstHPleft = other.firstHPleft;
	fullUnits = other.fullUnits;
	resurrected = other.resurrected;
	return *this;
}

void CHealth::init()
{
	reset();
	fullUnits = owner->unitBaseAmount() > 1 ? owner->unitBaseAmount() - 1 : 0;
	firstHPleft = owner->unitBaseAmount() > 0 ? owner->unitMaxHealth() : 0;
}

void CHealth::addResurrected(int32_t amount)
{
	resurrected += amount;
	vstd::amax(resurrected, 0);
}

int64_t CHealth::available() const
{
	return static_cast<int64_t>(firstHPleft) + owner->unitMaxHealth() * fullUnits;
}

int64_t CHealth::total() const
{
	return static_cast<int64_t>(owner->unitMaxHealth()) * owner->unitBaseAmount();
}

void CHealth::damage(int32_t & amount)
{
	const int32_t oldCount = getCount();

	const bool withKills = amount >= firstHPleft;

	if(withKills)
	{
		int64_t totalHealth = available();
		if(amount > totalHealth)
			amount = totalHealth;
		totalHealth -= amount;
		if(totalHealth <= 0)
		{
			fullUnits = 0;
			firstHPleft = 0;
		}
		else
		{
			setFromTotal(totalHealth);
		}
	}
	else
	{
		firstHPleft -= amount;
	}

	addResurrected(getCount() - oldCount);
}

void CHealth::heal(int32_t & amount, EHealLevel level, EHealPower power)
{
	const int32_t unitHealth = owner->unitMaxHealth();
	const int32_t oldCount = getCount();

	int32_t maxHeal = std::numeric_limits<int32_t>::max();

	switch(level)
	{
	case EHealLevel::HEAL:
		maxHeal = std::max(0, unitHealth - firstHPleft);
		break;
	case EHealLevel::RESURRECT:
		maxHeal = total() - available();
		break;
	default:
		assert(level == EHealLevel::OVERHEAL);
		break;
	}

	vstd::amax(maxHeal, 0);
	vstd::abetween(amount, 0, maxHeal);

	if(amount == 0)
		return;

	int64_t availableHealth = available();

	availableHealth	+= amount;
	setFromTotal(availableHealth);

	if(power == EHealPower::ONE_BATTLE)
		addResurrected(getCount() - oldCount);
	else
		assert(power == EHealPower::PERMANENT);
}

void CHealth::setFromTotal(const int64_t totalHealth)
{
	const int32_t unitHealth = owner->unitMaxHealth();
	firstHPleft = totalHealth % unitHealth;
	fullUnits = totalHealth / unitHealth;

	if(firstHPleft == 0 && fullUnits >= 1)
	{
		firstHPleft = unitHealth;
		fullUnits -= 1;
	}
}

void CHealth::reset()
{
	fullUnits = 0;
	firstHPleft = 0;
	resurrected = 0;
}

int32_t CHealth::getCount() const
{
	return fullUnits + (firstHPleft > 0 ? 1 : 0);
}

int32_t CHealth::getFirstHPleft() const
{
	return firstHPleft;
}

int32_t CHealth::getResurrected() const
{
	return resurrected;
}

void CHealth::takeResurrected()
{
	if(resurrected != 0)
	{
		int64_t totalHealth = available();

		totalHealth -= resurrected * owner->unitMaxHealth();
		vstd::amax(totalHealth, 0);
		setFromTotal(totalHealth);
		resurrected = 0;
	}
}

void CHealth::serializeJson(JsonSerializeFormat & handler)
{
	handler.serializeInt("firstHPleft", firstHPleft, 0);
	handler.serializeInt("fullUnits", fullUnits, 0);
	handler.serializeInt("resurrected", resurrected, 0);
}

bool IStackState::isDead() const
{
	return !alive() && !isGhost();
}

bool IStackState::isTurret() const
{
	return creatureIndex() == CreatureID::ARROW_TOWERS;
}

bool IStackState::isValidTarget(bool allowDead) const
{
	return (alive() || (allowDead && isDead())) && getPosition().isValid() && !isTurret();
}


///CStackState
CStackState::CStackState(const IUnitInfo * unit_, const IUnitBonus * bonus_)
	: unit(unit_),
	bonus(bonus_),
	cloned(false),
	defending(false),
	defendingAnim(false),
	drainedMana(false),
	fear(false),
	hadMorale(false),
	ghost(false),
	ghostPending(false),
	movedThisTurn(false),
	summoned(false),
	waiting(false),
	casts(this),
	counterAttacks(this),
	health(this),
	shots(this),
	cloneID(-1),
	position()
{

}

CStackState::CStackState(const CStackState & other)
	: unit(other.unit),
	bonus(other.bonus),
	cloned(other.cloned),
	defending(other.defending),
	defendingAnim(other.defendingAnim),
	drainedMana(other.drainedMana),
	fear(other.fear),
	hadMorale(other.hadMorale),
	ghost(other.ghost),
	ghostPending(other.ghostPending),
	movedThisTurn(other.movedThisTurn),
	summoned(other.summoned),
	waiting(other.waiting),
	casts(other.casts),
	counterAttacks(other.counterAttacks),
	health(other.health),
	shots(other.shots),
	cloneID(other.cloneID),
	position(other.position)
{

}

CStackState & CStackState::operator=(const CStackState & other)
{
	//do not change unit and bonus(?) info
	cloned = other.cloned;
	defending = other.defending;
	defendingAnim = other.defendingAnim;
	drainedMana = other.drainedMana;
	fear = other.fear;
	hadMorale = other.hadMorale;
	ghost = other.ghost;
	ghostPending = other.ghostPending;
	movedThisTurn = other.movedThisTurn;
	summoned = other.summoned;
	waiting = other.waiting;
	casts = other.casts;
	counterAttacks = other.counterAttacks;
	health = other.health;
	shots = other.shots;
	cloneID = other.cloneID;
	position = other.position;
	return *this;
}

bool CStackState::ableToRetaliate() const
{
	return alive()
		&& counterAttacks.canUse()
		&& !unitAsBearer()->hasBonusOfType(Bonus::SIEGE_WEAPON)
		&& !unitAsBearer()->hasBonusOfType(Bonus::HYPNOTIZED)
		&& !unitAsBearer()->hasBonusOfType(Bonus::NO_RETALIATION);
}

bool CStackState::alive() const
{
	return health.available() > 0;
}

bool CStackState::isGhost() const
{
	return ghost;
}

bool CStackState::isClone() const
{
	return cloned;
}

bool CStackState::hasClone() const
{
	return cloneID > 0;
}

bool CStackState::canCast() const
{
	return casts.canUse(1);//do not check specific cast abilities here
}

bool CStackState::isCaster() const
{
	return casts.total() > 0;//do not check specific cast abilities here
}

bool CStackState::canShoot() const
{
	return shots.canUse(1) && unitAsBearer()->hasBonusOfType(Bonus::SHOOTER);
}

bool CStackState::isShooter() const
{
	return shots.total() > 0 && unitAsBearer()->hasBonusOfType(Bonus::SHOOTER);
}

int32_t CStackState::getKilled() const
{
	int32_t res = unitBaseAmount() - health.getCount() + health.getResurrected();
	vstd::amax(res, 0);
	return res;
}

int32_t CStackState::getCount() const
{
	return health.getCount();
}

int32_t CStackState::getFirstHPleft() const
{
	return health.getFirstHPleft();
}

int64_t CStackState::getAvailableHealth() const
{
	return health.available();
}

int64_t CStackState::getTotalHealth() const
{
	return health.total();
}

BattleHex CStackState::getPosition() const
{
	return position;
}

bool CStackState::canMove(int turn) const
{
	return alive() && !unitAsBearer()->hasBonus(Selector::type(Bonus::NOT_ACTIVE).And(Selector::turns(turn))); //eg. Ammo Cart or blinded creature
}

bool CStackState::moved(int turn) const
{
	if(!turn)
		return movedThisTurn;
	else
		return false;
}

bool CStackState::willMove(int turn) const
{
	return (turn ? true : !defending)
		   && !moved(turn)
		   && canMove(turn);
}

bool CStackState::waited(int turn) const
{
	if(!turn)
		return waiting;
	else
		return false;
}

bool CStackState::doubleWide() const
{
	return unit->doubleWide();
}

uint32_t CStackState::unitId() const
{
	return unit->unitId();
}

ui8 CStackState::unitSide() const
{
	return unit->unitSide();
}

int32_t CStackState::creatureIndex() const
{
	return unit->creatureIndex();
}

CreatureID CStackState::creatureId() const
{
	return unit->creatureId();
}

int32_t CStackState::creatureLevel() const
{
	return unit->creatureLevel();
}

SlotID CStackState::unitSlot() const
{
	return unit->unitSlot();
}

int32_t CStackState::unitMaxHealth() const
{
	return unit->unitMaxHealth();
}

int32_t CStackState::unitBaseAmount() const
{
	return unit->unitBaseAmount();
}

const IBonusBearer * CStackState::unitAsBearer() const
{
	return bonus->unitAsBearer();
}

bool CStackState::unitHasAmmoCart() const
{
	return bonus->unitHasAmmoCart();
}

CStackState CStackState::asquire() const
{
	return CStackState(*this);
}

void CStackState::serializeJson(JsonSerializeFormat & handler)
{
	if(!handler.saving)
		reset();

	handler.serializeBool("cloned", cloned);
	handler.serializeBool("defending", defending);
	handler.serializeBool("defendingAnim", defendingAnim);
	handler.serializeBool("drainedMana", drainedMana);
	handler.serializeBool("fear", fear);
	handler.serializeBool("hadMorale", hadMorale);
	handler.serializeBool("ghost", ghost);
	handler.serializeBool("ghostPending", ghostPending);
	handler.serializeBool("moved", movedThisTurn);
	handler.serializeBool("summoned", summoned);
	handler.serializeBool("waiting", waiting);

	handler.serializeStruct("casts", casts);
	handler.serializeStruct("counterAttacks", counterAttacks);
	handler.serializeStruct("health", health);
	handler.serializeStruct("shots", shots);

	handler.serializeInt("cloneID", cloneID);

	handler.serializeInt("position", position);
}

void CStackState::localInit()
{
	reset();
	health.init();
}

void CStackState::reset()
{
	cloned = false;
	defending = false;
	defendingAnim = false;
	drainedMana = false;
	fear = false;
	hadMorale = false;
	ghost = false;
	ghostPending = false;
	movedThisTurn = false;
	summoned = false;
	waiting = false;

	casts.reset();
	counterAttacks.reset();
	health.reset();
	shots.reset();

	cloneID = -1;

	position = BattleHex::INVALID;
}

void CStackState::swap(CStackState & other)
{
	std::swap(cloned, other.cloned);
	std::swap(defending, other.defending);
	std::swap(defendingAnim, other.defendingAnim);
	std::swap(drainedMana, other.drainedMana);
	std::swap(fear, other.fear);
	std::swap(hadMorale, other.hadMorale);
	std::swap(ghost, other.ghost);
	std::swap(ghostPending, other.ghostPending);
	std::swap(movedThisTurn, other.movedThisTurn);
	std::swap(summoned, other.summoned);
	std::swap(waiting, other.waiting);

	std::swap(unit, other.unit);
	std::swap(bonus, other.bonus);
	std::swap(casts, other.casts);
	std::swap(counterAttacks, other.counterAttacks);
	std::swap(health, other.health);
	std::swap(shots, other.shots);

	std::swap(cloneID, other.cloneID);

	std::swap(position, other.position);
}

void CStackState::toInfo(CStackStateInfo & info)
{
	info.stackId = unitId();

	//TODO: use instance resolver for battle stacks
	info.data.clear();
	JsonSerializer ser(nullptr, info.data);
	ser.serializeStruct("state", *this);
}

void CStackState::fromInfo(const CStackStateInfo & info)
{
	if(info.stackId != unitId())
		logGlobal->error("Deserialised state from wrong stack");
	//TODO: use instance resolver for battle stacks
	reset();
    JsonDeserializer deser(nullptr, info.data);
    deser.serializeStruct("state", *this);
}

const IUnitInfo * CStackState::getUnitInfo() const
{
	return unit;
}

void CStackState::damage(int32_t & amount)
{
	if(cloned)
	{
		// block ability should not kill clone (0 damage)
		if(amount > 0)
		{
			amount = 1;//TODO: what should be actual damage against clone?
			health.reset();
		}
	}
	else
	{
		health.damage(amount);
	}

	if(health.available() <= 0 && (cloned || summoned))
		ghostPending = true;
}

void CStackState::heal(int32_t & amount, EHealLevel level, EHealPower power)
{
	if(level == EHealLevel::HEAL && power == EHealPower::ONE_BATTLE)
		logGlobal->error("Heal for one battle does not make sense");
	else if(cloned)
		logGlobal->error("Attempt to heal clone");
	else
		health.heal(amount, level, power);
}

///CStack
CStack::CStack(const CStackInstance * Base, PlayerColor O, int I, ui8 Side, SlotID S)
	: CBonusSystemNode(STACK_BATTLE),
	base(Base),
	ID(I),
	type(Base->type),
	baseAmount(base->count),
	owner(O),
	slot(S),
	side(Side),
	stackState(this, this),
	initialPosition()
{
	stackState.health.init(); //???
}

CStack::CStack()
	: CBonusSystemNode(STACK_BATTLE),
	stackState(this, this)
{
	base = nullptr;
	type = nullptr;
	ID = -1;
	baseAmount = -1;
	owner = PlayerColor::NEUTRAL;
	slot = SlotID(255);
	side = 1;
	initialPosition = BattleHex();
}

CStack::CStack(const CStackBasicDescriptor * stack, PlayerColor O, int I, ui8 Side, SlotID S)
	: CBonusSystemNode(STACK_BATTLE),
	base(nullptr),
	ID(I),
	type(stack->type),
	baseAmount(stack->count),
	owner(O),
	slot(S),
	side(Side),
	stackState(this, this),
	initialPosition()
{
	stackState.health.init(); //???
}

const CCreature * CStack::getCreature() const
{
	return type;
}

void CStack::localInit(BattleInfo * battleInfo)
{
	battle = battleInfo;
	assert(type);

	exportBonuses();
	if(base) //stack originating from "real" stack in garrison -> attach to it
	{
		attachTo(const_cast<CStackInstance *>(base));
	}
	else //attach directly to obj to which stack belongs and creature type
	{
		CArmedInstance * army = battle->battleGetArmyObject(side);
		attachTo(army);
		attachTo(const_cast<CCreature *>(type));
	}

	stackState.localInit();
	stackState.position = initialPosition;
}

ui32 CStack::level() const
{
	if(base)
		return base->getLevel(); //creature or commander
	else
		return std::max(1, (int)getCreature()->level); //war machine, clone etc
}

si32 CStack::magicResistance() const
{
	si32 magicResistance;
	if(base)  //TODO: make war machines receive aura of magic resistance
	{
		magicResistance = base->magicResistance();
		int auraBonus = 0;
		for(const CStack * stack : base->armyObj->battle-> batteAdjacentCreatures(this))
		{
			if(stack->owner == owner)
			{
				vstd::amax(auraBonus, stack->valOfBonuses(Bonus::SPELL_RESISTANCE_AURA)); //max value
			}
		}
		magicResistance += auraBonus;
		vstd::amin(magicResistance, 100);
	}
	else
		magicResistance = type->magicResistance();
	return magicResistance;
}

bool CStack::willMove(int turn) const
{
	return stackState.willMove(turn);
}

bool CStack::canMove(int turn) const
{
	return stackState.canMove(turn);
}

bool CStack::moved(int turn) const
{
	return stackState.moved(turn);
}

bool CStack::waited(int turn) const
{
	return stackState.waited(turn);
}

BattleHex CStack::occupiedHex() const
{
	return occupiedHex(stackState.position);
}

BattleHex CStack::occupiedHex(BattleHex assumedPos) const
{
	if(doubleWide())
	{
		if(side == BattleSide::ATTACKER)
			return assumedPos - 1;
		else
			return assumedPos + 1;
	}
	else
	{
		return BattleHex::INVALID;
	}
}

std::vector<BattleHex> CStack::getHexes() const
{
	return getHexes(stackState.position);
}

std::vector<BattleHex> CStack::getHexes(BattleHex assumedPos) const
{
	return getHexes(assumedPos, doubleWide(), side);
}

std::vector<BattleHex> CStack::getHexes(BattleHex assumedPos, bool twoHex, ui8 side)
{
	std::vector<BattleHex> hexes;
	hexes.push_back(assumedPos);

	if(twoHex)
	{
		if(side == BattleSide::ATTACKER)
			hexes.push_back(assumedPos - 1);
		else
			hexes.push_back(assumedPos + 1);
	}

	return hexes;
}

bool CStack::coversPos(BattleHex pos) const
{
	return vstd::contains(getHexes(), pos);
}

std::vector<BattleHex> CStack::getSurroundingHexes(BattleHex attackerPos) const
{
	BattleHex hex = (attackerPos != BattleHex::INVALID) ? attackerPos : stackState.position; //use hypothetical position
	std::vector<BattleHex> hexes;
	if(doubleWide())
	{
		const int WN = GameConstants::BFIELD_WIDTH;
		if(side == BattleSide::ATTACKER)
		{
			//position is equal to front hex
			BattleHex::checkAndPush(hex - ((hex / WN) % 2 ? WN + 2 : WN + 1), hexes);
			BattleHex::checkAndPush(hex - ((hex / WN) % 2 ? WN + 1 : WN), hexes);
			BattleHex::checkAndPush(hex - ((hex / WN) % 2 ? WN : WN - 1), hexes);
			BattleHex::checkAndPush(hex - 2, hexes);
			BattleHex::checkAndPush(hex + 1, hexes);
			BattleHex::checkAndPush(hex + ((hex / WN) % 2 ? WN - 2 : WN - 1), hexes);
			BattleHex::checkAndPush(hex + ((hex / WN) % 2 ? WN - 1 : WN), hexes);
			BattleHex::checkAndPush(hex + ((hex / WN) % 2 ? WN : WN + 1), hexes);
		}
		else
		{
			BattleHex::checkAndPush(hex - ((hex / WN) % 2 ? WN + 1 : WN), hexes);
			BattleHex::checkAndPush(hex - ((hex / WN) % 2 ? WN : WN - 1), hexes);
			BattleHex::checkAndPush(hex - ((hex / WN) % 2 ? WN - 1 : WN - 2), hexes);
			BattleHex::checkAndPush(hex + 2, hexes);
			BattleHex::checkAndPush(hex - 1, hexes);
			BattleHex::checkAndPush(hex + ((hex / WN) % 2 ? WN - 1 : WN), hexes);
			BattleHex::checkAndPush(hex + ((hex / WN) % 2 ? WN : WN + 1), hexes);
			BattleHex::checkAndPush(hex + ((hex / WN) % 2 ? WN + 1 : WN + 2), hexes);
		}
		return hexes;
	}
	else
	{
		return hex.neighbouringTiles();
	}
}

BattleHex::EDir CStack::destShiftDir() const
{
	if(doubleWide())
	{
		if(side == BattleSide::ATTACKER)
			return BattleHex::EDir::RIGHT;
		else
			return BattleHex::EDir::LEFT;
	}
	else
	{
		return BattleHex::EDir::NONE;
	}
}

std::vector<si32> CStack::activeSpells() const
{
	std::vector<si32> ret;

	std::stringstream cachingStr;
	cachingStr << "!type_" << Bonus::NONE << "source_" << Bonus::SPELL_EFFECT;
	CSelector selector = Selector::sourceType(Bonus::SPELL_EFFECT)
						 .And(CSelector([](const Bonus * b)->bool
	{
		return b->type != Bonus::NONE;
	}));

	TBonusListPtr spellEffects = getBonuses(selector, Selector::all, cachingStr.str());
	for(const std::shared_ptr<Bonus> it : *spellEffects)
	{
		if(!vstd::contains(ret, it->sid))  //do not duplicate spells with multiple effects
			ret.push_back(it->sid);
	}

	return ret;
}

CStack::~CStack()
{
	detachFromAll();
}

const CGHeroInstance * CStack::getMyHero() const
{
	if(base)
		return dynamic_cast<const CGHeroInstance *>(base->armyObj);
	else //we are attached directly?
		for(const CBonusSystemNode * n : getParentNodes())
			if(n->getNodeType() == HERO)
				return dynamic_cast<const CGHeroInstance *>(n);

	return nullptr;
}

std::string CStack::nodeName() const
{
	std::ostringstream oss;
	oss << owner.getStr();
	oss << " battle stack [" << ID << "]: " << stackState.getCount() << " of ";
	if(type)
		oss << type->namePl;
	else
		oss << "[UNDEFINED TYPE]";

	oss << " from slot " << slot;
	if(base && base->armyObj)
		oss << " of armyobj=" << base->armyObj->id.getNum();
	return oss.str();
}

void CStack::prepareAttacked(BattleStackAttacked & bsa, CRandomGenerator & rand) const
{
	prepareAttacked(bsa, rand, stackState);
}

void CStack::prepareAttacked(BattleStackAttacked & bsa, CRandomGenerator & rand, const CStackState & customState)
{
	CStackState afterAttack = customState;
	afterAttack.damage(bsa.damageAmount);

	bsa.killedAmount = customState.getCount() - afterAttack.getCount();

	if(!afterAttack.alive() && afterAttack.cloned)
	{
		bsa.flags |= BattleStackAttacked::CLONE_KILLED;
	}
	else if(!afterAttack.alive()) //stack killed
	{
		bsa.flags |= BattleStackAttacked::KILLED;

		int resurrectFactor = afterAttack.unitAsBearer()->valOfBonuses(Bonus::REBIRTH);
		if(resurrectFactor > 0 && afterAttack.canCast()) //there must be casts left
		{
			auto baseAmount = afterAttack.unitBaseAmount();
			int resurrectedStackCount = baseAmount * resurrectFactor / 100;

			// last stack has proportional chance to rebirth
			//FIXME: diff is always 0
			auto diff = baseAmount * resurrectFactor / 100.0 - resurrectedStackCount;
			if(diff > rand.nextDouble(0, 0.99))
			{
				resurrectedStackCount += 1;
			}

			if(afterAttack.unitAsBearer()->hasBonusOfType(Bonus::REBIRTH, 1))
			{
				// resurrect at least one Sacred Phoenix
				vstd::amax(resurrectedStackCount, 1);
			}

			if(resurrectedStackCount > 0)
			{
				afterAttack.casts.use();
				bsa.flags |= BattleStackAttacked::REBIRTH;
				int32_t toHeal = afterAttack.unitMaxHealth() * resurrectedStackCount;
				//TODO: use StackHealedOrResurrected
				//TODO: add one-battle rebirth?
				afterAttack.heal(toHeal, EHealLevel::RESURRECT, EHealPower::PERMANENT);
				afterAttack.counterAttacks.use(afterAttack.counterAttacks.available());
			}
		}
	}

	afterAttack.toInfo(bsa.newState);
	bsa.newState.healthDelta = -bsa.damageAmount;
}

bool CStack::isMeleeAttackPossible(const CStack * attacker, const CStack * defender, BattleHex attackerPos, BattleHex defenderPos)
{
	if(!attackerPos.isValid())
		attackerPos = attacker->stackState.position;
	if(!defenderPos.isValid())
		defenderPos = defender->stackState.position;

	return
		(BattleHex::mutualPosition(attackerPos, defenderPos) >= 0)//front <=> front
		|| (attacker->doubleWide()//back <=> front
			&& BattleHex::mutualPosition(attackerPos + (attacker->side == BattleSide::ATTACKER ? -1 : 1), defenderPos) >= 0)
		|| (defender->doubleWide()//front <=> back
			&& BattleHex::mutualPosition(attackerPos, defenderPos + (defender->side == BattleSide::ATTACKER ? -1 : 1)) >= 0)
		|| (defender->doubleWide() && attacker->doubleWide()//back <=> back
			&& BattleHex::mutualPosition(attackerPos + (attacker->side == BattleSide::ATTACKER ? -1 : 1), defenderPos + (defender->side == BattleSide::ATTACKER ? -1 : 1)) >= 0);

}

std::string CStack::getName() const
{
	return (stackState.getCount() == 1) ? type->nameSing : type->namePl; //War machines can't use base
}

bool CStack::canBeHealed() const
{
	return getFirstHPleft() < MaxHealth()
		   && isValidTarget()
		   && !hasBonusOfType(Bonus::SIEGE_WEAPON);
}

void CStack::makeGhost()
{
	stackState.health.reset();
	stackState.ghostPending = true;
}

ui8 CStack::getSpellSchoolLevel(const spells::Mode mode, const CSpell * spell, int * outSelectedSchool) const
{
	int skill = valOfBonuses(Selector::typeSubtype(Bonus::SPELLCASTER, spell->id));
	vstd::abetween(skill, 0, 3);
	return skill;
}

ui32 CStack::getSpellBonus(const CSpell * spell, ui32 base, const IStackState * affectedStack) const
{
	//stacks does not have sorcery-like bonuses (yet?)
	return base;
}

ui32 CStack::getSpecificSpellBonus(const CSpell * spell, ui32 base) const
{
	return base;
}

int CStack::getEffectLevel(const spells::Mode mode, const CSpell * spell) const
{
	return getSpellSchoolLevel(mode, spell);
}

int CStack::getEffectPower(const spells::Mode mode, const CSpell * spell) const
{
	return valOfBonuses(Bonus::CREATURE_SPELL_POWER) * stackState.getCount() / 100;
}

int CStack::getEnchantPower(const spells::Mode mode, const CSpell * spell) const
{
	int res = valOfBonuses(Bonus::CREATURE_ENCHANT_POWER);
	if(res <= 0)
		res = 3;//default for creatures
	return res;
}

int CStack::getEffectValue(const spells::Mode mode, const CSpell * spell) const
{
	return valOfBonuses(Bonus::SPECIFIC_SPELL_POWER, spell->id.toEnum()) * stackState.getCount();
}

const PlayerColor CStack::getOwner() const
{
	return battle->battleGetOwner(this);
}

void CStack::getCasterName(MetaString & text) const
{
	//always plural name in case of spell cast.
	addNameReplacement(text, true);
}

void CStack::getCastDescription(const CSpell * spell, MetaString & text) const
{
	text.addTxt(MetaString::GENERAL_TXT, 565);//The %s casts %s
	//todo: use text 566 for single creature
	getCasterName(text);
	text.addReplacement(MetaString::SPELL_NAME, spell->id.toEnum());
}

void CStack::getCastDescription(const CSpell * spell, const std::vector<const CStack *> & attacked, MetaString & text) const
{
	getCastDescription(spell, text);
}

void CStack::spendMana(const spells::Mode mode, const CSpell * spell, const spells::PacketSender * server, const int spellCost) const
{
	if(mode == spells::Mode::CREATURE_ACTIVE || mode == spells::Mode::ENCHANTER)
	{
		if(spellCost != 1)
			logGlobal->warn("Unexpected spell cost for creature %d", spellCost);

		BattleSetStackProperty ssp;
		ssp.stackID = ID;
		ssp.which = BattleSetStackProperty::CASTS;
		ssp.val = -spellCost;
		ssp.absolute = false;
		server->sendAndApply(&ssp);

		if(mode == spells::Mode::ENCHANTER)
		{
			auto bl = getBonuses(Selector::typeSubtype(Bonus::ENCHANTER, spell->id.toEnum()));

			int cooldown = 1;
			for(auto b : *(bl))
				if(b->additionalInfo > cooldown)
					cooldown = b->additionalInfo;

			BattleSetStackProperty ssp;
			ssp.which = BattleSetStackProperty::ENCHANTER_COUNTER;
			ssp.absolute = false;
			ssp.val = cooldown;
			ssp.stackID = ID;
			server->sendAndApply(&ssp);
		}
	}
}

int32_t CStack::creatureIndex() const
{
	return static_cast<int32_t>(creatureId().toEnum());
}

CreatureID CStack::creatureId() const
{
	return getCreature()->idNumber;
}

int32_t CStack::creatureLevel() const
{
	return static_cast<int32_t>(getCreature()->level);
}

int32_t CStack::unitMaxHealth() const
{
	return MaxHealth();
}

int32_t CStack::unitBaseAmount() const
{
	return baseAmount;
}

const IBonusBearer * CStack::unitAsBearer() const
{
	return this;
}

bool CStack::unitHasAmmoCart() const
{
	bool hasAmmoCart = false;

	for(const CStack * st : battle->stacks)
	{
		if(battle->battleMatchOwner(st, this, true) && st->getCreature()->idNumber == CreatureID::AMMO_CART && st->alive())
		{
			hasAmmoCart = true;
			break;
		}
	}
	return hasAmmoCart;
}

bool CStack::doubleWide() const
{
	return getCreature()->doubleWide;
}

uint32_t CStack::unitId() const
{
	return ID;
}

ui8 CStack::unitSide() const
{
	return side;
}

SlotID CStack::unitSlot() const
{
	return slot;
}

bool CStack::ableToRetaliate() const
{
	return stackState.ableToRetaliate();
}

bool CStack::alive() const
{
	return stackState.alive();
}

bool CStack::isClone() const
{
	return stackState.isClone();
}

bool CStack::hasClone() const
{
	return stackState.hasClone();
}

bool CStack::isGhost() const
{
	return stackState.isGhost();
}

int32_t CStack::getKilled() const
{
	return stackState.getKilled();
}

bool CStack::canCast() const
{
	return stackState.canCast();
}

bool CStack::isCaster() const
{
	return stackState.isCaster();
}

bool CStack::canShoot() const
{
	return stackState.canShoot();
}

bool CStack::isShooter() const
{
	return stackState.isShooter();
}

int32_t CStack::getCount() const
{
	return stackState.getCount();
}

int32_t CStack::getFirstHPleft() const
{
	return stackState.getFirstHPleft();
}

int64_t CStack::getAvailableHealth() const
{
	return stackState.getAvailableHealth();
}

int64_t CStack::getTotalHealth() const
{
	return stackState.getTotalHealth();
}

BattleHex CStack::getPosition() const
{
	return stackState.getPosition();
}

CStackState CStack::asquire() const
{
	return CStackState(stackState);
}

void CStack::addText(MetaString & text, ui8 type, int32_t serial, const boost::logic::tribool & plural) const
{
	if(boost::logic::indeterminate(plural))
		serial = VLC->generaltexth->pluralText(serial, stackState.getCount());
	else if(plural)
		serial = VLC->generaltexth->pluralText(serial, 2);
	else
		serial = VLC->generaltexth->pluralText(serial, 1);

	text.addTxt(type, serial);
}

void CStack::addNameReplacement(MetaString & text, const boost::logic::tribool & plural) const
{
	if(boost::logic::indeterminate(plural))
		text.addCreReplacement(type->idNumber, stackState.getCount());
	else if(plural)
		text.addReplacement(MetaString::CRE_PL_NAMES, type->idNumber.num);
	else
		text.addReplacement(MetaString::CRE_SING_NAMES, type->idNumber.num);
}

std::string CStack::formatGeneralMessage(const int32_t baseTextId) const
{
	const int32_t textId = VLC->generaltexth->pluralText(baseTextId, stackState.getCount());

	MetaString text;
	text.addTxt(MetaString::GENERAL_TXT, textId);
	text.addCreReplacement(type->idNumber, stackState.getCount());

	return text.toString();
}

