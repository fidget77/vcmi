/*
 * CUnitState.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "CUnitState.h"

#include "../NetPacksBase.h"

#include "../serializer/JsonDeserializer.h"
#include "../serializer/JsonSerializer.h"

namespace battle
{

CTotalsProxy::CTotalsProxy(const IBonusBearer * Target, CSelector Selector, int InitialValue)
	: target(Target),
	selector(Selector),
	initialValue(InitialValue),
	meleeCachedLast(0),
	meleeValue(0),
	rangedCachedLast(0),
	rangedValue(0)
{
}

CTotalsProxy::CTotalsProxy(const CTotalsProxy & other)
	: target(other.target),
	selector(other.selector),
	initialValue(other.initialValue),
	meleeCachedLast(other.meleeCachedLast),
	meleeValue(other.meleeValue),
	rangedCachedLast(other.rangedCachedLast),
	rangedValue(other.rangedValue)
{
}

CTotalsProxy & CTotalsProxy::operator=(const CTotalsProxy & other)
{
	initialValue = other.initialValue;
	meleeCachedLast = other.meleeCachedLast;
	meleeValue = other.meleeValue;
	rangedCachedLast = other.rangedCachedLast;
	rangedValue = other.rangedValue;
	return *this;
}

int CTotalsProxy::getMeleeValue() const
{
	static const auto limit = Selector::effectRange(Bonus::NO_LIMIT).Or(Selector::effectRange(Bonus::ONLY_MELEE_FIGHT));

	const auto treeVersion = target->getTreeVersion();

	if(treeVersion != meleeCachedLast)
	{
		auto bonuses = target->getBonuses(selector, limit);
		meleeValue = initialValue + bonuses->valOfBonuses(Selector::all);
		meleeCachedLast = treeVersion;
	}
	return meleeValue;
}

int CTotalsProxy::getRangedValue() const
{
	static const auto limit = Selector::effectRange(Bonus::NO_LIMIT).Or(Selector::effectRange(Bonus::ONLY_DISTANCE_FIGHT));

	const auto treeVersion = target->getTreeVersion();

	if(treeVersion != rangedCachedLast)
	{
		auto bonuses = target->getBonuses(selector, limit);
		rangedValue = initialValue + bonuses->valOfBonuses(Selector::all);
		rangedCachedLast = treeVersion;
	}
	return rangedValue;
}

///CCheckProxy
CCheckProxy::CCheckProxy(const IBonusBearer * Target, CSelector Selector)
	: target(Target),
	selector(Selector),
	cachedLast(0),
	hasBonus(false)
{
}

CCheckProxy::CCheckProxy(const CCheckProxy & other)
	: target(other.target),
	selector(other.selector),
	cachedLast(other.cachedLast),
	hasBonus(other.hasBonus)
{
}

bool CCheckProxy::getHasBonus() const
{
	const auto treeVersion = target->getTreeVersion();

	if(treeVersion != cachedLast)
	{
		hasBonus = target->hasBonus(selector);
		cachedLast = treeVersion;
	}

	return hasBonus;
}

///CAttacks
CAttacks::CAttacks(const battle::Unit * Owner)
	: CTotalsProxy(Owner, Selector::type(Bonus::ADDITIONAL_ATTACK), 1)
{
}

CAttacks::CAttacks(const CAttacks & other)
	: CTotalsProxy(other)
{
}

CAttacks & CAttacks::operator=(const CAttacks & other)
{
	CTotalsProxy::operator=(other);
	return *this;
}

///CAmmo
CAmmo::CAmmo(const battle::Unit * Owner, CSelector totalSelector)
	: used(0),
	owner(Owner),
	totalProxy(Owner, totalSelector)
{
	reset();
}

CAmmo::CAmmo(const CAmmo & other)
	: used(other.used),
	owner(other.owner),
	totalProxy(other.totalProxy)
{

}

CAmmo & CAmmo::operator= (const CAmmo & other)
{
	used = other.used;
	totalProxy = other.totalProxy;
	return *this;
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
CShots::CShots(const battle::Unit * Owner, const IUnitEnvironment * Env)
	: CAmmo(Owner, Selector::type(Bonus::SHOTS)),
	env(Env),
	shooter(Owner, Selector::type(Bonus::SHOOTER))
{
}

CShots::CShots(const CShots & other)
	: CAmmo(other),
	env(other.env),
	shooter(other.shooter)
{
}

CShots & CShots::operator=(const CShots & other)
{
	CAmmo::operator=(other);
	shooter = other.shooter;
	return *this;
}

bool CShots::isLimited() const
{
	return !env->unitHasAmmoCart() || !shooter.getHasBonus();
}

int32_t CShots::total() const
{
	if(shooter.getHasBonus())
		return CAmmo::total();
	else
		return 0;
}

///CCasts
CCasts::CCasts(const battle::Unit * Owner):
	CAmmo(Owner, Selector::type(Bonus::CASTS))
{
}

CCasts::CCasts(const CCasts & other)
	: CAmmo(other)
{
}

CCasts & CCasts::operator=(const CCasts & other)
{
	CAmmo::operator=(other);
	return *this;
}

///CRetaliations
CRetaliations::CRetaliations(const battle::Unit * Owner)
	: CAmmo(Owner, Selector::type(Bonus::ADDITIONAL_RETALIATION)),
	totalCache(0),
	noRetaliation(Owner, Selector::type(Bonus::SIEGE_WEAPON).Or(Selector::type(Bonus::HYPNOTIZED)).Or(Selector::type(Bonus::NO_RETALIATION))),
	unlimited(Owner, Selector::type(Bonus::UNLIMITED_RETALIATIONS))
{
}

CRetaliations::CRetaliations(const CRetaliations & other)
	: CAmmo(other),
	totalCache(other.totalCache),
	noRetaliation(other.noRetaliation),
	unlimited(other.unlimited)
{
}

CRetaliations & CRetaliations::operator=(const CRetaliations & other)
{
	CAmmo::operator=(other);
	totalCache = other.totalCache;
	noRetaliation = other.noRetaliation;
	unlimited = other.unlimited;
	return *this;
}

bool CRetaliations::isLimited() const
{
	return !unlimited.getHasBonus() || noRetaliation.getHasBonus();
}

int32_t CRetaliations::total() const
{
	if(noRetaliation.getHasBonus())
		return 0;

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

void CHealth::damage(int64_t & amount)
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

void CHealth::heal(int64_t & amount, EHealLevel level, EHealPower power)
{
	const int32_t unitHealth = owner->unitMaxHealth();
	const int32_t oldCount = getCount();

	int64_t maxHeal = std::numeric_limits<int64_t>::max();

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


///CUnitState
CUnitState::CUnitState(const IUnitInfo * unit_, const IBonusBearer * bonus_, const IUnitEnvironment * env_)
	: unit(unit_),
	bonus(bonus_),
	env(env_),
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
	health(unit_),
	shots(this, env_),
	totalAttacks(this),
	minDamage(this, Selector::typeSubtype(Bonus::CREATURE_DAMAGE, 0).Or(Selector::typeSubtype(Bonus::CREATURE_DAMAGE, 1)), 0),
	maxDamage(this, Selector::typeSubtype(Bonus::CREATURE_DAMAGE, 0).Or(Selector::typeSubtype(Bonus::CREATURE_DAMAGE, 2)), 0),
	attack(this, Selector::typeSubtype(Bonus::PRIMARY_SKILL, PrimarySkill::ATTACK), 0),
	defence(this, Selector::typeSubtype(Bonus::PRIMARY_SKILL, PrimarySkill::DEFENSE), 0),
	inFrenzy(this, Selector::type(Bonus::IN_FRENZY)),
	cloneID(-1),
	position()
{

}

CUnitState::CUnitState(const CUnitState & other)
	: unit(other.unit),
	bonus(other.bonus),
	env(other.env),
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
	totalAttacks(other.totalAttacks),
	minDamage(other.minDamage),
	maxDamage(other.maxDamage),
	inFrenzy(other.inFrenzy),
	attack(other.attack),
	defence(other.defence),
	cloneID(other.cloneID),
	position(other.position)
{

}

CUnitState & CUnitState::operator=(const CUnitState & other)
{
	//do not change unit and bonus info

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
	totalAttacks = other.totalAttacks;
	minDamage = other.minDamage;
	maxDamage = other.maxDamage;
	attack = other.attack;
	defence = other.defence;
	inFrenzy = other.inFrenzy;
	cloneID = other.cloneID;
	position = other.position;
	return *this;
}

bool CUnitState::ableToRetaliate() const
{
	return alive()
		&& counterAttacks.canUse();
}

bool CUnitState::alive() const
{
	return health.getCount() > 0;
}

bool CUnitState::isGhost() const
{
	return ghost;
}

bool CUnitState::isClone() const
{
	return cloned;
}

bool CUnitState::hasClone() const
{
	return cloneID > 0;
}

bool CUnitState::isSummoned() const
{
	return summoned;
}

bool CUnitState::canCast() const
{
	return casts.canUse(1);//do not check specific cast abilities here
}

bool CUnitState::isCaster() const
{
	return casts.total() > 0;//do not check specific cast abilities here
}

bool CUnitState::canShoot() const
{
	return shots.canUse(1);
}

bool CUnitState::isShooter() const
{
	return shots.total() > 0;
}

int32_t CUnitState::getKilled() const
{
	int32_t res = unitBaseAmount() - health.getCount() + health.getResurrected();
	vstd::amax(res, 0);
	return res;
}

int32_t CUnitState::getCount() const
{
	return health.getCount();
}

int32_t CUnitState::getFirstHPleft() const
{
	return health.getFirstHPleft();
}

int64_t CUnitState::getAvailableHealth() const
{
	return health.available();
}

int64_t CUnitState::getTotalHealth() const
{
	return health.total();
}

BattleHex CUnitState::getPosition() const
{
	return position;
}

int32_t CUnitState::getInitiative(int turn) const
{
	return valOfBonuses(Selector::type(Bonus::STACKS_SPEED).And(Selector::turns(turn)));
}

bool CUnitState::canMove(int turn) const
{
	return alive() && !hasBonus(Selector::type(Bonus::NOT_ACTIVE).And(Selector::turns(turn))); //eg. Ammo Cart or blinded creature
}

bool CUnitState::defended(int turn) const
{
	if(!turn)
		return defending;
	else
		return false;
}

bool CUnitState::moved(int turn) const
{
	if(!turn)
		return movedThisTurn;
	else
		return false;
}

bool CUnitState::willMove(int turn) const
{
	return (turn ? true : !defending)
		   && !moved(turn)
		   && canMove(turn);
}

bool CUnitState::waited(int turn) const
{
	if(!turn)
		return waiting;
	else
		return false;
}

uint32_t CUnitState::unitId() const
{
	return unit->unitId();
}

ui8 CUnitState::unitSide() const
{
	return unit->unitSide();
}

const CCreature * CUnitState::creatureType() const
{
	return unit->creatureType();
}

PlayerColor CUnitState::unitOwner() const
{
	return unit->unitOwner();
}

SlotID CUnitState::unitSlot() const
{
	return unit->unitSlot();
}

int32_t CUnitState::unitMaxHealth() const
{
	return unit->unitMaxHealth();
}

int32_t CUnitState::unitBaseAmount() const
{
	return unit->unitBaseAmount();
}

int CUnitState::battleQueuePhase(int turn) const
{
	if(turn <= 0 && waited()) //consider waiting state only for ongoing round
	{
		if(hadMorale)
			return 2;
		else
			return 3;
	}
	else if(creatureIndex() == CreatureID::CATAPULT || isTurret()) //catapult and turrets are first
	{
		return 0;
	}
	else
	{
		return 1;
	}
}

int CUnitState::getAttack(bool ranged) const
{
	int ret = ranged ? attack.getRangedValue() : attack.getMeleeValue();

	if(!inFrenzy->empty())
	{
		double frenzyPower = (double)inFrenzy->totalValue() / 100;
		frenzyPower *= (double) (ranged ? defence.getRangedValue() : defence.getMeleeValue());
		ret += frenzyPower;
	}

	vstd::amax(ret, 0);
	return ret;
}

int CUnitState::getDefence(bool ranged) const
{
	if(!inFrenzy->empty())
	{
		return 0;
	}
	else
	{
		int ret = ranged ? defence.getRangedValue() : defence.getMeleeValue();
		vstd::amax(ret, 0);
		return ret;
	}
}

std::shared_ptr<CUnitState> CUnitState::asquire() const
{
	return std::make_shared<CUnitState>(*this);
}

void CUnitState::serializeJson(JsonSerializeFormat & handler)
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

void CUnitState::localInit()
{
	reset();
	health.init();
}

void CUnitState::reset()
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

void CUnitState::toInfo(CStackStateInfo & info)
{
	info.stackId = unitId();

	//TODO: use instance resolver for battle stacks
	info.data.clear();
	JsonSerializer ser(nullptr, info.data);
	ser.serializeStruct("state", *this);
}

void CUnitState::fromInfo(const CStackStateInfo & info)
{
	if(info.stackId != unitId())
		logGlobal->error("Deserialised state from wrong stack");
	//TODO: use instance resolver for battle stacks
	reset();
    JsonDeserializer deser(nullptr, info.data);
    deser.serializeStruct("state", *this);
}

const IUnitInfo * CUnitState::getUnitInfo() const
{
	return unit;
}

void CUnitState::damage(int64_t & amount)
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

void CUnitState::heal(int64_t & amount, EHealLevel level, EHealPower power)
{
	if(level == EHealLevel::HEAL && power == EHealPower::ONE_BATTLE)
		logGlobal->error("Heal for one battle does not make sense");
	else if(cloned)
		logGlobal->error("Attempt to heal clone");
	else
		health.heal(amount, level, power);
}

const TBonusListPtr CUnitState::getAllBonuses(const CSelector & selector, const CSelector & limit, const CBonusSystemNode * root, const std::string & cachingStr) const
{
	return bonus->getAllBonuses(selector, limit, root, cachingStr);
}

int64_t CUnitState::getTreeVersion() const
{
	return bonus->getTreeVersion();
}

void CUnitState::afterAttack(bool ranged, bool counter)
{
	if(counter)
		counterAttacks.use();

	if(ranged)
		shots.use();
}

} // namespace battle
