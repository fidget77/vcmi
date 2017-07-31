/*
 * BattleSpellMechanics.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "BattleSpellMechanics.h"

#include "../NetPacks.h"
#include "../CStack.h"
#include "../battle/BattleInfo.h"
#include "../mapObjects/CGHeroInstance.h"
#include "../mapObjects/CGTownInstance.h"



namespace spells
{

///HealingSpellMechanics
HealingSpellMechanics::HealingSpellMechanics(const CSpell * s, const CBattleInfoCallback * Cb, const Caster * caster_)
	: RegularSpellMechanics(s, Cb, caster_)
{
}

void HealingSpellMechanics::applyBattleEffects(const SpellCastEnvironment * env, const BattleCast & parameters, SpellCastContext & ctx) const
{
	EHealLevel healLevel = getHealLevel(parameters.effectLevel);
	EHealPower healPower = getHealPower(parameters.effectLevel);

	int hpGained = calculateHealedHP(env, parameters, ctx);
	StacksHealedOrResurrected shr;
	shr.lifeDrain = false;
	shr.tentHealing = false;

	//special case for Archangel
	bool cure = mode == Mode::CREATURE_ACTIVE && owner->id == SpellID::RESURRECTION;

	for(auto & attackedCre : ctx.attackedCres)
	{
		int32_t stackHPgained = caster->getSpellBonus(owner, hpGained, attackedCre);
		CHealth health = attackedCre->healthAfterHealed(stackHPgained, healLevel, healPower);

		CHealthInfo hi;
		health.toInfo(hi);
		hi.stackId = attackedCre->ID;
		hi.delta = stackHPgained;
		shr.healedStacks.push_back(hi);
	}
	if(!shr.healedStacks.empty())
		env->sendAndApply(&shr);

	if(cure)
		doDispell(env, ctx, cureSelector);
}

bool HealingSpellMechanics::cureSelector(const Bonus * b)
{
	if(b->source == Bonus::SPELL_EFFECT)
	{
		const CSpell * sourceSpell = SpellID(b->sid).toSpell();
		if(!sourceSpell)
			return false;
		return sourceSpell->isNegative();
	}
	else
		return false;
}

int HealingSpellMechanics::calculateHealedHP(const SpellCastEnvironment * env, const BattleCast & parameters, SpellCastContext & ctx) const
{
	return parameters.getEffectValue();
}

///AntimagicMechanics
AntimagicMechanics::AntimagicMechanics(const CSpell * s, const CBattleInfoCallback * Cb, const Caster * caster_)
	: RegularSpellMechanics(s, Cb, caster_)
{
}

void AntimagicMechanics::applyBattleEffects(const SpellCastEnvironment * env, const BattleCast & parameters, SpellCastContext & ctx) const
{
	doDispell(env, ctx, [this](const Bonus * b) -> bool
	{
		if(b->source == Bonus::SPELL_EFFECT)
		{
			const CSpell * sourceSpell = SpellID(b->sid).toSpell();
			if(!sourceSpell)
				return false;//error
			//keep positive effects
			if(sourceSpell->isPositive())
				return false;
			//keep own effects
			if(sourceSpell == owner)
				return false;
			//remove all others
			return true;
		}

		return false; //not a spell effect
	});

	RegularSpellMechanics::applyBattleEffects(env, parameters, ctx);
}

///ChainLightningMechanics
ChainLightningMechanics::ChainLightningMechanics(const CSpell * s, const CBattleInfoCallback * Cb, const Caster * caster_)
	: RegularSpellMechanics(s, Cb, caster_)
{
}

void ChainLightningMechanics::applyBattleEffects(const SpellCastEnvironment * env, const BattleCast & parameters, SpellCastContext & ctx) const
{
	const int rawDamage = parameters.getEffectValue();
	int chainLightningModifier = 0;
	for(auto & attackedCre : ctx.attackedCres)
	{
		BattleStackAttacked bsa;
		bsa.damageAmount = owner->adjustRawDamage(caster, attackedCre, rawDamage) >> chainLightningModifier;
		ctx.addDamageToDisplay(bsa.damageAmount);

		bsa.stackAttacked = (attackedCre)->ID;
		bsa.attackerID = -1;
		attackedCre->prepareAttacked(bsa, env->getRandomGenerator());
		ctx.si.stacks.push_back(bsa);

		++chainLightningModifier;
	}
}

std::vector<const CStack *> ChainLightningMechanics::calculateAffectedStacks(int spellLvl, BattleHex destination) const
{
	std::vector<const CStack *> res;
	std::set<BattleHex> possibleHexes;
	for(auto stack : cb->battleGetAllStacks())
	{
		if(stack->isValidTarget())
			for(auto hex : stack->getHexes())
				possibleHexes.insert(hex);
	}
	static const std::array<int, 4> targetsOnLevel = {4, 4, 5, 5};

	BattleHex lightningHex = destination;
	for(int i = 0; i < targetsOnLevel.at(spellLvl); ++i)
	{
		auto stack = cb->battleGetStackByPos(lightningHex, true);
		if(!stack)
			break;
		res.push_back(stack);
		for(auto hex : stack->getHexes())
			possibleHexes.erase(hex); //can't hit same stack twice
		if(possibleHexes.empty()) //not enough targets
			break;
		lightningHex = BattleHex::getClosestTile(stack->side, lightningHex, possibleHexes);
	}

	return res;
}

///CloneMechanics
CloneMechanics::CloneMechanics(const CSpell * s, const CBattleInfoCallback * Cb, const Caster * caster_)
	: RegularSpellMechanics(s, Cb, caster_)
{
}

void CloneMechanics::applyBattleEffects(const SpellCastEnvironment * env, const BattleCast & parameters, SpellCastContext & ctx) const
{
	const CStack * clonedStack = vstd::frontOrNull(ctx.attackedCres);
	if(!clonedStack)
	{
		env->complain("No target stack to clone!");
		return;
	}

	BattleStackAdded bsa;
	bsa.creID = clonedStack->type->idNumber;
	bsa.side = casterSide;
	bsa.summoned = true;
	bsa.pos = cb->getAvaliableHex(bsa.creID, casterSide);
	bsa.amount = clonedStack->getCount();
	env->sendAndApply(&bsa);

	BattleSetStackProperty ssp;
	ssp.stackID = bsa.newStackID;//we know stack ID after apply
	ssp.which = BattleSetStackProperty::CLONED;
	ssp.val = 0;
	ssp.absolute = 1;
	env->sendAndApply(&ssp);

	ssp.stackID = clonedStack->ID;
	ssp.which = BattleSetStackProperty::HAS_CLONE;
	ssp.val = bsa.newStackID;
	ssp.absolute = 1;
	env->sendAndApply(&ssp);

	SetStackEffect sse;
	sse.stacks.push_back(bsa.newStackID);
	Bonus lifeTimeMarker(Bonus::N_TURNS, Bonus::NONE, Bonus::SPELL_EFFECT, 0, owner->id.num);
	lifeTimeMarker.turnsRemain = parameters.effectDuration;
	sse.effect.push_back(lifeTimeMarker);
	env->sendAndApply(&sse);
}

bool CloneMechanics::isImmuneByStack(const CStack * obj) const
{
	//can't clone already cloned creature
	if(obj->isClone())
		return true;
	//can`t clone if old clone still alive
	if(obj->cloneID != -1)
		return true;
	ui8 schoolLevel;
	schoolLevel = caster->getEffectLevel(mode, owner);

	if(schoolLevel < 3)
	{
		int maxLevel = (std::max(schoolLevel, (ui8)1) + 4);
		int creLevel = obj->getCreature()->level;
		if(maxLevel < creLevel) //tier 1-5 for basic, 1-6 for advanced, any level for expert
			return true;
	}
	//use default algorithm only if there is no mechanics-related problem
	return RegularSpellMechanics::isImmuneByStack(obj);
}

///CureMechanics
CureMechanics::CureMechanics(const CSpell * s, const CBattleInfoCallback * Cb, const Caster * caster_)
	: HealingSpellMechanics(s, Cb, caster_)
{
}

void CureMechanics::applyBattleEffects(const SpellCastEnvironment * env, const BattleCast & parameters, SpellCastContext & ctx) const
{
	HealingSpellMechanics::applyBattleEffects(env, parameters, ctx);
	doDispell(env, ctx, dispellSelector);
}

EHealLevel CureMechanics::getHealLevel(int effectLevel) const
{
	return EHealLevel::HEAL;
}

EHealPower CureMechanics::getHealPower(int effectLevel) const
{
	return EHealPower::PERMANENT;
}

bool CureMechanics::dispellSelector(const Bonus * b)
{
	if(b->source == Bonus::SPELL_EFFECT)
	{
		const CSpell * sp = SpellID(b->sid).toSpell();
		return sp && sp->isNegative();
	}
	return false; //not a spell effect
}

bool CureMechanics::isImmuneByStack(const CStack * obj) const
{
	//Selector method name is ok as cashing string. --AVS
	if(!obj->canBeHealed() && !canDispell(obj, dispellSelector, "CureMechanics::dispellSelector"))
		return true;

	return RegularSpellMechanics::isImmuneByStack(obj);
}

///DispellMechanics
DispellMechanics::DispellMechanics(const CSpell * s, const CBattleInfoCallback * Cb, const Caster * caster_)
	: RegularSpellMechanics(s, Cb, caster_)
{
}

bool DispellMechanics::isImmuneByStack(const CStack * obj) const
{
	//just in case
	if(!obj->alive())
		return true;

	//DISPELL ignores all immunities, except specific absolute immunity
	{
		//SPELL_IMMUNITY absolute case
		std::stringstream cachingStr;
		cachingStr << "type_" << Bonus::SPELL_IMMUNITY << "subtype_" << owner->id.toEnum() << "addInfo_1";
		if(obj->hasBonus(Selector::typeSubtypeInfo(Bonus::SPELL_IMMUNITY, owner->id.toEnum(), 1), cachingStr.str()))
			return true;
	}

	if(canDispell(obj, Selector::all, "DefaultSpellMechanics::dispellSelector"))
		return false;
	else
		return true;
	//any other immunities are ignored - do not execute default algorithm
}

void DispellMechanics::applyBattleEffects(const SpellCastEnvironment * env, const BattleCast & parameters, SpellCastContext & ctx) const
{
	doDispell(env, ctx, Selector::all);

	if(parameters.spellLvl > 2)
	{
		//expert DISPELL also removes spell-created obstacles
		ObstaclesRemoved packet;

		for(auto obstacle : cb->battleGetAllObstacles(BattlePerspective::ALL_KNOWING))
		{
			if(obstacle->obstacleType == CObstacleInstance::FIRE_WALL
				|| obstacle->obstacleType == CObstacleInstance::FORCE_FIELD
				|| obstacle->obstacleType == CObstacleInstance::LAND_MINE)
				packet.obstacles.insert(obstacle->uniqueID);
		}

		if(!packet.obstacles.empty())
			env->sendAndApply(&packet);
	}
}

///EarthquakeMechanics
EarthquakeMechanics::EarthquakeMechanics(const CSpell * s, const CBattleInfoCallback * Cb, const Caster * caster_)
	: SpecialSpellMechanics(s, Cb, caster_)
{
}

void EarthquakeMechanics::applyBattleEffects(const SpellCastEnvironment * env, const BattleCast & parameters, SpellCastContext & ctx) const
{
	if(nullptr == cb->battleGetDefendedTown())
	{
		env->complain("EarthquakeMechanics: not town siege");
		return;
	}

	if(CGTownInstance::NONE == cb->battleGetDefendedTown()->fortLevel())
	{
		env->complain("EarthquakeMechanics: town has no fort");
		return;
	}

	//start with all destructible parts
	std::set<EWallPart::EWallPart> possibleTargets =
	{
		EWallPart::KEEP,
		EWallPart::BOTTOM_TOWER,
		EWallPart::BOTTOM_WALL,
		EWallPart::BELOW_GATE,
		EWallPart::OVER_GATE,
		EWallPart::UPPER_WALL,
		EWallPart::UPPER_TOWER,
		EWallPart::GATE
	};

	assert(possibleTargets.size() == EWallPart::PARTS_COUNT);

	const int targetsToAttack = 2 + std::max<int>(parameters.spellLvl - 1, 0);

	CatapultAttack ca;
	ca.attacker = -1;

	for(int i = 0; i < targetsToAttack; i++)
	{
		//Any destructible part can be hit regardless of its HP. Multiple hit on same target is allowed.
		EWallPart::EWallPart target = *RandomGeneratorUtil::nextItem(possibleTargets, env->getRandomGenerator());

		auto state = cb->battleGetWallState(target);

		if(state == EWallState::DESTROYED || state == EWallState::NONE)
			continue;

		CatapultAttack::AttackInfo attackInfo;

		attackInfo.damageDealt = 1;
		attackInfo.attackedPart = target;
		attackInfo.destinationTile = cb->wallPartToBattleHex(target);

		ca.attackedParts.push_back(attackInfo);

		//removing creatures in turrets / keep if one is destroyed
		BattleHex posRemove;

		switch(target)
		{
		case EWallPart::KEEP:
			posRemove = -2;
			break;
		case EWallPart::BOTTOM_TOWER:
			posRemove = -3;
			break;
		case EWallPart::UPPER_TOWER:
			posRemove = -4;
			break;
		}

		if(posRemove != BattleHex::INVALID)
		{
			BattleStacksRemoved bsr;
			auto all = cb->battleGetStacksIf([=](const CStack * s)
			{
				return !s->isGhost();
			});

			for(auto & elem : all)
			{
				if(elem->position == posRemove)
				{
					bsr.stackIDs.insert(elem->ID);
					break;
				}
			}
			if(bsr.stackIDs.size() > 0)
				env->sendAndApply(&bsr);
		}
	}

	env->sendAndApply(&ca);
}

bool EarthquakeMechanics::canBeCast(Problem & problem) const
{
	if(mode == Mode::AFTER_ATTACK || mode == Mode::BEFORE_ATTACK || mode == Mode::SPELL_LIKE_ATTACK || mode == Mode::MAGIC_MIRROR)
	{
		logGlobal->warn("Invalid spell cast attempt: spell %s, mode %d", owner->name, (int)mode); //should not even try to do it
		return adaptProblem(ESpellCastProblem::INVALID, problem);
	}

	if(nullptr == cb->battleGetDefendedTown())
	{
		return adaptProblem(ESpellCastProblem::NO_APPROPRIATE_TARGET, problem);
	}

	if(CGTownInstance::NONE == cb->battleGetDefendedTown()->fortLevel())
	{
		return adaptProblem(ESpellCastProblem::NO_APPROPRIATE_TARGET, problem);
	}

	CSpell::TargetInfo ti(owner, caster->getSpellSchoolLevel(mode, owner), mode);
	if(ti.smart)
	{
		const auto side = cb->playerToSide(caster->getOwner());
		if(!side)
			return adaptProblem(ESpellCastProblem::INVALID, problem);
		//if spell targeting is smart, then only attacker can use it
		if(side.get() != BattleSide::ATTACKER)
			return adaptProblem(ESpellCastProblem::NO_APPROPRIATE_TARGET, problem);
	}

	const auto attackableBattleHexes = cb->getAttackableBattleHexes();

	if(attackableBattleHexes.empty())
		return adaptProblem(ESpellCastProblem::NO_APPROPRIATE_TARGET, problem);

	return true;
}

bool EarthquakeMechanics::requiresCreatureTarget() const
{
	return false;
}

///HypnotizeMechanics
HypnotizeMechanics::HypnotizeMechanics(const CSpell * s, const CBattleInfoCallback * Cb, const Caster * caster_)
	: RegularSpellMechanics(s, Cb, caster_)
{
}

bool HypnotizeMechanics::isImmuneByStack(const CStack * obj) const
{
	//todo: maybe do not resist on passive cast
	//TODO: what with other creatures casting hypnotize, Faerie Dragons style?
	int64_t subjectHealth = obj->health.available();
	//apply 'damage' bonus for hypnotize, including hero specialty
	int64_t maxHealth = caster->getSpellBonus(owner, owner->calculateRawEffectValue(caster->getEffectLevel(mode, owner), caster->getEffectPower(mode, owner), 1), obj);
	if(subjectHealth > maxHealth)
		return true;
	return RegularSpellMechanics::isImmuneByStack(obj);
}

///ObstacleMechanics
ObstacleMechanics::ObstacleMechanics(const CSpell * s, const CBattleInfoCallback * Cb, const Caster * caster_)
	: SpecialSpellMechanics(s, Cb, caster_)
{
}

bool ObstacleMechanics::canBeCastAt(BattleHex destination) const
{
	const auto level = caster->getSpellSchoolLevel(mode, owner);
	bool hexesOutsideBattlefield = false;

	auto tilesThatMustBeClear = rangeInHexes(destination, level, &hexesOutsideBattlefield);
	const CSpell::TargetInfo ti(owner, level, mode);
	for(const BattleHex & hex : tilesThatMustBeClear)
		if(!isHexAviable(cb, hex, ti.clearAffected))
			return false;

	if(hexesOutsideBattlefield)
		return false;

	return true;
}

bool ObstacleMechanics::isHexAviable(const CBattleInfoCallback * cb, const BattleHex & hex, const bool mustBeClear)
{
	if(!hex.isAvailable())
		return false;

	if(!mustBeClear)
		return true;

	if(cb->battleGetStackByPos(hex, true))
		return false;

	auto obst = cb->battleGetAllObstaclesOnPos(hex, false);

	for(auto & i : obst)
		if(i->obstacleType != CObstacleInstance::MOAT)
			return false;

	if(cb->battleGetDefendedTown() != nullptr && cb->battleGetDefendedTown()->fortLevel() != CGTownInstance::NONE)
	{
		EWallPart::EWallPart part = cb->battleHexToWallPart(hex);

		if(part == EWallPart::INVALID || part == EWallPart::INDESTRUCTIBLE_PART_OF_GATE)
			return true;//no fortification here
		else if(static_cast<int>(part) < 0)
			return false;//indestuctible part (cant be checked by battleGetWallState)
		else if(part == EWallPart::BOTTOM_TOWER || part == EWallPart::UPPER_TOWER)
			return false;//destructible, but should not be available
		else if(cb->battleGetWallState(part) != EWallState::DESTROYED && cb->battleGetWallState(part) != EWallState::NONE)
			return false;
	}

	return true;
}

void ObstacleMechanics::placeObstacle(const SpellCastEnvironment * env, const BattleCast & parameters, const BattleHex & pos) const
{
	//do not trust obstacle count, some of them may be removed
	auto all = cb->battleGetAllObstacles(BattlePerspective::ALL_KNOWING);

	int obstacleIdToGive = 1;
	for(auto & one : all)
		if(one->uniqueID >= obstacleIdToGive)
			obstacleIdToGive = one->uniqueID + 1;

	auto obstacle = std::make_shared<SpellCreatedObstacle>();
	obstacle->pos = pos;
	obstacle->casterSide = casterSide;
	obstacle->ID = owner->id;
	obstacle->spellLevel = parameters.effectLevel;
	obstacle->casterSpellPower = parameters.effectPower;
	obstacle->uniqueID = obstacleIdToGive;
	obstacle->customSize = std::vector<BattleHex>(1, pos);
	setupObstacle(obstacle.get());

	BattleObstaclePlaced bop;
	bop.obstacle = obstacle;
	env->sendAndApply(&bop);
}

///PatchObstacleMechanics
PatchObstacleMechanics::PatchObstacleMechanics(const CSpell * s, const CBattleInfoCallback * Cb, const Caster * caster_)
	: ObstacleMechanics(s, Cb, caster_)
{
}

void PatchObstacleMechanics::applyBattleEffects(const SpellCastEnvironment * env, const BattleCast & parameters, SpellCastContext & ctx) const
{
	std::vector<BattleHex> availableTiles;
	for(int i = 0; i < GameConstants::BFIELD_SIZE; i += 1)
	{
		BattleHex hex = i;
		if(isHexAviable(cb, hex, true))
			availableTiles.push_back(hex);
	}
	RandomGeneratorUtil::randomShuffle(availableTiles, env->getRandomGenerator());
	const int patchesForSkill[] = {4, 4, 6, 8};
	const int patchesToPut = std::min<int>(patchesForSkill[parameters.spellLvl], availableTiles.size());

	//land mines or quicksand patches are handled as spell created obstacles
	for (int i = 0; i < patchesToPut; i++)
		placeObstacle(env, parameters, availableTiles.at(i));
}

///LandMineMechanics
LandMineMechanics::LandMineMechanics(const CSpell * s, const CBattleInfoCallback * Cb, const Caster * caster_)
	: PatchObstacleMechanics(s, Cb, caster_)
{
}

bool LandMineMechanics::canBeCast(Problem & problem) const
{
	//LandMine are useless if enemy has native stack and can see mines, check for LandMine damage immunity is done in general way by CSpell
	const auto side = cb->playerToSide(caster->getOwner());
	if(!side)
		return adaptProblem(ESpellCastProblem::INVALID, problem);

	const ui8 otherSide = cb->otherSide(side.get());

	if(cb->battleHasNativeStack(otherSide))
		return adaptProblem(ESpellCastProblem::NO_APPROPRIATE_TARGET, problem);

	return SpecialSpellMechanics::canBeCast(problem);
}

int LandMineMechanics::defaultDamageEffect(const SpellCastEnvironment * env, const BattleCast & parameters, StacksInjured & si, const TStacks & target) const
{
	auto res = PatchObstacleMechanics::defaultDamageEffect(env, parameters, si, target);

	for(BattleStackAttacked & bsa : si.stacks)
	{
		bsa.effect = 82;
		bsa.flags |= BattleStackAttacked::EFFECT;
	}

	return res;
}

bool LandMineMechanics::requiresCreatureTarget() const
{
	return true;
}

void LandMineMechanics::setupObstacle(SpellCreatedObstacle * obstacle) const
{
	obstacle->obstacleType = CObstacleInstance::LAND_MINE;
	obstacle->turnsRemaining = -1;
	obstacle->visibleForAnotherSide = false;
}

///QuicksandMechanics
QuicksandMechanics::QuicksandMechanics(const CSpell * s, const CBattleInfoCallback * Cb, const Caster * caster_)
	: PatchObstacleMechanics(s, Cb, caster_)
{
}

bool QuicksandMechanics::requiresCreatureTarget() const
{
	return false;
}

void QuicksandMechanics::setupObstacle(SpellCreatedObstacle * obstacle) const
{
	obstacle->obstacleType = CObstacleInstance::QUICKSAND;
	obstacle->turnsRemaining = -1;
	obstacle->visibleForAnotherSide = false;
}

///WallMechanics
WallMechanics::WallMechanics(const CSpell * s, const CBattleInfoCallback * Cb, const Caster * caster_)
	: ObstacleMechanics(s, Cb, caster_)
{
}

std::vector<BattleHex> WallMechanics::rangeInHexes(BattleHex centralHex, ui8 schoolLvl, bool * outDroppedHexes) const
{
	std::vector<BattleHex> ret;

	//Special case - shape of obstacle depends on caster's side
	//TODO make it possible through spell config

	BattleHex::EDir firstStep, secondStep;
	if(casterSide)
	{
		firstStep = BattleHex::TOP_LEFT;
		secondStep = BattleHex::TOP_RIGHT;
	}
	else
	{
		firstStep = BattleHex::TOP_RIGHT;
		secondStep = BattleHex::TOP_LEFT;
	}

	//Adds hex to the ret if it's valid. Otherwise sets output arg flag if given.
	auto addIfValid = [&](BattleHex hex)
	{
		if(hex.isValid())
			ret.push_back(hex);
		else if(outDroppedHexes)
			*outDroppedHexes = true;
	};

	ret.push_back(centralHex);
	addIfValid(centralHex.moveInDirection(firstStep, false));
	if(schoolLvl >= 2) //advanced versions of fire wall / force field cotnains of 3 hexes
		addIfValid(centralHex.moveInDirection(secondStep, false)); //moveInDir function modifies subject hex

	return ret;
}

///FireWallMechanics
FireWallMechanics::FireWallMechanics(const CSpell * s, const CBattleInfoCallback * Cb, const Caster * caster_)
	: WallMechanics(s, Cb, caster_)
{
}

bool FireWallMechanics::requiresCreatureTarget() const
{
	return true;
}

void FireWallMechanics::applyBattleEffects(const SpellCastEnvironment * env, const BattleCast & parameters, SpellCastContext & ctx) const
{
	const BattleHex destination = parameters.getFirstDestinationHex();

	if(!destination.isValid())
	{
		env->complain("Invalid destination for FIRE_WALL");
		return;
	}
	//firewall is build from multiple obstacles - one fire piece for each affected hex
	auto affectedHexes = rangeInHexes(destination, parameters.spellLvl);
	for(BattleHex hex : affectedHexes)
		placeObstacle(env, parameters, hex);
}

void FireWallMechanics::setupObstacle(SpellCreatedObstacle * obstacle) const
{
	obstacle->obstacleType = CObstacleInstance::FIRE_WALL;
	obstacle->turnsRemaining = 2;
	obstacle->visibleForAnotherSide = true;
}

///ForceFieldMechanics
ForceFieldMechanics::ForceFieldMechanics(const CSpell * s, const CBattleInfoCallback * Cb, const Caster * caster_)
	: WallMechanics(s, Cb, caster_)
{
}

bool ForceFieldMechanics::requiresCreatureTarget() const
{
	return false;
}

void ForceFieldMechanics::applyBattleEffects(const SpellCastEnvironment * env, const BattleCast & parameters, SpellCastContext & ctx) const
{
	const BattleHex destination = parameters.getFirstDestinationHex();

	if(!destination.isValid())
	{
		env->complain("Invalid destination for FORCE_FIELD");
		return;
	}
	placeObstacle(env, parameters, destination);
}

void ForceFieldMechanics::setupObstacle(SpellCreatedObstacle * obstacle) const
{
	obstacle->obstacleType = CObstacleInstance::FORCE_FIELD;
	obstacle->turnsRemaining = 2;
	obstacle->visibleForAnotherSide = true;
	obstacle->customSize = rangeInHexes(obstacle->pos, obstacle->spellLevel);
}

///RemoveObstacleMechanics
RemoveObstacleMechanics::RemoveObstacleMechanics(const CSpell * s, const CBattleInfoCallback * Cb, const Caster * caster_)
	: SpecialSpellMechanics(s, Cb, caster_)
{
}

void RemoveObstacleMechanics::applyBattleEffects(const SpellCastEnvironment * env, const BattleCast & parameters, SpellCastContext & ctx) const
{
	auto obstacleToRemove = cb->battleGetAllObstaclesOnPos(parameters.getFirstDestinationHex(), false);
	if(!obstacleToRemove.empty())
	{
		ObstaclesRemoved obr;
		bool complain = true;
		for(auto & i : obstacleToRemove)
		{
			if(canRemove(i.get(), parameters.spellLvl))
			{
				obr.obstacles.insert(i->uniqueID);
				complain = false;
			}
		}
		if(!complain)
			env->sendAndApply(&obr);
		else if(complain || obr.obstacles.empty())
			env->complain("Cant remove this obstacle!");
	}
	else
		env->complain("There's no obstacle to remove!");
}

bool RemoveObstacleMechanics::canBeCast(Problem & problem) const
{
	if(mode == Mode::AFTER_ATTACK || mode == Mode::BEFORE_ATTACK || mode == Mode::SPELL_LIKE_ATTACK || mode == Mode::MAGIC_MIRROR)
	{
		logGlobal->warn("Invalid spell cast attempt: spell %s, mode %d", owner->name, (int)mode); //should not even try to do it
		return adaptProblem(ESpellCastProblem::INVALID, problem);
	}

	const int spellLevel = caster->getSpellSchoolLevel(mode, owner);

	for(auto obstacle : cb->battleGetAllObstacles())
		if(canRemove(obstacle.get(), spellLevel))
			return true;

	return adaptProblem(ESpellCastProblem::NO_APPROPRIATE_TARGET, problem);
}

bool RemoveObstacleMechanics::canBeCastAt(BattleHex destination) const
{
	const auto level = caster->getSpellSchoolLevel(mode, owner);
	auto obstacles = cb->battleGetAllObstaclesOnPos(destination, false);
	if(!obstacles.empty())
	{
		for(auto & i : obstacles)
			if(canRemove(i.get(), level))
				return true;
	}
	return false;
}

bool RemoveObstacleMechanics::canRemove(const CObstacleInstance * obstacle, const int spellLevel) const
{
	switch (obstacle->obstacleType)
	{
	case CObstacleInstance::ABSOLUTE_OBSTACLE: //cliff-like obstacles can't be removed
	case CObstacleInstance::MOAT:
		return false;
	case CObstacleInstance::USUAL:
		return true;
	case CObstacleInstance::FIRE_WALL:
		if(spellLevel >= 2)
			return true;
		break;
	case CObstacleInstance::QUICKSAND:
	case CObstacleInstance::LAND_MINE:
	case CObstacleInstance::FORCE_FIELD:
		if(spellLevel >= 3)
			return true;
		break;
	default:
		break;
	}
	return false;
}

bool RemoveObstacleMechanics::requiresCreatureTarget() const
{
	return false;
}

///RisingSpellMechanics
RisingSpellMechanics::RisingSpellMechanics(const CSpell * s, const CBattleInfoCallback * Cb, const Caster * caster_)
	: HealingSpellMechanics(s, Cb, caster_)
{
}

EHealLevel RisingSpellMechanics::getHealLevel(int effectLevel) const
{
	return EHealLevel::RESURRECT;
}

EHealPower RisingSpellMechanics::getHealPower(int effectLevel) const
{
	//this may be even distinct class
	if((effectLevel <= 1) && (owner->id == SpellID::RESURRECTION))
		return EHealPower::ONE_BATTLE;
	else
		return EHealPower::PERMANENT;
}

///SacrificeMechanics
SacrificeMechanics::SacrificeMechanics(const CSpell * s, const CBattleInfoCallback * Cb, const Caster * caster_)
	: RisingSpellMechanics(s, Cb, caster_)
{
}

bool SacrificeMechanics::canBeCast(Problem & problem) const
{
	if(mode == Mode::AFTER_ATTACK || mode == Mode::BEFORE_ATTACK || mode == Mode::SPELL_LIKE_ATTACK || mode == Mode::MAGIC_MIRROR)
	{
		logGlobal->warn("Invalid spell cast attempt: spell %s, mode %d", owner->name, (int)mode); //should not even try to do it
		return adaptProblem(ESpellCastProblem::INVALID, problem);
	}

	// for sacrifice we have to check for 2 targets (one dead to resurrect and one living to destroy)

	bool targetExists = false;
	bool targetToSacrificeExists = false;

	for(const CStack * stack : cb->battleGetAllStacks())
	{
		//using isImmuneBy directly as this mechanics does not have overridden immunity check
		//therefore we do not need to check caster and casting mode
		//TODO: check that we really should check immunity for both stacks
		const bool immune = owner->internalIsImmune(caster, stack);
		const bool casterStack = stack->owner == caster->getOwner();

		if(!immune && casterStack)
		{
			if(stack->alive())
				targetToSacrificeExists = true;
			else
				targetExists = true;
			if(targetExists && targetToSacrificeExists)
				break;
		}
	}

	if(targetExists && targetToSacrificeExists)
		return true;
	else
		return adaptProblem(ESpellCastProblem::NO_APPROPRIATE_TARGET, problem);
}

void SacrificeMechanics::applyBattleEffects(const SpellCastEnvironment * env, const BattleCast & parameters, SpellCastContext & ctx) const
{
	const CStack * victim = nullptr;
	if(parameters.target.size() == 2)
	{
		victim = parameters.target[1].stackValue;
	}

	if(nullptr == victim)
	{
		env->complain("SacrificeMechanics: No stack to sacrifice");
		return;
	}
	//resurrect target after basic checks
	RisingSpellMechanics::applyBattleEffects(env, parameters, ctx);
	//it is safe to remove even active stack
	BattleStacksRemoved bsr;
	bsr.stackIDs.insert(victim->ID);
	env->sendAndApply(&bsr);
}

int SacrificeMechanics::calculateHealedHP(const SpellCastEnvironment * env, const BattleCast& parameters, SpellCastContext& ctx) const
{
	const CStack * victim = nullptr;

	if(parameters.target.size() == 2)
	{
		victim = parameters.target[1].stackValue;
	}

	if(nullptr == victim)
	{
		env->complain("SacrificeMechanics: No stack to sacrifice");
		return 0;
	}

	return (parameters.effectPower + victim->MaxHealth() + owner->getPower(parameters.effectLevel)) * victim->getCount();
}

bool SacrificeMechanics::requiresCreatureTarget() const
{
	return false;//canBeCast do all target existence checks
}

///SpecialRisingSpellMechanics
SpecialRisingSpellMechanics::SpecialRisingSpellMechanics(const CSpell * s, const CBattleInfoCallback * Cb, const Caster * caster_)
	: RisingSpellMechanics(s, Cb, caster_)
{
}

bool SpecialRisingSpellMechanics::canBeCastAt(BattleHex destination) const
{
	const auto level = caster->getSpellSchoolLevel(mode, owner);
	const CSpell::TargetInfo ti(owner, level, mode);
	auto mainFilter = [ti, destination, this](const CStack * s) -> bool
	{
		const bool ownerMatches = !ti.smart || cb->battleMatchOwner(caster->getOwner(), s, owner->getPositiveness());
		return ownerMatches && s->coversPos(destination) && !isImmuneByStack(s);
	};

	auto getStackIf = [this](TStackFilter predicate)-> const CStack *
	{
		auto stacks = cb->battleGetStacksIf(predicate);

		if(!stacks.empty())
			return stacks.front();
		else
			return nullptr;
	};

	//find alive possible target
	const CStack * stackToHeal = getStackIf([mainFilter](const CStack * s)
	{
		return s->isValidTarget(false) && mainFilter(s);
	});

	if(nullptr == stackToHeal)
	{
		//find dead possible target if there is no alive target
		stackToHeal = getStackIf([mainFilter](const CStack * s)
		{
			return s->isValidTarget(true) && mainFilter(s);
		});

		//we have found dead target
		if(nullptr != stackToHeal)
		{
			for(const BattleHex & hex : stackToHeal->getHexes())
			{
				const CStack * other = getStackIf([hex, stackToHeal](const CStack * s)
				{
					return s->isValidTarget(true) && s->coversPos(hex) && s != stackToHeal;
				});
				if(nullptr != other)
					return false;//alive stack blocks resurrection
			}
		}
	}

	if(nullptr == stackToHeal)
		return false;

	return true;
}

bool SpecialRisingSpellMechanics::isImmuneByStack(const CStack * obj) const
{
	// following does apply to resurrect and animate dead(?) only
	// for sacrifice health calculation and health limit check don't matter

	if(obj->getCount() >= obj->baseAmount)
		return true;

	//FIXME: code duplication with BattleCast
	auto getEffectValue = [&]() -> si32
	{
		si32 effectValue = caster->getEffectValue(mode, owner);
		return (effectValue == 0) ? owner->calculateRawEffectValue(caster->getEffectLevel(mode, owner), caster->getEffectPower(mode, owner), 1) : effectValue;
	};

	auto maxHealth = getEffectValue();
	if (maxHealth < obj->MaxHealth()) //must be able to rise at least one full creature
		return true;

	return RegularSpellMechanics::isImmuneByStack(obj);
}

///TeleportMechanics
TeleportMechanics::TeleportMechanics(const CSpell * s, const CBattleInfoCallback * Cb, const Caster * caster_)
	: RegularSpellMechanics(s, Cb, caster_)
{
}

void TeleportMechanics::applyBattleEffects(const SpellCastEnvironment * env, const BattleCast & parameters, SpellCastContext & ctx) const
{
	if(parameters.target.size() == 2)
	{
		//first destination hex to move to
		const BattleHex destination = parameters.target[0].hexValue;
		if(!destination.isValid())
		{
			env->complain("TeleportMechanics: invalid teleport destination");
			return;
		}

		//second destination creature to move
		const CStack * target = parameters.target[1].stackValue;
		if(nullptr == target)
		{
			env->complain("TeleportMechanics: no stack to teleport");
			return;
		}

		if(!cb->battleCanTeleportTo(target, destination, parameters.effectLevel))
		{
			env->complain("TeleportMechanics: forbidden teleport");
			return;
		}

		BattleStackMoved bsm;
		bsm.distance = -1;
		bsm.stack = target->ID;
		std::vector<BattleHex> tiles;
		tiles.push_back(destination);
		bsm.tilesToMove = tiles;
		bsm.teleporting = true;
		env->sendAndApply(&bsm);
	}
	else
	{
		env->complain("TeleportMechanics: 2 destinations required.");
			return;
	}
}

bool TeleportMechanics::canBeCast(Problem & problem) const
{
	if(mode == Mode::AFTER_ATTACK || mode == Mode::BEFORE_ATTACK || mode == Mode::SPELL_LIKE_ATTACK || mode == Mode::MAGIC_MIRROR)
	{
		logGlobal->warn("Invalid spell cast attempt: spell %s, mode %d", owner->name, (int)mode); //should not even try to do it
		return adaptProblem(ESpellCastProblem::INVALID, problem);
	}

	return RegularSpellMechanics::canBeCast(problem);
}

} // namespace spells

