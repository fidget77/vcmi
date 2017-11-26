/*
 * ISpellMechanics.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "ISpellMechanics.h"

#include "../CStack.h"
#include "../HeroBonus.h"
#include "../battle/CBattleInfoCallback.h"
#include "../battle/IBattleState.h"

#include "../NetPacks.h"

#include "../serializer/JsonDeserializer.h"
#include "../serializer/JsonSerializer.h"

#include "TargetCondition.h"
#include "Problem.h"

#include "AdventureSpellMechanics.h"
#include "BattleSpellMechanics.h"
#include "CustomSpellMechanics.h"

#include "effects/Effects.h"
#include "effects/Damage.h"
#include "effects/Timed.h"

#include "CSpellHandler.h"

#include "../CHeroHandler.h"//todo: remove

namespace spells
{

static std::shared_ptr<TargetCondition> makeCondition(const CSpell * s)
{
	std::shared_ptr<TargetCondition> res = std::make_shared<TargetCondition>();

	JsonDeserializer deser(nullptr, s->targetCondition);
	res->serializeJson(deser);

	return res;
}

template<typename T>
class SpellMechanicsFactory : public ISpellMechanicsFactory
{
public:
	SpellMechanicsFactory(const CSpell * s)
		: ISpellMechanicsFactory(s)
	{
		targetCondition = makeCondition(s);
	}

	std::unique_ptr<Mechanics> create(const IBattleCast * event) const override
	{
		T * ret = new T(event);
		ret->targetCondition = targetCondition;
		return std::unique_ptr<Mechanics>(ret);
	}
private:
	std::shared_ptr<TargetCondition> targetCondition;
};

class CustomMechanicsFactory : public ISpellMechanicsFactory
{
public:
	std::unique_ptr<Mechanics> create(const IBattleCast * event) const override
	{
		CustomSpellMechanics * ret = new CustomSpellMechanics(event, effects);
		ret->targetCondition = targetCondition;
		return std::unique_ptr<Mechanics>(ret);
	}
protected:
	std::shared_ptr<effects::Effects> effects;

	CustomMechanicsFactory(const CSpell * s)
		: ISpellMechanicsFactory(s), effects(new effects::Effects)
	{
		targetCondition = makeCondition(s);
	}

	void loadEffects(const JsonNode & config, const int level)
	{
		JsonDeserializer deser(nullptr, config);
		effects->serializeJson(deser, level);
	}
private:
	std::shared_ptr<TargetCondition> targetCondition;
};

class ConfigurableMechanicsFactory : public CustomMechanicsFactory
{
public:
	ConfigurableMechanicsFactory(const CSpell * s)
		: CustomMechanicsFactory(s)
	{
		for(int level = 0; level < GameConstants::SPELL_SCHOOL_LEVELS; level++)
			loadEffects(s->getLevelInfo(level).battleEffects, level);
	}
};


//to be used for spells configured with old format
class FallbackMechanicsFactory : public CustomMechanicsFactory
{
public:
	FallbackMechanicsFactory(const CSpell * s)
		: CustomMechanicsFactory(s)
	{
		for(int level = 0; level < GameConstants::SPELL_SCHOOL_LEVELS; level++)
		{
			const CSpell::LevelInfo & levelInfo = s->getLevelInfo(level);
			assert(levelInfo.battleEffects.isNull());

			if(s->isOffensiveSpell())
			{
				//default constructed object should be enough
				effects->add("directDamage", std::make_shared<effects::Damage>(level), level);
			}

			std::shared_ptr<effects::Effect> effect;

			if(!levelInfo.effects.empty())
			{
				auto timed = new effects::Timed(level);
				timed->cumulative = false;
				timed->bonus = levelInfo.effects;
				effect.reset(timed);
			}

			if(!levelInfo.cumulativeEffects.empty())
			{
				auto timed = new effects::Timed(level);
				timed->cumulative = true;
				timed->bonus = levelInfo.cumulativeEffects;
				effect.reset(timed);
			}

			if(effect)
				effects->add("timed", effect, level);
		}
	}
};


BattleCast::BattleCast(const CBattleInfoCallback * cb, const Caster * caster_, const Mode mode_, const CSpell * spell_)
	: spell(spell_),
	cb(cb),
	caster(caster_),
	mode(mode_),
	spellLvl(),
	effectLevel(),
	effectPower(),
	effectDuration(),
	effectValue()
{

}

BattleCast::BattleCast(const BattleCast & orig, const Caster * caster_)
	: spell(orig.spell),
	cb(orig.cb),
	caster(caster_),
	mode(Mode::MAGIC_MIRROR),
	spellLvl(orig.spellLvl),
	effectLevel(orig.effectLevel),
	effectPower(orig.effectPower),
	effectDuration(orig.effectDuration),
	effectValue(orig.effectValue)
{
}

BattleCast::~BattleCast() = default;

const CSpell * BattleCast::getSpell() const
{
	return spell;
}

Mode BattleCast::getMode() const
{
	return mode;
}

const Caster * BattleCast::getCaster() const
{
	return caster;
}

const CBattleInfoCallback * BattleCast::getBattle() const
{
	return cb;
}

BattleCast::OptionalValue BattleCast::getEffectLevel() const
{
	if(effectLevel)
		return effectLevel;
	else
		return spellLvl;
}

BattleCast::OptionalValue BattleCast::getRangeLevel() const
{
	if(rangeLevel)
		return rangeLevel;
	else
		return spellLvl;
}

BattleCast::OptionalValue BattleCast::getEffectPower() const
{
	return effectPower;
}

BattleCast::OptionalValue BattleCast::getEffectDuration() const
{
	return effectDuration;
}

BattleCast::OptionalValue64 BattleCast::getEffectValue() const
{
	return effectValue;
}

void BattleCast::setSpellLevel(BattleCast::Value value)
{
	spellLvl = boost::make_optional(value);
}

void BattleCast::setEffectLevel(BattleCast::Value value)
{
	effectLevel = boost::make_optional(value);
}

void BattleCast::setRangeLevel(BattleCast::Value value)
{
	rangeLevel = boost::make_optional(value);
}

void BattleCast::setEffectPower(BattleCast::Value value)
{
	effectPower = boost::make_optional(value);
}

void BattleCast::setEffectDuration(BattleCast::Value value)
{
	effectDuration = boost::make_optional(value);
}

void BattleCast::setEffectValue(BattleCast::Value64 value)
{
	effectValue = boost::make_optional(value);
}

void BattleCast::aimToHex(const BattleHex & destination)
{
	target.push_back(Destination(destination));
}

void BattleCast::aimToUnit(const battle::Unit * destination)
{
	if(nullptr == destination)
		logGlobal->error("BattleCast::aimToUnit: invalid unit.");
	else
		target.push_back(Destination(destination));
}

void BattleCast::applyEffects(const SpellCastEnvironment * env) const
{
	auto m = spell->battleMechanics(this);
	m->applyEffects(env, target);
}

void BattleCast::applyEffectsForced(const SpellCastEnvironment * env) const
{
	auto m = spell->battleMechanics(this);
	m->applyEffectsForced(env, target);
}

void BattleCast::cast(const SpellCastEnvironment * env)
{
	if(target.empty())
		aimToHex(BattleHex::INVALID);
	auto m = spell->battleMechanics(this);

	std::vector <const CStack*> reflected;//for magic mirror

	m->cast(env, target, reflected);

	//Magic Mirror effect
	for(auto & attackedCre : reflected)
	{
		if(mode == Mode::MAGIC_MIRROR)
		{
			logGlobal->error("Magic mirror recurrence!");
			return;
		}

		TStacks mirrorTargets = cb->battleGetStacksIf([this](const CStack * battleStack)
		{
			//Get all caster stacks. Magic mirror can reflect to immune creature (with no effect)
			return battleStack->owner == caster->getOwner() && battleStack->isValidTarget(false);
		});

		if(!mirrorTargets.empty())
		{
			int targetHex = (*RandomGeneratorUtil::nextItem(mirrorTargets, env->getRandomGenerator()))->getPosition();

			BattleCast mirror(*this, attackedCre);
			mirror.aimToHex(targetHex);
			mirror.cast(env);
		}
	}
}

void BattleCast::cast(IBattleState * battleState, vstd::RNG & rng)
{
	//TODO: make equivalent to normal cast
	if(target.empty())
		aimToHex(BattleHex::INVALID);
	auto m = spell->battleMechanics(this);

	//TODO: reflection
	//TODO: random effects evaluation

	m->cast(battleState, rng, target);
}

bool BattleCast::castIfPossible(const SpellCastEnvironment * env)
{
	if(spell->canBeCast(cb, mode, caster))
	{
		cast(env);
		return true;
	}
	return false;
}

std::vector<Target> BattleCast::findPotentialTargets() const
{
	//TODO: for more that 2 destinations per target much more efficient algorithm is required

	auto m = spell->battleMechanics(this);

	auto targetTypes = m->getTargetTypes();


	if(targetTypes.empty() || targetTypes.size() > 2)
	{
		return std::vector<Target>();
	}
	else
	{
		std::vector<Target> previous;
		std::vector<Target> next;

		for(size_t index = 0; index < targetTypes.size(); index++)
		{
			std::swap(previous, next);
			next.clear();

			std::vector<Destination> destinations;

			if(previous.empty())
			{
				Target empty;
				destinations = m->getPossibleDestinations(index, targetTypes.at(index), empty);

				for(auto & destination : destinations)
				{
					Target target;
					target.emplace_back(destination);
					next.push_back(target);
				}
			}
			else
			{
				for(const Target & current : previous)
				{
					destinations = m->getPossibleDestinations(index, targetTypes.at(index), current);

					for(auto & destination : destinations)
					{
						Target target = current;
						target.emplace_back(destination);
						next.push_back(target);
					}
				}
			}

			if(next.empty())
				break;
		}
		return next;
	}
}

///ISpellMechanicsFactory
ISpellMechanicsFactory::ISpellMechanicsFactory(const CSpell * s)
	: spell(s)
{

}

ISpellMechanicsFactory::~ISpellMechanicsFactory()
{

}

std::unique_ptr<ISpellMechanicsFactory> ISpellMechanicsFactory::get(const CSpell * s)
{
	//ignore spell id if there are special effects
	if(s->hasBattleEffects())
		return make_unique<ConfigurableMechanicsFactory>(s);

	//to be converted
	switch(s->id)
	{

	case SpellID::FIRE_WALL:
		return make_unique<SpellMechanicsFactory<FireWallMechanics>>(s);
	case SpellID::FORCE_FIELD:
		return make_unique<SpellMechanicsFactory<ForceFieldMechanics>>(s);
	case SpellID::LAND_MINE:
		return make_unique<SpellMechanicsFactory<LandMineMechanics>>(s);
	case SpellID::QUICKSAND:
		return make_unique<SpellMechanicsFactory<QuicksandMechanics>>(s);
	default:
		return make_unique<FallbackMechanicsFactory>(s);
	}
}

///Mechanics
Mechanics::Mechanics(const IBattleCast * event)
	: cb(event->getBattle()),
	mode(event->getMode()),
	caster(event->getCaster())
{
	casterUnit = dynamic_cast<const CStack *>(caster);

	//FIXME: ensure caster and check for valid player and side
	casterSide = 0;
	if(caster)
		casterSide = cb->playerToSide(caster->getOwner()).get();
}

Mechanics::~Mechanics() = default;

BaseMechanics::BaseMechanics(const IBattleCast * event)
	: Mechanics(event),
	owner(event->getSpell())
{
	{
		auto value = event->getRangeLevel();
		if(value)
			rangeLevel = value.get();
		else
			rangeLevel = caster->getSpellSchoolLevel(mode, owner);
		vstd::abetween(rangeLevel, 0, 3);
	}
	{
		auto value = event->getEffectLevel();
        if(value)
			effectLevel = value.get();
		else
			effectLevel = caster->getEffectLevel(mode, owner);
		vstd::abetween(effectLevel, 0, 3);
	}
	{
		auto value = event->getEffectPower();
		if(value)
			effectPower = value.get();
		else
			effectPower = caster->getEffectPower(mode, owner);
		vstd::amax(effectPower, 0);
	}
	{
		auto value = event->getEffectDuration();
		if(value)
			effectDuration = value.get();
		else
			effectDuration = caster->getEnchantPower(mode, owner);
		vstd::amax(effectDuration, 0); //???
	}
	{
		auto value = event->getEffectValue();
		if(value)
		{
			effectValue = value.get();
		}
		else
		{
			auto casterValue = caster->getEffectValue(mode, owner);
			if(casterValue == 0)
				effectValue = owner->calculateRawEffectValue(effectLevel, effectPower, 1);
			else
				effectValue = casterValue;
		}
		vstd::amax(effectValue, 0);
	}
}

BaseMechanics::~BaseMechanics() = default;

bool BaseMechanics::adaptGenericProblem(Problem & target) const
{
	MetaString text;
	// %s recites the incantations but they seem to have no effect.
	text.addTxt(MetaString::GENERAL_TXT, 541);
	caster->getCasterName(text);

	target.add(std::move(text), spells::Problem::NORMAL);
	return false;
}

bool BaseMechanics::adaptProblem(ESpellCastProblem::ESpellCastProblem source, Problem & target) const
{
	if(source == ESpellCastProblem::OK)
		return true;

	switch(source)
	{
	case ESpellCastProblem::SPELL_LEVEL_LIMIT_EXCEEDED:
		{
			MetaString text;
			//TODO: refactor
			auto hero = dynamic_cast<const CGHeroInstance *>(caster);
			if(!hero)
				return adaptGenericProblem(target);

			//Recanter's Cloak or similar effect. Try to retrieve bonus
			const auto b = hero->getBonusLocalFirst(Selector::type(Bonus::BLOCK_MAGIC_ABOVE));
			//TODO what about other values and non-artifact sources?
			if(b && b->val == 2 && b->source == Bonus::ARTIFACT)
			{
				//The %s prevents %s from casting 3rd level or higher spells.
				text.addTxt(MetaString::GENERAL_TXT, 536);
				text.addReplacement(MetaString::ART_NAMES, b->sid);
				caster->getCasterName(text);
				target.add(std::move(text), spells::Problem::NORMAL);
			}
			else if(b && b->source == Bonus::TERRAIN_OVERLAY && b->sid == BFieldType::CURSED_GROUND)
			{
				text.addTxt(MetaString::GENERAL_TXT, 537);
				target.add(std::move(text), spells::Problem::NORMAL);
			}
			else
			{
				return adaptGenericProblem(target);
			}
		}
		break;
	case ESpellCastProblem::WRONG_SPELL_TARGET:
	case ESpellCastProblem::STACK_IMMUNE_TO_SPELL:
	case ESpellCastProblem::NO_APPROPRIATE_TARGET:
		{
			MetaString text;
			text.addTxt(MetaString::GENERAL_TXT, 185);
			target.add(std::move(text), spells::Problem::NORMAL);
		}
		break;
	case ESpellCastProblem::INVALID:
		{
			MetaString text;
			text.addReplacement("Internal error during check of spell cast.");
			target.add(std::move(text), spells::Problem::CRITICAL);
		}
		break;
	default:
		return adaptGenericProblem(target);
	}

	return false;
}

bool BaseMechanics::isReceptive(const battle::Unit * target) const
{
	return targetCondition->isReceptive(cb, caster, this, target);
}

int32_t BaseMechanics::getSpellIndex() const
{
	return getSpellId().toEnum();
}

SpellID BaseMechanics::getSpellId() const
{
	return owner->id;
}

std::string BaseMechanics::getSpellName() const
{
	return owner->name;
}

int32_t BaseMechanics::getSpellLevel() const
{
	return owner->level;
}

bool BaseMechanics::isSmart() const
{
	const CSpell::TargetInfo targetInfo(owner, getRangeLevel(), mode);
	return targetInfo.smart;
}

bool BaseMechanics::isMassive() const
{
	const CSpell::TargetInfo targetInfo(owner, getRangeLevel(), mode);
	return targetInfo.massive;
}

bool BaseMechanics::isNegativeSpell() const
{
	return owner->isNegative();
}

bool BaseMechanics::isPositiveSpell() const
{
	return owner->isPositive();
}

int64_t BaseMechanics::adjustEffectValue(const battle::Unit * target) const
{
	return owner->adjustRawDamage(caster, target, getEffectValue());
}

int64_t BaseMechanics::applySpellBonus(int64_t value, const battle::Unit* target) const
{
	return caster->getSpellBonus(owner, value, target);
}

int64_t BaseMechanics::applySpecificSpellBonus(int64_t value) const
{
	return caster->getSpecificSpellBonus(owner, value);
}

int64_t BaseMechanics::calculateRawEffectValue(int32_t basePowerMultiplier, int32_t levelPowerMultiplier) const
{
	return owner->calculateRawEffectValue(getEffectLevel(), basePowerMultiplier, levelPowerMultiplier);
}

std::vector<Bonus::BonusType> BaseMechanics::getElementalImmunity() const
{
	std::vector<Bonus::BonusType> ret;

	owner->forEachSchool([&](const SchoolInfo & cnf, bool & stop)
	{
		ret.push_back(cnf.immunityBonus);
	});

	return ret;
}

bool BaseMechanics::ownerMatches(const battle::Unit * unit) const
{
    return cb->battleMatchOwner(caster->getOwner(), unit, owner->getPositiveness());
}

IBattleCast::Value BaseMechanics::getEffectLevel() const
{
	return effectLevel;
}

IBattleCast::Value BaseMechanics::getRangeLevel() const
{
	return rangeLevel;
}

IBattleCast::Value BaseMechanics::getEffectPower() const
{
	return effectPower;
}

IBattleCast::Value BaseMechanics::getEffectDuration() const
{
	return effectDuration;
}

IBattleCast::Value64 BaseMechanics::getEffectValue() const
{
	return effectValue;
}

std::vector<AimType> BaseMechanics::getTargetTypes() const
{
	std::vector<AimType> ret;
	detail::ProblemImpl ingored;

	if(canBeCast(ingored))
	{
		auto spellTargetType = owner->getTargetType();

		if(isMassive())
			spellTargetType = AimType::NO_TARGET;
		else if(spellTargetType == AimType::OBSTACLE)
			spellTargetType = AimType::LOCATION;

		ret.push_back(spellTargetType);
	}

	return ret;
}


} //namespace spells

///IAdventureSpellMechanics
IAdventureSpellMechanics::IAdventureSpellMechanics(const CSpell * s)
	: owner(s)
{
}

std::unique_ptr<IAdventureSpellMechanics> IAdventureSpellMechanics::createMechanics(const CSpell * s)
{
	switch (s->id)
	{
	case SpellID::SUMMON_BOAT:
		return make_unique<SummonBoatMechanics>(s);
	case SpellID::SCUTTLE_BOAT:
		return make_unique<ScuttleBoatMechanics>(s);
	case SpellID::DIMENSION_DOOR:
		return make_unique<DimensionDoorMechanics>(s);
	case SpellID::FLY:
	case SpellID::WATER_WALK:
	case SpellID::VISIONS:
	case SpellID::DISGUISE:
		return make_unique<AdventureSpellMechanics>(s); //implemented using bonus system
	case SpellID::TOWN_PORTAL:
		return make_unique<TownPortalMechanics>(s);
	case SpellID::VIEW_EARTH:
		return make_unique<ViewEarthMechanics>(s);
	case SpellID::VIEW_AIR:
		return make_unique<ViewAirMechanics>(s);
	default:
		return std::unique_ptr<IAdventureSpellMechanics>();
	}
}
