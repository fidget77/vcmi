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
#include "StackWithBonuses.h"
#include "EnemyInfo.h"
#include "../../lib/spells/CSpellHandler.h"

#define LOGL(text) print(text)
#define LOGFL(text, formattingEl) print(boost::str(boost::format(text) % formattingEl))

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
			cancel.actionType = Battle::CANCEL;
			return cancel;
		}

		if(auto action = considerFleeingOrSurrendering())
			return *action;
		//best action is from effective owner PoV, we are effective owner as we received "activeStack"
		PotentialTargets targets(stack);
		if(targets.possibleAttacks.size())
		{
			auto hlp = targets.bestAction();
			if(hlp.attack.shooting)
				return BattleAction::makeShotAttack(stack, &hlp.enemy);
			else
				return BattleAction::makeMeleeAttack(stack, &hlp.enemy, hlp.tile);
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


enum SpellTypes
{
	OFFENSIVE_SPELL, TIMED_EFFECT, OTHER
};

SpellTypes spellType(const CSpell *spell)
{
	if (spell->isOffensiveSpell())
		return OFFENSIVE_SPELL;
	if (spell->hasEffects())
		return TIMED_EFFECT;
	return OTHER;
}

void CBattleAI::attemptCastingSpell()
{
	//FIXME: support special spell effects (at least damage and timed effects)
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
	{return spellType(s) == OTHER; });
	LOGFL("I know about workings of %d of them.", possibleSpells.size());

	//Get possible spell-target pairs
	std::vector<PossibleSpellcast> possibleCasts;
	for(auto spell : possibleSpells)
	{
		for(auto hex : getTargetsToConsider(spell, hero))
		{
			PossibleSpellcast ps = {spell, hex, 0};
			possibleCasts.push_back(ps);
		}
	}
	LOGFL("Found %d spell-target combinations.", possibleCasts.size());
	if(possibleCasts.empty())
		return;

	using ValueMap = std::map<const CStack *, int>;

	auto evaluateQueue = [&](ValueMap & values, const TStacks & queue, HypotheticBattle & state)
	{
		for(const CStack * stack : queue)
		{
			if(vstd::contains(values, stack))
				break;

			PotentialTargets pt(stack, state);

			if(!pt.possibleAttacks.empty())
			{
				AttackPossibility ap = pt.bestAction();

				auto swb = getValOr(state.stackStates, stack->unitId(), std::make_shared<StackWithBonuses>(&stack->stackState));
				swb->state = ap.attack.attacker;
				swb->state.position = ap.tile;
				state.stackStates[stack->unitId()] = swb;

				swb = getValOr(state.stackStates, ap.attack.defender.unitId(), std::make_shared<StackWithBonuses>(&ap.attack.defender));
				swb->state = ap.attack.defender;
				state.stackStates[ap.attack.defender.unitId()] = swb;
			}

			auto bav = pt.bestActionValue();
			const IStackState * info = stack;

			//was stack actually changed?
			auto iter = state.stackStates.find(stack->unitId());
			if(iter != state.stackStates.end())
				info = &iter->second->state;

			//best action is from effective owner PoV, we need to convert to our PoV
			if(getCbc()->battleGetOwner(info) != playerID)
				bav = -bav;
			values[stack] = bav;
		}
	};

	ValueMap valueOfStack;

	TStacks all = cb->battleGetAllStacks(true);

	auto amount = all.size();

	TStacks queue;
	cb->battleGetStackQueue(queue, amount);

	{
		HypotheticBattle state(cb);
		evaluateQueue(valueOfStack, queue, state);
	}

	auto evaluateSpellcast = [&] (const PossibleSpellcast &ps) -> double
	{
		const int skillLevel = hero->getSpellSchoolLevel(spells::Mode::HERO, ps.spell);
		const int spellPower = hero->getPrimSkillLevel(PrimarySkill::SPELL_POWER);

		int totalGain = 0;
		int32_t damageDiff = 0;

		//TODO: calculate stack state changes inside spell susbsystem

		HypotheticBattle state(cb);

		auto stacksAffected = ps.spell->getAffectedStacks(cb.get(), spells::Mode::HERO, hero, skillLevel, ps.dest);

		if(stacksAffected.empty())
			return -1;

		for(const CStack * sta : stacksAffected)
		{
			auto swb = std::make_shared<StackWithBonuses>(&sta->stackState);

			switch(spellType(ps.spell))
			{
			case OFFENSIVE_SPELL:
				{
					int32_t dmg = ps.spell->calculateDamage(hero, sta, skillLevel, spellPower);
					if(sta->owner == playerID)
						dmg *= 10;

					swb->state.damage(dmg);

					//we try to avoid damage to our stacks even if they are mind-controlled
					if(sta->owner == playerID)
						damageDiff -= dmg;
					else
						damageDiff += dmg;

					//TODO tactic effect too
				}
				break;
			case TIMED_EFFECT:
				{
					ps.spell->getEffects(swb->bonusesToUpdate, skillLevel, false, hero->getEnchantPower(spells::Mode::HERO, ps.spell));
					ps.spell->getEffects(swb->bonusesToAdd, skillLevel, true, hero->getEnchantPower(spells::Mode::HERO, ps.spell));
				}
				break;
			default:
				return -1;
			}

			state.stackStates[sta->unitId()] = swb;
		}

		ValueMap newValueOfStack;

		//FIXME: calculate new queue on hypothetic states
		evaluateQueue(newValueOfStack, queue, state);

		for(auto sta : all)
		{
			auto newValue = getValOr(newValueOfStack, sta, 0);
			auto oldValue = getValOr(valueOfStack, sta, 0);

			auto gain = newValue - oldValue;

			if(gain != 0)
			{
				LOGFL("%s would change %s by %d points (from %d to %d)",
					  ps.spell->name % sta->nodeName() % (gain) % (oldValue) % (newValue));
				totalGain += gain;
			}
		}

		if(totalGain != 0)
			LOGFL("Total gain of cast %s at hex %d is %d", ps.spell->name % (ps.dest.hex) % (totalGain));

		if(damageDiff < 0)
		{
			//ignore gain if we receiving more damage than giving
			return (float)-1;//a bit worse than nothing
		}
		else
		{
			//return gain, but add damage diff as fractional part
			return (float)totalGain + atan((float)damageDiff)/M_PI;
		}
	};

	for(PossibleSpellcast & psc : possibleCasts)
		psc.value = evaluateSpellcast(psc);
	auto pscValue = [] (const PossibleSpellcast &ps) -> float
	{
		return ps.value;
	};
	auto castToPerform = *vstd::maxElementByFun(possibleCasts, pscValue);

	if(castToPerform.value > 0)
	{
		LOGFL("Best spell is %s. Will cast.", castToPerform.spell->name);
		BattleAction spellcast;
		spellcast.actionType = Battle::HERO_SPELL;
		spellcast.additionalInfo = castToPerform.spell->id;
		spellcast.destinationTile = castToPerform.dest;
		spellcast.side = side;
		spellcast.stackNumber = (!side) ? -1 : -2;
		cb->battleMakeAction(&spellcast);
	}
	else
	{
		LOGFL("Best spell is %s. But it is actually useless (value %f).", castToPerform.spell->name % castToPerform.value);
	}
}

std::vector<BattleHex> CBattleAI::getTargetsToConsider(const CSpell * spell, const spells::Caster * caster) const
{
	//todo: move to CSpell
	const CSpell::TargetInfo targetInfo(spell, caster->getSpellSchoolLevel(spells::Mode::HERO, spell), spells::Mode::HERO);
	std::vector<BattleHex> ret;
	if(targetInfo.massive || targetInfo.type == CSpell::NO_TARGET)
	{
		ret.push_back(BattleHex());
	}
	else
	{
		switch(targetInfo.type)
		{
		case CSpell::CREATURE:
		case CSpell::LOCATION:
			for(int i = 0; i < GameConstants::BFIELD_SIZE; i++)
			{
				BattleHex dest(i);
				if(dest.isAvailable())
					if(spell->canBeCastAt(getCbc().get(), spells::Mode::HERO, caster, dest))
						ret.push_back(i);
			}
			break;
		default:
			break;
		}
	}
	return ret;
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



