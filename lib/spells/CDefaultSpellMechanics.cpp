/*
 * CDefaultSpellMechanics.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"

#include "CDefaultSpellMechanics.h"

#include "../CStack.h"
#include "../battle/BattleInfo.h"

#include "../CGeneralTextHandler.h"

namespace spells
{
namespace SRSLPraserHelpers
{
	static int XYToHex(int x, int y)
	{
		return x + GameConstants::BFIELD_WIDTH * y;
	}

	static int XYToHex(std::pair<int, int> xy)
	{
		return XYToHex(xy.first, xy.second);
	}

	static int hexToY(int battleFieldPosition)
	{
		return battleFieldPosition/GameConstants::BFIELD_WIDTH;
	}

	static int hexToX(int battleFieldPosition)
	{
		int pos = battleFieldPosition - hexToY(battleFieldPosition) * GameConstants::BFIELD_WIDTH;
		return pos;
	}

	static std::pair<int, int> hexToPair(int battleFieldPosition)
	{
		return std::make_pair(hexToX(battleFieldPosition), hexToY(battleFieldPosition));
	}

	//moves hex by one hex in given direction
	//0 - left top, 1 - right top, 2 - right, 3 - right bottom, 4 - left bottom, 5 - left
	static std::pair<int, int> gotoDir(int x, int y, int direction)
	{
		switch(direction)
		{
		case 0: //top left
			return std::make_pair((y%2) ? x-1 : x, y-1);
		case 1: //top right
			return std::make_pair((y%2) ? x : x+1, y-1);
		case 2:  //right
			return std::make_pair(x+1, y);
		case 3: //right bottom
			return std::make_pair((y%2) ? x : x+1, y+1);
		case 4: //left bottom
			return std::make_pair((y%2) ? x-1 : x, y+1);
		case 5: //left
			return std::make_pair(x-1, y);
		default:
			throw std::runtime_error("Disaster: wrong direction in SRSLPraserHelpers::gotoDir!\n");
		}
	}

	static std::pair<int, int> gotoDir(std::pair<int, int> xy, int direction)
	{
		return gotoDir(xy.first, xy.second, direction);
	}

	static bool isGoodHex(std::pair<int, int> xy)
	{
		return xy.first >=0 && xy.first < GameConstants::BFIELD_WIDTH && xy.second >= 0 && xy.second < GameConstants::BFIELD_HEIGHT;
	}

	//helper function for rangeInHexes
	static std::set<ui16> getInRange(unsigned int center, int low, int high)
	{
		std::set<ui16> ret;
		if(low == 0)
		{
			ret.insert(center);
		}

		std::pair<int, int> mainPointForLayer[6]; //A, B, C, D, E, F points
		for(auto & elem : mainPointForLayer)
			elem = hexToPair(center);

		for(int it=1; it<=high; ++it) //it - distance to the center
		{
			for(int b=0; b<6; ++b)
				mainPointForLayer[b] = gotoDir(mainPointForLayer[b], b);

			if(it>=low)
			{
				std::pair<int, int> curHex;

				//adding lines (A-b, B-c, C-d, etc)
				for(int v=0; v<6; ++v)
				{
					curHex = mainPointForLayer[v];
					for(int h=0; h<it; ++h)
					{
						if(isGoodHex(curHex))
							ret.insert(XYToHex(curHex));
						curHex = gotoDir(curHex, (v+2)%6);
					}
				}

			} //if(it>=low)
		}

		return ret;
	}
}

SpellCastContext::SpellCastContext(const Mechanics * mechanics_, const SpellCastEnvironment * env_, const BattleCast & parameters_):
	mechanics(mechanics_), env(env_), attackedCres(), sc(), si(), parameters(parameters_), otherHero(nullptr), spellCost(0), damageToDisplay(0)
{
	sc.side = mechanics->casterSide;
	sc.id = mechanics->owner->id;
	sc.skill = parameters.spellLvl;
	sc.tile = parameters.getFirstDestinationHex();
	sc.castByHero = parameters.mode == Mode::HERO;
	sc.casterStack = (mechanics->casterStack ? mechanics->casterStack->ID : -1);
	sc.manaGained = 0;

	//check it there is opponent hero
	const ui8 otherSide = 1-mechanics->casterSide;

	if(mechanics->cb->battleHasHero(otherSide))
		otherHero = mechanics->cb->battleGetFightingHero(otherSide);

	logGlobal->debug("Started spell cast. Spell: %s; mode: %d", mechanics->owner->name, static_cast<int>(parameters.mode));
}

SpellCastContext::~SpellCastContext()
{
	logGlobal->debug("Finished spell cast. Spell: %s; mode: %d", mechanics->owner->name, static_cast<int>(parameters.mode));
}

void SpellCastContext::addDamageToDisplay(const si32 value)
{
	damageToDisplay += value;
}

void SpellCastContext::setDamageToDisplay(const si32 value)
{
	damageToDisplay = value;
}

si32 SpellCastContext::getDamageToDisplay() const
{
	return damageToDisplay;
}

void SpellCastContext::addBattleLog(MetaString && line)
{
	sc.battleLog.push_back(line);
}

void SpellCastContext::addCustomEffect(const CStack * target, ui32 effect)
{
	BattleSpellCast::CustomEffect customEffect;
	customEffect.effect = effect;
	customEffect.stack = target->ID;
	sc.customEffects.push_back(customEffect);
}

void SpellCastContext::beforeCast()
{
	//calculate spell cost
	if(mechanics->mode == Mode::HERO)
	{
		auto casterHero = dynamic_cast<const CGHeroInstance *>(mechanics->caster);
		spellCost = mechanics->cb->battleGetSpellCost(mechanics->owner, casterHero);

		if(nullptr != otherHero) //handle mana channel
		{
			int manaChannel = 0;
			for(const CStack * stack : mechanics->cb->battleGetAllStacks(true)) //TODO: shouldn't bonus system handle it somehow?
			{
				if(stack->owner == otherHero->tempOwner)
				{
					vstd::amax(manaChannel, stack->valOfBonuses(Bonus::MANA_CHANNELING));
				}
			}
			sc.manaGained = (manaChannel * spellCost) / 100;
		}
	}

	sc.activeCast = parameters.mode == ECastingMode::HERO_CASTING ||
		parameters.mode == ECastingMode::CREATURE_ACTIVE_CASTING ||
		parameters.mode == ECastingMode::ENCHANTER_CASTING;
}

void SpellCastContext::cast()
{
	DefaultSpellMechanics::doRemoveEffects(env, *this, std::bind(&SpellCastContext::counteringSelector, this, _1));

	for(auto sta : attackedCres)
		sc.affectedCres.insert(sta->ID);

	env->sendAndApply(&sc);

	if(!si.stacks.empty()) //after spellcast info shows
		env->sendAndApply(&si);
}

void SpellCastContext::afterCast()
{
	//todo: should this be before applyBattleEffects?


	if(mechanics->mode == Mode::HERO)
	{
		mechanics->caster->spendMana(mechanics->mode, mechanics->owner, env, spellCost);

		if(sc.manaGained > 0)
		{
			assert(otherHero);
			otherHero->spendMana(Mode::HERO, mechanics->owner, env, -sc.manaGained);
		}
	}
	else if(mechanics->mode == Mode::CREATURE_ACTIVE || mechanics->mode == Mode::ENCHANTER)
	{
		mechanics->caster->spendMana(mechanics->mode, mechanics->owner, env, 1);
	}
}

bool SpellCastContext::counteringSelector(const Bonus * bonus) const
{
	if(bonus->source != Bonus::SPELL_EFFECT)
		return false;

	for(const SpellID & id : mechanics->owner->counteredSpells)
	{
		if(bonus->sid == id.toEnum())
			return true;
	}

	return false;
}


///DefaultSpellMechanics
DefaultSpellMechanics::DefaultSpellMechanics(const CSpell * s, const CBattleInfoCallback * Cb, const Caster * caster_)
	: BaseMechanics(s, Cb, caster_)
{
};

void DefaultSpellMechanics::applyEffects(const SpellCastEnvironment * env, const BattleCast & parameters) const
{
	Target targets;

	for(const Destination & dest : parameters.target)
	{
		if(dest.stackValue)
		{
			if(!isImmuneByStack(dest.stackValue))
				targets.push_back(dest);
		}
		else
		{
			targets.push_back(dest);
		}
	}

	applyEffectsForced(env, parameters, targets);
}

void DefaultSpellMechanics::applyEffectsForced(const SpellCastEnvironment * env, const BattleCast & parameters) const
{
	applyEffectsForced(env, parameters, parameters.target);
}

void DefaultSpellMechanics::applyEffectsForced(const SpellCastEnvironment * env, const BattleCast & parameters, const Target & targets) const
{
	std::vector<const CStack *> stacks;
	for(auto & dest : targets)
	{
		if(dest.stackValue)
			stacks.push_back(dest.stackValue);
	}

	if(owner->isDamageSpell())
	{
		StacksInjured si;
		defaultDamageEffect(env, parameters, si, stacks);
		if(!si.stacks.empty())
			env->sendAndApply(&si);
	}

	if(owner->hasEffects())
	{
		SetStackEffect sse;
		defaultTimedEffect(env, parameters, sse, stacks);
		if(!sse.stacks.empty())
			env->sendAndApply(&sse);
	}
}

int DefaultSpellMechanics::defaultDamageEffect(const SpellCastEnvironment * env, const BattleCast & parameters, StacksInjured & si, const TStacks & target) const
{
	int totalDamage = 0;
	const int rawDamage = parameters.getEffectValue();
	for(auto & attackedCre : target)
	{
		BattleStackAttacked bsa;
		bsa.damageAmount = owner->adjustRawDamage(caster, attackedCre, rawDamage);
		totalDamage += bsa.damageAmount;

		bsa.stackAttacked = attackedCre->ID;
		bsa.attackerID = -1;
		attackedCre->prepareAttacked(bsa, env->getRandomGenerator());
		si.stacks.push_back(bsa);
	}

	return totalDamage;
}

void DefaultSpellMechanics::defaultTimedEffect(const SpellCastEnvironment * env, const BattleCast & parameters, SetStackEffect & sse, const TStacks & target) const
{
	//get default spell duration (spell power with bonuses for heroes)
	int duration = parameters.effectDuration;
	//generate actual stack bonuses
	{
		si32 maxDuration = 0;

		owner->getEffects(sse.effect, parameters.effectLevel, false, duration, &maxDuration);
		owner->getEffects(sse.cumulativeEffects, parameters.effectLevel, true, duration, &maxDuration);

		//if all spell effects have special duration, use it later for special bonuses
		duration = maxDuration;
	}

	//we need to know who cast Bind
	if(owner->id == SpellID::BIND && casterStack)
	{
		sse.effect.at(sse.effect.size() - 1).additionalInfo = casterStack->ID;
	}
	std::shared_ptr<Bonus> bonus = nullptr;
	auto casterHero = dynamic_cast<const CGHeroInstance *>(caster);
	if(casterHero)
		bonus = casterHero->getBonusLocalFirst(Selector::typeSubtype(Bonus::SPECIAL_PECULIAR_ENCHANT, owner->id));
	//TODO does hero specialty should affects his stack casting spells?

	for(const CStack * affected : target)
	{
		si32 power = 0;
		sse.stacks.push_back(affected->ID);

		//Apply hero specials - peculiar enchants
		const ui8 tier = std::max((ui8)1, affected->getCreature()->level); //don't divide by 0 for certain creatures (commanders, war machines)
		if(bonus)
		{
			switch(bonus->additionalInfo)
			{
			case 0: //normal
				{
					switch(tier)
					{
					case 1:
					case 2:
						power = 3;
						break;
					case 3:
					case 4:
						power = 2;
						break;
					case 5:
					case 6:
						power = 1;
						break;
					}
					for(const Bonus & b : sse.effect)
					{
						Bonus specialBonus(b);
						specialBonus.val = power; //it doesn't necessarily make sense for some spells, use it wisely
						specialBonus.turnsRemain = duration;
						sse.uniqueBonuses.push_back(std::pair<ui32, Bonus>(affected->ID, specialBonus)); //additional premy to given effect
					}
					for(const Bonus & b : sse.cumulativeEffects)
					{
						Bonus specialBonus(b);
						specialBonus.val = power; //it doesn't necessarily make sense for some spells, use it wisely
						specialBonus.turnsRemain = duration;
						sse.cumulativeUniqueBonuses.push_back(std::pair<ui32, Bonus>(affected->ID, specialBonus)); //additional premy to given effect
					}
				}
				break;
			case 1: //only Coronius as yet
				{
					power = std::max(5 - tier, 0);
					Bonus specialBonus(Bonus::N_TURNS, Bonus::PRIMARY_SKILL, Bonus::SPELL_EFFECT, power, owner->id, PrimarySkill::ATTACK);
					specialBonus.turnsRemain = duration;
					sse.uniqueBonuses.push_back(std::pair<ui32,Bonus>(affected->ID, specialBonus)); //additional attack to Slayer effect
				}
				break;
			}
		}
		if(casterHero && casterHero->hasBonusOfType(Bonus::SPECIAL_BLESS_DAMAGE, owner->id)) //TODO: better handling of bonus percentages
		{
			int damagePercent = casterHero->level * casterHero->valOfBonuses(Bonus::SPECIAL_BLESS_DAMAGE, owner->id.toEnum()) / tier;
			Bonus specialBonus(Bonus::N_TURNS, Bonus::CREATURE_DAMAGE, Bonus::SPELL_EFFECT, damagePercent, owner->id, 0, Bonus::PERCENT_TO_ALL);
			specialBonus.turnsRemain = duration;
			sse.uniqueBonuses.push_back(std::pair<ui32,Bonus>(affected->ID, specialBonus));
		}
	}
}

std::vector<BattleHex> DefaultSpellMechanics::rangeInHexes(BattleHex centralHex, ui8 schoolLvl, bool *outDroppedHexes) const
{
	using namespace SRSLPraserHelpers;

	std::vector<BattleHex> ret;
	std::string rng = owner->getLevelInfo(schoolLvl).range + ','; //copy + artificial comma for easier handling

	if(rng.size() >= 2 && rng[0] != 'X') //there is at least one hex in range (+artificial comma)
	{
		std::string number1, number2;
		int beg, end;
		bool readingFirst = true;
		for(auto & elem : rng)
		{
			if(std::isdigit(elem) ) //reading number
			{
				if(readingFirst)
					number1 += elem;
				else
					number2 += elem;
			}
			else if(elem == ',') //comma
			{
				//calculating variables
				if(readingFirst)
				{
					beg = atoi(number1.c_str());
					number1 = "";
				}
				else
				{
					end = atoi(number2.c_str());
					number2 = "";
				}
				//obtaining new hexes
				std::set<ui16> curLayer;
				if(readingFirst)
				{
					curLayer = getInRange(centralHex, beg, beg);
				}
				else
				{
					curLayer = getInRange(centralHex, beg, end);
					readingFirst = true;
				}
				//adding obtained hexes
				for(auto & curLayer_it : curLayer)
				{
					ret.push_back(curLayer_it);
				}

			}
			else if(elem == '-') //dash
			{
				beg = atoi(number1.c_str());
				number1 = "";
				readingFirst = false;
			}
		}
	}

	//remove duplicates (TODO check if actually needed)
	range::unique(ret);
	return ret;
}

bool DefaultSpellMechanics::canBeCast(Problem & problem) const
{
	//check for creature target existence
	//allow to cast spell if there is at least one smart target
	if(requiresCreatureTarget())
	{
		CSpell::TargetInfo tinfo(owner, caster->getSpellSchoolLevel(mode, owner), mode);
		bool targetExists = false;

		for(const CStack * stack : cb->battleGetAllStacks())
		{
			const bool immune = !(stack->isValidTarget(!tinfo.onlyAlive)) || isImmuneByStack(stack);

			switch(mode)
			{
			case spells::Mode::HERO:
			case spells::Mode::CREATURE_ACTIVE:
			case spells::Mode::ENCHANTER:
			case spells::Mode::PASSIVE:
				{
					const bool ownerMatches = cb->battleMatchOwner(caster->getOwner(), stack, owner->getPositiveness());
					targetExists = !immune && ownerMatches;
				}
				break;
			default:
				targetExists = !immune;
				break;
			}

			if(targetExists)
				break;
		}

		if(!targetExists)
			return adaptProblem(ESpellCastProblem::NO_APPROPRIATE_TARGET, problem);
	}
	return true;
}

bool DefaultSpellMechanics::isImmuneByStack(const CStack * obj) const
{
	//by default use general algorithm
	return owner->internalIsImmune(caster, obj);
}

bool DefaultSpellMechanics::dispellSelector(const Bonus * bonus)
{
	if(bonus->source == Bonus::SPELL_EFFECT)
	{
		const CSpell * sourceSpell = SpellID(bonus->sid).toSpell();
		if(!sourceSpell)
			return false;//error
		//Special case: DISRUPTING_RAY is "immune" to dispell
		//Other even PERMANENT effects can be removed (f.e. BIND)
		if(sourceSpell->id == SpellID::DISRUPTING_RAY)
			return false;
		//Special case:do not remove lifetime marker
		if(sourceSpell->id == SpellID::CLONE)
			return false;
		//stack may have inherited effects
		if(sourceSpell->isAdventureSpell())
			return false;
		return true;
	}
	//not spell effect
	return false;
}

void DefaultSpellMechanics::doDispell(const SpellCastEnvironment * env, SpellCastContext & ctx, const CSelector & selector) const
{
	doRemoveEffects(env, ctx, selector.And(dispellSelector));
}

bool DefaultSpellMechanics::canDispell(const IBonusBearer * obj, const CSelector & selector, const std::string & cachingStr) const
{
	return obj->hasBonus(selector.And(dispellSelector), Selector::all, cachingStr);
}

void DefaultSpellMechanics::doRemoveEffects(const SpellCastEnvironment * env, SpellCastContext & ctx, const CSelector & selector)
{
	SetStackEffect sse;

	for(const CStack * s : ctx.attackedCres)
	{
		std::vector<Bonus> buffer;
		auto bl = s->getBonuses(selector);

		for(auto item : *bl)
			buffer.emplace_back(*item);

		if(!buffer.empty())
			sse.toRemove.push_back(std::make_pair(s->ID, buffer));
	}

	if(!sse.toRemove.empty())
		env->sendAndApply(&sse);
}

void DefaultSpellMechanics::handleMagicMirror(const SpellCastEnvironment * env, SpellCastContext & ctx, std::vector <const CStack*> & reflected) const
{
	//reflection is applied only to negative spells
	//if it is actual spell and can be reflected to single target, no recurrence
	const bool tryMagicMirror = owner->isNegative() && owner->level && owner->getLevelInfo(0).range == "0";
	if(tryMagicMirror)
	{
		for(auto s : ctx.attackedCres)
		{
			const int mirrorChance = (s)->valOfBonuses(Bonus::MAGIC_MIRROR);
			if(env->getRandomGenerator().nextInt(99) < mirrorChance)
				reflected.push_back(s);
		}

		vstd::erase_if(ctx.attackedCres, [&reflected](const CStack * s)
		{
			return vstd::contains(reflected, s);
		});

		for(auto s : reflected)
			ctx.addCustomEffect(s, 3);
	}
}

void DefaultSpellMechanics::handleResistance(const SpellCastEnvironment * env, SpellCastContext & ctx) const
{
	//checking if creatures resist
	//resistance is applied only to negative spells
	if(owner->isNegative())
	{
		std::vector <const CStack*> resisted;
		for(auto s : ctx.attackedCres)
		{
			//magic resistance
			const int prob = std::min((s)->magicResistance(), 100); //probability of resistance in %

			if(env->getRandomGenerator().nextInt(99) < prob)
			{
				resisted.push_back(s);
			}
		}

		vstd::erase_if(ctx.attackedCres, [&resisted](const CStack * s)
		{
			return vstd::contains(resisted, s);
		});

		for(auto s : resisted)
			ctx.addCustomEffect(s, 78);
	}
}

bool DefaultSpellMechanics::requiresCreatureTarget() const
{
	//most spells affects creatures somehow regardless of Target Type
	//for few exceptions see overrides
	return true;
}

///RegularSpellMechanics
RegularSpellMechanics::RegularSpellMechanics(const CSpell * s, const CBattleInfoCallback * Cb, const Caster * caster_)
	: DefaultSpellMechanics(s, Cb, caster_)
{

}

void RegularSpellMechanics::applyBattleEffects(const SpellCastEnvironment * env, const BattleCast & parameters, SpellCastContext & ctx) const
{
	if(owner->isOffensiveSpell())
	{
		ctx.addDamageToDisplay(defaultDamageEffect(env, parameters, ctx.si, ctx.attackedCres));
	}

	if(owner->hasEffects())
	{
		SetStackEffect sse;
		defaultTimedEffect(env, parameters, sse, ctx.attackedCres);

		if(!sse.stacks.empty())
			env->sendAndApply(&sse);
	}
}

std::vector<const CStack *> RegularSpellMechanics::calculateAffectedStacks(int spellLvl, BattleHex destination) const
{
	std::set<const CStack *> attackedCres;//std::set to exclude multiple occurrences of two hex creatures
	CSpell::TargetInfo ti(owner, spellLvl, mode);
	auto attackedHexes = rangeInHexes(destination, spellLvl);

	//hackfix for banned creature massive spells
	if(!ti.massive && owner->getLevelInfo(spellLvl).range == "X")
		attackedHexes.push_back(destination);

	auto mainFilter = [&](const CStack * s)
	{
		const bool ownerMatches = cb->battleMatchOwner(caster->getOwner(), s, owner->getPositiveness());
		const bool validTarget = s->isValidTarget(!ti.onlyAlive); //todo: this should be handled by spell class
		const bool positivenessFlag = !ti.smart || ownerMatches;

		return positivenessFlag && validTarget;
	};

	if(ti.type == CSpell::CREATURE && attackedHexes.size() == 1)
	{
		//for single target spells we must select one target. Alive stack is preferred (issue #1763)

		auto predicate = [&](const CStack * s)
		{
			return s->coversPos(attackedHexes.at(0)) && mainFilter(s);
		};

		TStacks stacks = cb->battleGetStacksIf(predicate);

		for(auto stack : stacks)
		{
			if(stack->alive())
			{
				attackedCres.insert(stack);
				break;
			}
		}

		if(attackedCres.empty() && !stacks.empty())
			attackedCres.insert(stacks.front());
	}
	else if(ti.massive)
	{
		TStacks stacks = cb->battleGetStacksIf(mainFilter);
		for(auto stack : stacks)
			attackedCres.insert(stack);
	}
	else //custom range from attackedHexes
	{
		for(BattleHex hex : attackedHexes)
		{
			if(const CStack * st = cb->battleGetStackByPos(hex, ti.onlyAlive))
				if(mainFilter(st))
					attackedCres.insert(st);
		}
	}

	std::vector<const CStack *> res;
	std::copy(attackedCres.begin(), attackedCres.end(), std::back_inserter(res));

	return res;
}

bool RegularSpellMechanics::canBeCastAt(BattleHex destination) const
{
	const auto level = caster->getSpellSchoolLevel(mode, owner);
	std::vector<const CStack *> affected = getAffectedStacks(level, destination);

	bool targetExists = false;

	switch(mode)
	{
	case spells::Mode::HERO:
	case spells::Mode::CREATURE_ACTIVE:
	case spells::Mode::ENCHANTER:
	case spells::Mode::PASSIVE:
		//allow to cast spell if it affects at least one smart target

		for(const CStack * stack : affected)
		{
			targetExists = cb->battleMatchOwner(caster->getOwner(), stack, owner->getPositiveness());
			if(targetExists)
				break;
		}

		break;
	default:
		targetExists = !affected.empty();
		break;
	}

	return targetExists;
}

void RegularSpellMechanics::cast(const SpellCastEnvironment * env, const BattleCast & parameters, SpellCastContext & ctx, std::vector<const CStack*> & reflected) const
{
	ctx.attackedCres = getAffectedStacks(parameters.spellLvl, parameters.getFirstDestinationHex());

	logGlobal->debug("will affect %d stacks", ctx.attackedCres.size());

	handleResistance(env, ctx);

	if(mode != Mode::MAGIC_MIRROR)
		handleMagicMirror(env, ctx, reflected);

	applyBattleEffects(env, parameters, ctx);
	prepareBattleLog(ctx);
	ctx.cast();
}

std::vector<const CStack *> RegularSpellMechanics::getAffectedStacks(int spellLvl, BattleHex destination) const
{
	std::vector<const CStack *> result = calculateAffectedStacks(spellLvl, destination);
	CSpell::TargetInfo ti(owner, spellLvl, mode);

	auto predicate = [&, this](const CStack * s)->bool
	{
		const bool hitDirectly = ti.alwaysHitDirectly && s->coversPos(destination);
		const bool immune = isImmuneByStack(s);
		return !hitDirectly && immune;
	};
	vstd::erase_if(result, predicate);
	return result;
}

void RegularSpellMechanics::prepareBattleLog(SpellCastContext & ctx) const
{
	if(!ctx.si.battleLog.empty())
		return;

	auto battleLogDefault = [this, &ctx]()
	{
		MetaString line;
		caster->getCastDescription(owner, ctx.attackedCres, line);
		ctx.addBattleLog(std::move(line));
	};

	bool displayDamage = true;

	if(ctx.attackedCres.size() != 1)
	{
		battleLogDefault();
		return;
	}

	auto attackedStack = ctx.attackedCres.at(0);

	auto addLogLine = [attackedStack, &ctx](const int baseTextID, const boost::logic::tribool & plural)
	{
		MetaString line;
		attackedStack->addText(line, MetaString::GENERAL_TXT, baseTextID, plural);
		attackedStack->addNameReplacement(line, plural);
		ctx.addBattleLog(std::move(line));
	};

	displayDamage = false; //in most following cases damage info text is custom

	switch(owner->id)
	{
	case SpellID::STONE_GAZE:
		addLogLine(558, boost::logic::indeterminate);
		break;
	case SpellID::POISON:
		addLogLine(561, boost::logic::indeterminate);
		break;
	case SpellID::BIND:
		addLogLine(-560, true);//"Roots and vines bind the %s to the ground!"
		break;
	case SpellID::DISEASE:
		addLogLine(553, boost::logic::indeterminate);
		break;
	case SpellID::PARALYZE:
		addLogLine(563, boost::logic::indeterminate);
		break;
	case SpellID::AGE:
		{
			//"The %s shrivel with age, and lose %d hit points."
			MetaString line;
			attackedStack->addText(line, MetaString::GENERAL_TXT, 551);
			attackedStack->addNameReplacement(line);

			//todo: display effect from only this cast
			TBonusListPtr bl = attackedStack->getBonuses(Selector::type(Bonus::STACK_HEALTH));
			const int fullHP = bl->totalValue();
			bl->remove_if(Selector::source(Bonus::SPELL_EFFECT, SpellID::AGE));
			line.addReplacement(fullHP - bl->totalValue());
			ctx.addBattleLog(std::move(line));
		}
		break;
	case SpellID::THUNDERBOLT:
		{
			addLogLine(-367, true);
			MetaString line;
			//todo: handle newlines in metastring
			std::string text = VLC->generaltexth->allTexts[343].substr(1, VLC->generaltexth->allTexts[343].size() - 1); //Does %d points of damage.
			line << text;
			line.addReplacement(ctx.getDamageToDisplay()); //no more text afterwards
			ctx.addBattleLog(std::move(line));
		}
		break;
	case SpellID::DISPEL_HELPFUL_SPELLS:
		addLogLine(-555, true);
		break;
	default:
		displayDamage = true;
		battleLogDefault();
		break;
	}

	displayDamage = displayDamage && ctx.getDamageToDisplay() > 0;

	if(displayDamage)
	{
		MetaString line;

		line.addTxt(MetaString::GENERAL_TXT, 376);
		line.addReplacement(MetaString::SPELL_NAME, owner->id.toEnum());
		line.addReplacement(ctx.getDamageToDisplay());

		ctx.addBattleLog(std::move(line));
	}
}


///SpecialSpellMechanics
SpecialSpellMechanics::SpecialSpellMechanics(const CSpell * s, const CBattleInfoCallback * Cb, const Caster * caster_)
	: DefaultSpellMechanics(s, Cb, caster_)
{
}

bool SpecialSpellMechanics::canBeCastAt(BattleHex destination) const
{
	//no problems by default
	//common problems handled by CSpell
	return true;
}

std::vector<const CStack *> SpecialSpellMechanics::getAffectedStacks(int spellLvl, BattleHex destination) const
{
	return std::vector<const CStack *>();
}

void SpecialSpellMechanics::cast(const SpellCastEnvironment * env, const BattleCast & parameters, SpellCastContext & ctx, std::vector<const CStack*> & reflected) const
{
	reflected.clear();

	MetaString line;
	caster->getCastDescription(owner, line);
	ctx.addBattleLog(std::move(line));

	ctx.cast();

	applyBattleEffects(env, parameters, ctx);
}


} // namespace spells
