/*
 * BattleAI.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "BattleAI.h"

#include <vstd/RNG.h>

#include "StackWithBonuses.h"
#include "EnemyInfo.h"
#include "PossibleSpellcast.h"
#include "../../lib/CThreadHelper.h"
#include "../../lib/spells/CSpellHandler.h"
#include "../../lib/spells/ISpellMechanics.h"

#define LOGL(text) print(text)
#define LOGFL(text, formattingEl) print(boost::str(boost::format(text) % formattingEl))

class RNGStub : public vstd::RNG
{
public:
	vstd::TRandI64 getInt64Range(int64_t lower, int64_t upper) override
	{
		return [=]()->int64_t
		{
			return (lower + upper)/2;
		};
	}

	vstd::TRand getDoubleRange(double lower, double upper) override
	{
		return [=]()->double
		{
			return (lower + upper)/2;
		};
	}
};

CBattleAI::CBattleAI(void)
	: side(-1), wasWaitingForRealize(false), wasUnlockingGs(false)
{
}

CBattleAI::~CBattleAI(void)
{
	if(cb)
	{
		//Restore previous state of CB - it may be shared with the main AI (like VCAI)
		cb->waitTillRealize = wasWaitingForRealize;
		cb->unlockGsWhenWaiting = wasUnlockingGs;
	}
}

void CBattleAI::init(std::shared_ptr<CBattleCallback> CB)
{
	setCbc(CB);
	cb = CB;
	playerID = *CB->getPlayerID(); //TODO should be sth in callback
	wasWaitingForRealize = cb->waitTillRealize;
	wasUnlockingGs = CB->unlockGsWhenWaiting;
	CB->waitTillRealize = true;
	CB->unlockGsWhenWaiting = false;
}

BattleAction CBattleAI::activeStack( const CStack * stack )
{
	LOG_TRACE_PARAMS(logAi, "stack: %s", stack->nodeName())	;
	setCbc(cb); //TODO: make solid sure that AIs always use their callbacks (need to take care of event handlers too)
	try
	{
		if(stack->type->idNumber == CreatureID::CATAPULT)
			return useCatapult(stack);
		if(stack->hasBonusOfType(Bonus::SIEGE_WEAPON) && stack->hasBonusOfType(Bonus::HEALER))
		{
			auto healingTargets = cb->battleGetStacks(CBattleInfoEssentials::ONLY_MINE);
			std::map<int, const CStack*> woundHpToStack;
			for(auto stack : healingTargets)
				if(auto woundHp = stack->MaxHealth() - stack->getFirstHPleft())
					woundHpToStack[woundHp] = stack;
			if(woundHpToStack.empty())
				return BattleAction::makeDefend(stack);
			else
				return BattleAction::makeHeal(stack, woundHpToStack.rbegin()->second); //last element of the woundHpToStack is the most wounded stack
		}

		attemptCastingSpell();

		if(auto ret = getCbc()->battleIsFinished())
		{
			//spellcast may finish battle
			//send special preudo-action
			BattleAction cancel;
			cancel.actionType = EActionType::CANCEL;
			return cancel;
		}

		if(auto action = considerFleeingOrSurrendering())
			return *action;
		//best action is from effective owner PoV, we are effective owner as we received "activeStack"

		HypotheticBattle hb(getCbc());

		PotentialTargets targets(stack, &hb);
		if(targets.possibleAttacks.size())
		{
			auto hlp = targets.bestAction();
			if(hlp.attack.shooting)
				return BattleAction::makeShotAttack(stack, hlp.enemy.get());
			else
				return BattleAction::makeMeleeAttack(stack, hlp.enemy.get(), hlp.tile);
		}
		else
		{
			if(stack->waited())
			{
				//ThreatMap threatsToUs(stack); // These lines may be usefull but they are't used in the code.
				auto dists = getCbc()->battleGetDistances(stack, stack->getPosition());
				const EnemyInfo &ei= *range::min_element(targets.unreachableEnemies, std::bind(isCloser, _1, _2, std::ref(dists)));
				if(distToNearestNeighbour(ei.s->getPosition(), dists) < GameConstants::BFIELD_SIZE)
				{
					return goTowards(stack, ei.s->getPosition());
				}
			}
			else
			{
				return BattleAction::makeWait(stack);
			}
		}
	}
	catch(boost::thread_interrupted &)
	{
		throw;
	}
	catch(std::exception &e)
	{
		logAi->error("Exception occurred in %s %s",__FUNCTION__, e.what());
	}
	return BattleAction::makeDefend(stack);
}

BattleAction CBattleAI::goTowards(const CStack * stack, BattleHex destination)
{
	assert(destination.isValid());
	auto avHexes = cb->battleGetAvailableHexes(stack, false);
	auto reachability = cb->getReachability(stack);
	if(vstd::contains(avHexes, destination))
		return BattleAction::makeMove(stack, destination);
	auto destNeighbours = destination.neighbouringTiles();
	if(vstd::contains_if(destNeighbours, [&](BattleHex n) { return stack->coversPos(destination); }))
	{
		logAi->warn("Warning: already standing on neighbouring tile!");
		//We shouldn't even be here...
		return BattleAction::makeDefend(stack);
	}
	vstd::erase_if(destNeighbours, [&](BattleHex hex){ return !reachability.accessibility.accessible(hex, stack); });
	if(!avHexes.size() || !destNeighbours.size()) //we are blocked or dest is blocked
	{
		return BattleAction::makeDefend(stack);
	}
	if(stack->hasBonusOfType(Bonus::FLYING))
	{
		// Flying stack doesn't go hex by hex, so we can't backtrack using predecessors.
		// We just check all available hexes and pick the one closest to the target.
		auto distToDestNeighbour = [&](BattleHex hex) -> int
		{
			auto nearestNeighbourToHex = vstd::minElementByFun(destNeighbours, [&](BattleHex a)
			{return BattleHex::getDistance(a, hex);});
			return BattleHex::getDistance(*nearestNeighbourToHex, hex);
		};
		auto nearestAvailableHex = vstd::minElementByFun(avHexes, distToDestNeighbour);
		return BattleAction::makeMove(stack, *nearestAvailableHex);
	}
	else
	{
		BattleHex bestNeighbor = destination;
		if(distToNearestNeighbour(destination, reachability.distances, &bestNeighbor) > GameConstants::BFIELD_SIZE)
		{
			return BattleAction::makeDefend(stack);
		}
		BattleHex currentDest = bestNeighbor;
		while(1)
		{
			assert(currentDest.isValid());
			if(vstd::contains(avHexes, currentDest))
				return BattleAction::makeMove(stack, currentDest);
			currentDest = reachability.predecessors[currentDest];
		}
	}
}

BattleAction CBattleAI::useCatapult(const CStack * stack)
{
	throw std::runtime_error("The method or operation is not implemented.");
}


enum class SpellTypes
{
	ADVENTURE, BATTLE, OTHER
};

SpellTypes spellType(const CSpell * spell)
{
	if(!spell->isCombatSpell() || spell->isCreatureAbility())
		return SpellTypes::OTHER;

	if(spell->isOffensiveSpell() || spell->hasEffects() || spell->hasBattleEffects())
		return SpellTypes::BATTLE;

	return SpellTypes::OTHER;
}

void CBattleAI::attemptCastingSpell()
{
	auto hero = cb->battleGetMyHero();
	if(!hero)
		return;

	if(cb->battleCanCastSpell(hero, spells::Mode::HERO) != ESpellCastProblem::OK)
		return;

	LOGL("Casting spells sounds like fun. Let's see...");
	//Get all spells we can cast
	std::vector<const CSpell*> possibleSpells;
	vstd::copy_if(VLC->spellh->objects, std::back_inserter(possibleSpells), [this, hero] (const CSpell *s) -> bool
	{
		return s->canBeCast(getCbc().get(), spells::Mode::HERO, hero);
	});
	LOGFL("I can cast %d spells.", possibleSpells.size());

	vstd::erase_if(possibleSpells, [](const CSpell *s)
	{
		return spellType(s) != SpellTypes::BATTLE;
	});

	LOGFL("I know how %d of them works.", possibleSpells.size());

	//Get possible spell-target pairs
	std::vector<PossibleSpellcast> possibleCasts;
	for(auto spell : possibleSpells)
	{
		spells::BattleCast temp(getCbc().get(), hero, spells::Mode::HERO, spell);

		for(auto & target : temp.findPotentialTargets())
		{
			PossibleSpellcast ps;
			ps.dest = target;
			ps.spell = spell;
			possibleCasts.push_back(ps);
		}
	}
	LOGFL("Found %d spell-target combinations.", possibleCasts.size());
	if(possibleCasts.empty())
		return;

	using ValueMap = PossibleSpellcast::ValueMap;

	auto evaluateQueue = [&](ValueMap & values, const std::vector<battle::Units> & queue, HypotheticBattle * state)
	{
		for(auto & turn : queue)
		{
			for(auto unit : turn)
			{
				if(vstd::contains(values, unit->unitId()))
					continue;

				PotentialTargets pt(unit, state);

				if(!pt.possibleAttacks.empty())
				{
					AttackPossibility ap = pt.bestAction();

					auto swb = state->getForUpdate(unit->unitId());
					swb->state = *ap.attack.attacker;
					swb->state.position = ap.tile;


					swb = state->getForUpdate(ap.attack.defender->unitId());
					swb->state = *ap.attack.defender;
				}

				auto bav = pt.bestActionValue();

				//best action is from effective owner PoV, we need to convert to our PoV
				if(state->battleGetOwner(unit) != playerID)
					bav = -bav;
				values[unit->unitId()] = bav;
			}
		}
	};

	RNGStub rngStub;

	ValueMap valueOfStack;

	TStacks all = cb->battleGetAllStacks(true);

	auto amount = all.size();

	std::vector<battle::Units> turnOrder;

	cb->battleGetTurnOrder(turnOrder, amount, 2); //no more than 1 turn after current, each unit at least once

	{
		HypotheticBattle state(cb);
		evaluateQueue(valueOfStack, turnOrder, &state);
	}

	auto evaluateSpellcast = [&] (PossibleSpellcast * ps)
	{
		int64_t totalGain = 0;

		HypotheticBattle state(cb);

		spells::BattleCast cast(&state, hero, spells::Mode::HERO, ps->spell);
		cast.target = ps->dest;
		cast.cast(&state, rngStub);

		std::vector<battle::Units> newTurnOrder;
		state.battleGetTurnOrder(newTurnOrder, amount, 2);

		ValueMap newValueOfStack;

		evaluateQueue(newValueOfStack, newTurnOrder, &state);

		for(auto sta : all)
		{
			auto newValue = getValOr(newValueOfStack, sta->unitId(), 0);
			auto oldValue = getValOr(valueOfStack, sta->unitId(), 0);

			auto gain = newValue - oldValue;

			if(gain != 0)
			{
//				LOGFL("%s would change %s by %d points (from %d to %d)",
//					  ps->spell->name % sta->nodeName() % (gain) % (oldValue) % (newValue));
				totalGain += gain;
			}
		}

//		if(totalGain != 0)
//			LOGFL("Total gain of cast %s at hex %d is %d", ps->spell->name % (ps->dest.hex) % (totalGain));

		ps->value = totalGain;
	};

	std::vector<std::function<void()>> tasks;

	for(PossibleSpellcast & psc : possibleCasts)
	{
		tasks.push_back(std::bind(evaluateSpellcast, &psc));

	}

	CThreadHelper threadHelper(&tasks, std::max<uint32_t>(boost::thread::hardware_concurrency(), 1));
	threadHelper.run();

	auto pscValue = [] (const PossibleSpellcast &ps) -> int64_t
	{
		return ps.value;
	};
	auto castToPerform = *vstd::maxElementByFun(possibleCasts, pscValue);

	if(castToPerform.value > 0)
	{
		LOGFL("Best spell is %s. Will cast.", castToPerform.spell->name);
		BattleAction spellcast;
		spellcast.actionType = EActionType::HERO_SPELL;
		spellcast.additionalInfo = castToPerform.spell->id;
		spellcast.setTarget(castToPerform.dest);
		spellcast.side = side;
		spellcast.stackNumber = (!side) ? -1 : -2;
		cb->battleMakeAction(&spellcast);
	}
	else
	{
		LOGFL("Best spell is %s. But it is actually useless (value %d).", castToPerform.spell->name % castToPerform.value);
	}
}

int CBattleAI::distToNearestNeighbour(BattleHex hex, const ReachabilityInfo::TDistances &dists, BattleHex *chosenHex)
{
	int ret = 1000000;
	for(BattleHex n : hex.neighbouringTiles())
	{
		if(dists[n] >= 0 && dists[n] < ret)
		{
			ret = dists[n];
			if(chosenHex)
				*chosenHex = n;
		}
	}
	return ret;
}

void CBattleAI::battleStart(const CCreatureSet *army1, const CCreatureSet *army2, int3 tile, const CGHeroInstance *hero1, const CGHeroInstance *hero2, bool Side)
{
	LOG_TRACE(logAi);
	side = Side;
}

bool CBattleAI::isCloser(const EnemyInfo &ei1, const EnemyInfo &ei2, const ReachabilityInfo::TDistances &dists)
{
	return distToNearestNeighbour(ei1.s->getPosition(), dists) < distToNearestNeighbour(ei2.s->getPosition(), dists);
}

void CBattleAI::print(const std::string &text) const
{
	logAi->trace("%s Battle AI[%p]: %s", playerID.getStr(), this, text);
}

boost::optional<BattleAction> CBattleAI::considerFleeingOrSurrendering()
{
	if(cb->battleCanSurrender(playerID))
	{
	}
	if(cb->battleCanFlee())
	{
	}
	return boost::none;
}



