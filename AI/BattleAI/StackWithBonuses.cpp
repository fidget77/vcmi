/*
 * StackWithBonuses.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "StackWithBonuses.h"
#include "../../lib/NetPacksBase.h"
#include "../../lib/CStack.h"

void actualizeEffect(TBonusListPtr target, const Bonus & ef)
{
	for(auto bonus : *target) //TODO: optimize
	{
		if(bonus->source == Bonus::SPELL_EFFECT && bonus->type == ef.type && bonus->subtype == ef.subtype)
		{
			bonus->turnsRemain = std::max(bonus->turnsRemain, ef.turnsRemain);
		}
	}
}

StackWithBonuses::StackWithBonuses(const HypotheticBattle * Owner, const battle::CUnitState * Stack)
	: state(Stack->getUnitInfo(), this, this),
	origBearer(Stack),
	owner(Owner)
{
	state = *Stack;
}

StackWithBonuses::~StackWithBonuses() = default;


const TBonusListPtr StackWithBonuses::getAllBonuses(const CSelector & selector, const CSelector & limit,
	const CBonusSystemNode * root, const std::string & cachingStr) const
{
	TBonusListPtr ret = std::make_shared<BonusList>();
	const TBonusListPtr originalList = origBearer->getAllBonuses(selector, limit, root, cachingStr);

	vstd::copy_if(*originalList, std::back_inserter(*ret), [this](const std::shared_ptr<Bonus> & b)
	{
		return !vstd::contains(bonusesToRemove, b);
	});


	for(const Bonus & bonus : bonusesToUpdate)
	{
		if(selector(&bonus) && (!limit || !limit(&bonus)))
		{
			if(ret->getFirst(Selector::source(Bonus::SPELL_EFFECT, bonus.sid).And(Selector::typeSubtype(bonus.type, bonus.subtype))))
			{
				actualizeEffect(ret, bonus);
			}
			else
			{
				auto b = std::make_shared<Bonus>(bonus);
				ret->push_back(b);
			}
		}
	}

	for(auto & bonus : bonusesToAdd)
	{
		auto b = std::make_shared<Bonus>(bonus);
		if(selector(b.get()) && (!limit || !limit(b.get())))
			ret->push_back(b);
	}
	//TODO limiters?
	return ret;
}

int64_t StackWithBonuses::getTreeVersion() const
{
	return owner->getTreeVersion();
}


bool StackWithBonuses::unitHasAmmoCart() const
{
	//FIXME: check ammocart alive state here
	return false;
}

void StackWithBonuses::addUnitBonus(const std::vector<Bonus> & bonus)
{
	vstd::concatenate(bonusesToAdd, bonus);
}

void StackWithBonuses::updateUnitBonus(const std::vector<Bonus> & bonus)
{
	//TODO: optimize, actualize to last value

	vstd::concatenate(bonusesToUpdate, bonus);
}

void StackWithBonuses::removeUnitBonus(const std::vector<Bonus> & bonus)
{
	for(const Bonus & one : bonus)
	{
		CSelector selector([&one](const Bonus * b) -> bool
		{
			//compare everything but turnsRemain, limiter and propagator
			return one.duration == b->duration
			&& one.type == b->type
			&& one.subtype == b->subtype
			&& one.source == b->source
			&& one.val == b->val
			&& one.sid == b->sid
			&& one.valType == b->valType
			&& one.additionalInfo == b->additionalInfo
			&& one.effectRange == b->effectRange
			&& one.description == b->description;
		});

		removeUnitBonus(selector);
	}
}

void StackWithBonuses::removeUnitBonus(const CSelector & selector)
{
	TBonusListPtr toRemove = origBearer->getBonuses(selector);

	for(auto b : *toRemove)
		bonusesToRemove.insert(b);

	vstd::erase_if(bonusesToAdd, [&](const Bonus & b){return selector(&b);});
	vstd::erase_if(bonusesToUpdate, [&](const Bonus & b){return selector(&b);});
}


HypotheticBattle::HypotheticBattle(Subject realBattle)
	: BattleProxy(realBattle),
	bonusTreeVersion(1)
{

}

std::shared_ptr<StackWithBonuses> HypotheticBattle::getForUpdate(uint32_t id)
{
	auto iter = stackStates.find(id);

	if(iter == stackStates.end())
	{
		const CStack * s = subject->battleGetStackByID(id, false);

		auto ret = std::make_shared<StackWithBonuses>(this, &s->stackState);
		stackStates[id] = ret;
		return ret;
	}
	else
	{
		return iter->second;
	}
}

battle::Units HypotheticBattle::getUnitsIf(battle::UnitFilter predicate) const
{
	//TODO: added units (clones, summoned etc)

	auto getAll = [](const battle::Unit * unit)->bool
	{
		return !unit->isGhost();
	};

	battle::Units all = BattleProxy::getUnitsIf(getAll);

	battle::Units ret;

	for(auto one : all)
	{
		auto ID = one->unitId();

		auto iter = stackStates.find(ID);

		if(iter == stackStates.end())
		{
			if(predicate(one))
				ret.push_back(one);
		}
		else
		{
			auto & swb = iter->second;
			const battle::Unit * changed = &(swb->state);

			if(predicate(changed))
				ret.push_back(changed);
		}
	}

	return ret;
}

void HypotheticBattle::nextRound(int32_t roundNr)
{
	//TODO:HypotheticBattle::nextRound

	for(auto unit : battleAliveUnits())
	{
		auto forUpdate = getForUpdate(unit->unitId());
		//TODO: update Bonus::NTurns effects
		forUpdate->state.afterNewRound();
	}
}

void HypotheticBattle::nextTurn(uint32_t unitId)
{
	auto unit = getForUpdate(unitId);

	unit->removeUnitBonus(Bonus::UntilGetsTurn);

	unit->state.afterGetsTurn();
}

void HypotheticBattle::updateUnit(const CStackStateInfo & changes)
{
	std::shared_ptr<StackWithBonuses> changed = getForUpdate(changes.stackId);

	changed->state.fromInfo(changes);

	if(changes.healthDelta < 0)
	{
		changed->removeUnitBonus(Bonus::UntilBeingAttacked);
	}
}

void HypotheticBattle::addUnitBonus(uint32_t id, const std::vector<Bonus> & bonus)
{
	getForUpdate(id)->addUnitBonus(bonus);
	bonusTreeVersion++;
}

void HypotheticBattle::updateUnitBonus(uint32_t id, const std::vector<Bonus> & bonus)
{
	getForUpdate(id)->updateUnitBonus(bonus);
	bonusTreeVersion++;
}

void HypotheticBattle::removeUnitBonus(uint32_t id, const std::vector<Bonus> & bonus)
{
	getForUpdate(id)->removeUnitBonus(bonus);
	bonusTreeVersion++;
}

uint32_t HypotheticBattle::nextUnitId() const
{
	//TODO:
	return subject->battleNextUnitId();

}

int64_t HypotheticBattle::getActualDamage(const TDmgRange & damage, int32_t attackerCount, vstd::RNG & rng) const
{
	return (damage.first + damage.second) / 2;
}

int64_t HypotheticBattle::getTreeVersion() const
{
	return getBattleNode()->getTreeVersion() + bonusTreeVersion;
}
