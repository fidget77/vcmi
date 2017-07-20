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
#include "../battle/BattleInfo.h"

#include "../NetPacks.h"

#include "../serializer/JsonDeserializer.h"

#include "CDefaultSpellMechanics.h"

#include "AdventureSpellMechanics.h"
#include "BattleSpellMechanics.h"
#include "CreatureSpellMechanics.h"
#include "CustomSpellMechanics.h"

#include "effects/Effects.h"

template<typename Mechanics>
class SpellMechanicsFactory : public ISpellMechanicsFactory
{
public:
	SpellMechanicsFactory(const CSpell * s)
		: ISpellMechanicsFactory(s)
	{}

	std::unique_ptr<ISpellMechanics> create(const CBattleInfoCallback * cb) const override
	{
		return make_unique<Mechanics>(spell, cb);
	}
};

template<>
class SpellMechanicsFactory<SummonMechanics> : public ISpellMechanicsFactory
{
public:
	SpellMechanicsFactory(const CSpell * s, CreatureID c)
		: ISpellMechanicsFactory(s), cre(c)
	{}

	std::unique_ptr<ISpellMechanics> create(const CBattleInfoCallback * cb) const override
	{
		return make_unique<SummonMechanics>(spell, cb, cre);
	}

private:
	CreatureID cre;
};

template<>
class SpellMechanicsFactory<CustomSpellMechanics> : public ISpellMechanicsFactory
{
public:
	SpellMechanicsFactory(const CSpell * s)
		: ISpellMechanicsFactory(s), effects()
	{
		effects = std::make_shared<spells::effects::Effects>();

		for(int level = 0; level < GameConstants::SPELL_LEVELS; level++)
		{
			const CSpell::LevelInfo & levelInfo = s->getLevelInfo(level);

			JsonDeserializer deser(nullptr, levelInfo.specialEffects);
			effects->serializeJson(deser, level);
		}
	}

	std::unique_ptr<ISpellMechanics> create(const CBattleInfoCallback * cb) const override
	{
		return make_unique<CustomSpellMechanics>(spell, cb, effects);
	}
private:
	std::shared_ptr<spells::effects::Effects> effects;
};

BattleSpellCastParameters::Destination::Destination(const CStack * destination):
	stackValue(destination),
	hexValue(destination->position)
{

}

BattleSpellCastParameters::Destination::Destination(const BattleHex & destination):
	stackValue(nullptr),
	hexValue(destination)
{

}

BattleSpellCastParameters::BattleSpellCastParameters(const BattleInfo * cb, const ISpellCaster * caster, const CSpell * spell_)
	: spell(spell_), cb(cb), caster(caster), casterColor(caster->getOwner()), casterSide(cb->whatSide(casterColor)),
	casterHero(nullptr),
	mode(ECastingMode::HERO_CASTING), casterStack(nullptr),
	spellLvl(0),  effectLevel(0), effectPower(0), enchantPower(0), effectValue(0)
{
	casterStack = dynamic_cast<const CStack *>(caster);
	casterHero = dynamic_cast<const CGHeroInstance *>(caster);

	spellLvl = caster->getSpellSchoolLevel(spell);
	effectLevel = caster->getEffectLevel(spell);
	effectPower = caster->getEffectPower(spell);
	effectValue = caster->getEffectValue(spell);
	enchantPower = caster->getEnchantPower(spell);

	vstd::amax(spellLvl, 0);
	vstd::amax(effectLevel, 0);
	vstd::amax(enchantPower, 0);
	vstd::amax(enchantPower, 0);
	vstd::amax(effectValue, 0);
}

BattleSpellCastParameters::BattleSpellCastParameters(const BattleSpellCastParameters & orig, const ISpellCaster * caster)
	:spell(orig.spell), cb(orig.cb), caster(caster), casterColor(caster->getOwner()), casterSide(cb->whatSide(casterColor)),
	casterHero(nullptr), mode(ECastingMode::MAGIC_MIRROR), casterStack(nullptr),
	spellLvl(orig.spellLvl),  effectLevel(orig.effectLevel), effectPower(orig.effectPower), enchantPower(orig.enchantPower), effectValue(orig.effectValue)
{
	casterStack = dynamic_cast<const CStack *>(caster);
	casterHero = dynamic_cast<const CGHeroInstance *>(caster);
}

void BattleSpellCastParameters::aimToHex(const BattleHex& destination)
{
	destinations.push_back(Destination(destination));
}

void BattleSpellCastParameters::aimToStack(const CStack * destination)
{
	if(nullptr == destination)
		logGlobal->error("BattleSpellCastParameters::aimToStack invalid stack.");
	else
		destinations.push_back(Destination(destination));
}

void BattleSpellCastParameters::cast(const SpellCastEnvironment * env)
{
	if(destinations.empty())
		aimToHex(BattleHex::INVALID);
	spell->battleCast(env, *this);
}

bool BattleSpellCastParameters::castIfPossible(const SpellCastEnvironment * env)
{
	if(ESpellCastProblem::OK == spell->canBeCast(cb, mode, caster))
	{
		cast(env);
		return true;
	}
	return false;
}

BattleHex BattleSpellCastParameters::getFirstDestinationHex() const
{
	if(destinations.empty())
	{
		logGlobal->error("Spell have no destinations.");
        return BattleHex::INVALID;
	}
	return destinations.at(0).hexValue;
}

int BattleSpellCastParameters::getEffectValue() const
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
	//ignore spell id if there are special effects
	if(s->hasSpecialEffects())
		return make_unique<SpellMechanicsFactory<CustomSpellMechanics>>(s);

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
	case SpellID::CLONE:
		return make_unique<SpellMechanicsFactory<CloneMechanics>>(s);
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
	case SpellID::SUMMON_FIRE_ELEMENTAL:
		return make_unique<SpellMechanicsFactory<SummonMechanics>>(s, CreatureID::FIRE_ELEMENTAL);
	case SpellID::SUMMON_EARTH_ELEMENTAL:
		return make_unique<SpellMechanicsFactory<SummonMechanics>>(s, CreatureID::EARTH_ELEMENTAL);
	case SpellID::SUMMON_WATER_ELEMENTAL:
		return make_unique<SpellMechanicsFactory<SummonMechanics>>(s, CreatureID::WATER_ELEMENTAL);
	case SpellID::SUMMON_AIR_ELEMENTAL:
		return make_unique<SpellMechanicsFactory<SummonMechanics>>(s, CreatureID::AIR_ELEMENTAL);
	case SpellID::TELEPORT:
		return make_unique<SpellMechanicsFactory<TeleportMechanics>>(s);
	default:
		return make_unique<SpellMechanicsFactory<DefaultSpellMechanics>>(s);
	}
}


///ISpellMechanics
ISpellMechanics::ISpellMechanics(const CSpell * s, const CBattleInfoCallback * Cb)
	: owner(s), cb(Cb)
{
}

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
