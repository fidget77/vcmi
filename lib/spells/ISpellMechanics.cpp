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
#include "../battle/CBattleInfoCallback.h"

#include "../NetPacks.h"

#include "../serializer/JsonDeserializer.h"
#include "../serializer/JsonSerializer.h"

#include "CDefaultSpellMechanics.h"

#include "AdventureSpellMechanics.h"
#include "BattleSpellMechanics.h"
#include "CreatureSpellMechanics.h"
#include "CustomSpellMechanics.h"

#include "effects/Effects.h"
#include "effects/Damage.h"
#include "effects/Timed.h"


#include "../CHeroHandler.h"//todo: remove

namespace spells
{

template<typename T>
class SpellMechanicsFactory : public ISpellMechanicsFactory
{
public:
	SpellMechanicsFactory(const CSpell * s)
		: ISpellMechanicsFactory(s)
	{}

	std::unique_ptr<Mechanics> create(const CBattleInfoCallback * cb, Mode mode, const Caster * caster) const override
	{
		std::unique_ptr<Mechanics> ret(new T(spell, cb, caster));
		ret->mode = mode;
		return ret;
	}
};


class CustomMechanicsFactory : public ISpellMechanicsFactory
{
public:
	std::unique_ptr<Mechanics> create(const CBattleInfoCallback * cb, Mode mode, const Caster * caster) const override
	{
		std::unique_ptr<Mechanics> ret(new CustomSpellMechanics(spell, cb, caster, effects));
		ret->mode = mode;
		return ret;
	}
protected:
	std::shared_ptr<spells::effects::Effects> effects;

	CustomMechanicsFactory(const CSpell * s)
		: ISpellMechanicsFactory(s), effects(new spells::effects::Effects)
	{
	}

	void loadEffects(const JsonNode & config, const int level)
	{
		JsonDeserializer deser(nullptr, config);
		effects->serializeJson(deser, level);
	}
};

class ConfigurableMechanicsFactory : public CustomMechanicsFactory
{
public:
	ConfigurableMechanicsFactory(const CSpell * s)
		: CustomMechanicsFactory(s)
	{
		for(int level = 0; level < GameConstants::SPELL_SCHOOL_LEVELS; level++)
			loadEffects(s->getLevelInfo(level).specialEffects, level);
	}
};

class CloneMechanicsFactory : public CustomMechanicsFactory
{
public:
	CloneMechanicsFactory(const CSpell * s)
		: CustomMechanicsFactory(s)
	{
		for(int level = 0; level < GameConstants::SPELL_SCHOOL_LEVELS; level++)
		{
			int maxTier = 1000;
			//tier 1-5 for basic, 1-6 for advanced, any level for expert
			if(level < 3)
			{
				maxTier = (std::max<int>(level, 1) + 4);
			}

			std::string effectName = "core:clone";
			JsonNode config(JsonNode::DATA_STRUCT);
			JsonSerializer ser(nullptr, config);

			auto guard = ser.enterStruct(effectName);
			ser.serializeString("type", effectName);
			ser.serializeInt("maxTier", maxTier);
			loadEffects(config, level);
		}
	}
};

class SummonMechanicsFactory : public CustomMechanicsFactory
{
public:
	SummonMechanicsFactory(const CSpell * s, CreatureID c)
		: CustomMechanicsFactory(s)
	{
		for(int level = 0; level < GameConstants::SPELL_SCHOOL_LEVELS; level++)
		{
			std::string effectName = "core:summon";
			JsonNode config(JsonNode::JsonType::DATA_STRUCT);
			JsonSerializer ser(nullptr, config);

			auto guard = ser.enterStruct(effectName);
			ser.serializeString("type", effectName);
			ser.serializeId("id", c, CreatureID());
			loadEffects(config, level);
		}
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
			assert(levelInfo.specialEffects.isNull());

			std::shared_ptr<effects::Effect> effect;

			if(s->isOffensiveSpell())
			{
				//default constructed object should be enough
				effect = std::make_shared<effects::Damage>(level);
			}

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
				effects->add(effect, level);
		}
	}
};

Destination::Destination()
	: stackValue(nullptr),
	hexValue(BattleHex::INVALID)
{

}

Destination::Destination(const CStack * destination)
	: stackValue(destination),
	hexValue(destination->position)
{

}

Destination::Destination(const BattleHex & destination)
	: stackValue(nullptr),
	hexValue(destination)
{

}

Destination::Destination(const Destination & other)
	: stackValue(other.stackValue),
	hexValue(other.hexValue)
{

}

Destination & Destination::operator=(const Destination & other)
{
	stackValue = other.stackValue;
	hexValue = other.hexValue;
	return *this;
}

IBattleCast::~IBattleCast() = default;

BattleCast::BattleCast(const CBattleInfoCallback * cb, const Caster * caster_, const Mode mode_, const CSpell * spell_)
	: spell(spell_),
	cb(cb),
	caster(caster_),
	mode(mode_),
	spellLvl(caster->getSpellSchoolLevel(mode, spell)),
	effectLevel(caster->getEffectLevel(mode, spell)),
	effectPower(caster->getEffectPower(mode, spell)),
	effectDuration(caster->getEnchantPower(mode, spell)),
	effectValue(caster->getEffectValue(mode, spell))
{
	vstd::abetween(spellLvl, 0, 3);
	vstd::abetween(effectLevel, 0, 3); //??? may be we can allow higher value here

	vstd::amax(effectPower, 0);
	vstd::amax(effectDuration, 0);
	vstd::amax(effectValue, 0);
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

void BattleCast::aimToHex(const BattleHex & destination)
{
	target.push_back(Destination(destination));
}

void BattleCast::aimToStack(const CStack * destination)
{
	if(nullptr == destination)
		logGlobal->error("BattleCast::aimToStack invalid stack.");
	else
		target.push_back(Destination(destination));
}

void BattleCast::applyEffects(const SpellCastEnvironment * env) const
{
	auto m = spell->battleMechanics(cb, mode, caster);
	m->applyEffects(env, *this);
}

void BattleCast::applyEffectsForced(const SpellCastEnvironment * env) const
{
	auto m = spell->battleMechanics(cb, mode, caster);
	m->applyEffectsForced(env, *this);
}

void BattleCast::cast(const SpellCastEnvironment * env)
{
	if(target.empty())
		aimToHex(BattleHex::INVALID);
	auto m = spell->battleMechanics(cb, mode, caster);

	std::vector <const CStack*> reflected;//for magic mirror

	{
		SpellCastContext ctx(m.get(), env, *this);

		ctx.beforeCast();

		m->cast(env, *this, ctx, reflected);

		ctx.afterCast();
	}

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
			int targetHex = (*RandomGeneratorUtil::nextItem(mirrorTargets, env->getRandomGenerator()))->position;

			BattleCast mirror(*this, attackedCre);
			mirror.aimToHex(targetHex);
			mirror.cast(env);
		}
	}
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

BattleHex BattleCast::getFirstDestinationHex() const
{
	if(target.empty())
	{
		logGlobal->error("Spell have no target.");
        return BattleHex::INVALID;
	}
	return target.at(0).hexValue;
}

int BattleCast::getEffectValue() const
{
	return (effectValue == 0) ? spell->calculateRawEffectValue(effectLevel, effectPower, 1) : effectValue;
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
	//TODO: use it
	//TODO: immunity special cases
	static const std::map<SpellID, std::vector<std::string>> SPECIAL_EFFECTS =
	{
		{
			SpellID::ANIMATE_DEAD,
			{
				"core:heal"//need configuration
			}
		},
		{
			SpellID::RESURRECTION,
			{
				"core:heal"//need configuration
			}
		},
		{
			SpellID::ANTI_MAGIC,
			{
				"core:timed",
				"core:dispel"//need configuration
			}
		},
		{
			SpellID::ACID_BREATH_DAMAGE,
			{
				"core:acid"
			}
		},
		{
			SpellID::CHAIN_LIGHTNING,
			{
				"core:chainDamage"
			}
		},
		{
			SpellID::CLONE,
			{
				"core:clone"
			}
		},
		{
			SpellID::CURE,
			{
				"core:dispel",//need configuration
				"core:heal"//need configuration
			}
		},
		{
			SpellID::DEATH_STARE,//custom immunity
			{
				"core:deathStare"
			}
		},
		{
			SpellID::DISPEL,//custom immunity
			{
				"core:dispel",//need configuration
				"core:removeObstacle"//need configuration
			}
		},
		{
			SpellID::DISPEL_HELPFUL_SPELLS,
			{
				"core:dispel"//need configuration
			}
		},
		{
			SpellID::EARTHQUAKE,
			{
				"core:catapult"
			}
		},
		{
			SpellID::FIRE_WALL,
			{
				"core:obstacle",//need configuration
				"core:damage"
			}
		},
		{
			SpellID::FORCE_FIELD,
			{
				"core:obstacle"//need configuration
			}
		},
		{
			SpellID::HYPNOTIZE,//custom immunity
			{
				"core:timed"
			}
		},
		{
			SpellID::LAND_MINE,
			{
				"core:obstacle",//need configuration
				"core:damage"
			}
		},
		{
			SpellID::QUICKSAND,
			{
				"core:obstacle"//need configuration
			}
		},
		{
			SpellID::REMOVE_OBSTACLE,
			{
				"core:removeObstacle"
			}
		},
		{
			SpellID::SACRIFICE,
			{
				"core:heal",//need configuration
				"core:removeStack"
			}
		},
		{
			SpellID::TELEPORT,
			{
				"core:teleport"
			}
		}
	};

	//ignore spell id if there are special effects
	if(s->hasSpecialEffects())
		return make_unique<ConfigurableMechanicsFactory>(s);

	//spells converted to configurable mechanics
	switch(s->id)
	{
	case SpellID::SUMMON_FIRE_ELEMENTAL:
		return make_unique<SummonMechanicsFactory>(s, CreatureID::FIRE_ELEMENTAL);
	case SpellID::SUMMON_EARTH_ELEMENTAL:
		return make_unique<SummonMechanicsFactory>(s, CreatureID::EARTH_ELEMENTAL);
	case SpellID::SUMMON_WATER_ELEMENTAL:
		return make_unique<SummonMechanicsFactory>(s, CreatureID::WATER_ELEMENTAL);
	case SpellID::SUMMON_AIR_ELEMENTAL:
		return make_unique<SummonMechanicsFactory>(s, CreatureID::AIR_ELEMENTAL);
	case SpellID::CLONE:
		return make_unique<CloneMechanicsFactory>(s);
	default:
		break;
	}

	//to be converted
	switch(s->id)
	{
	case SpellID::ANIMATE_DEAD:
	case SpellID::RESURRECTION:
		return make_unique<SpellMechanicsFactory<SpecialRisingSpellMechanics>>(s);
	case SpellID::ANTI_MAGIC:
		return make_unique<SpellMechanicsFactory<AntimagicMechanics>>(s);
	case SpellID::ACID_BREATH_DAMAGE:
		return make_unique<SpellMechanicsFactory<AcidBreathDamageMechanics>>(s);
	case SpellID::CHAIN_LIGHTNING:
		return make_unique<SpellMechanicsFactory<ChainLightningMechanics>>(s);
	case SpellID::CURE:
		return make_unique<SpellMechanicsFactory<CureMechanics>>(s);
	case SpellID::DEATH_STARE:
		return make_unique<SpellMechanicsFactory<DeathStareMechanics>>(s);
	case SpellID::DISPEL:
		return make_unique<SpellMechanicsFactory<DispellMechanics>>(s);
	case SpellID::DISPEL_HELPFUL_SPELLS:
		return make_unique<SpellMechanicsFactory<DispellHelpfulMechanics>>(s);
	case SpellID::EARTHQUAKE:
		return make_unique<SpellMechanicsFactory<EarthquakeMechanics>>(s);
	case SpellID::FIRE_WALL:
		return make_unique<SpellMechanicsFactory<FireWallMechanics>>(s);
	case SpellID::FORCE_FIELD:
		return make_unique<SpellMechanicsFactory<ForceFieldMechanics>>(s);
	case SpellID::HYPNOTIZE:
		return make_unique<SpellMechanicsFactory<HypnotizeMechanics>>(s);
	case SpellID::LAND_MINE:
		return make_unique<SpellMechanicsFactory<LandMineMechanics>>(s);
	case SpellID::QUICKSAND:
		return make_unique<SpellMechanicsFactory<QuicksandMechanics>>(s);
	case SpellID::REMOVE_OBSTACLE:
		return make_unique<SpellMechanicsFactory<RemoveObstacleMechanics>>(s);
	case SpellID::SACRIFICE:
		return make_unique<SpellMechanicsFactory<SacrificeMechanics>>(s);
	case SpellID::TELEPORT:
		return make_unique<SpellMechanicsFactory<TeleportMechanics>>(s);
	case SpellID::STONE_GAZE:
	case SpellID::POISON:
	case SpellID::BIND:
	case SpellID::DISEASE:
	case SpellID::PARALYZE:
	case SpellID::THUNDERBOLT:
	case SpellID::ACID_BREATH_DEFENSE:
	case SpellID::AGE:
		//do not touch original creature abilities for now
		//TODO: configurable log messages
		return make_unique<SpellMechanicsFactory<RegularSpellMechanics>>(s);
	default:
		return make_unique<FallbackMechanicsFactory>(s);
	}
}

///Mechanics
Mechanics::Mechanics(const CSpell * s, const CBattleInfoCallback * Cb, const Caster * caster_)
	: owner(s), cb(Cb), caster(caster_)
{
	casterStack = dynamic_cast<const CStack *>(caster);

	//FIXME: ensure caster and check for valid player and side
	casterSide = 0;
	if(caster)
		casterSide = cb->playerToSide(caster->getOwner()).get();
}

Mechanics::~Mechanics() = default;

BaseMechanics::BaseMechanics(const CSpell * s, const CBattleInfoCallback * Cb, const Caster * caster_)
	: Mechanics(s, Cb, caster_)
{
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
			auto hero = dynamic_cast<const CGHeroInstance *>(caster);

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
