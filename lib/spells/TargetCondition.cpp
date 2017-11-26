/*
 * TargetCondition.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "TargetCondition.h"

#include "ISpellMechanics.h"

#include "../GameConstants.h"
#include "../CBonusTypeHandler.h"
#include "../battle/CBattleInfoCallback.h"
#include "../battle/Unit.h"

#include "../serializer/JsonSerializeFormat.h"
#include "../VCMI_Lib.h"
#include "../CModHandler.h"


namespace spells
{

class BonusCondition : public TargetCondition::Item
{
public:
	BonusCondition(BonusTypeID type_);

protected:
	bool check(const Mechanics * m, const battle::Unit * target) const override;

private:
	BonusTypeID type;
};

class CreatureCondition : public TargetCondition::Item
{
public:
	CreatureCondition(CreatureID type_);

protected:
	bool check(const Mechanics * m, const battle::Unit * target) const override;

private:
	CreatureID type;
};

class AnitimagicCondition : public TargetCondition::Item
{
public:
	AnitimagicCondition();

protected:
	bool check(const Mechanics * m, const battle::Unit * target) const override;
};

class AbsoluteLevelCondition : public TargetCondition::Item
{
public:
	AbsoluteLevelCondition();

protected:
	bool check(const Mechanics * m, const battle::Unit * target) const override;
};

class ElementalCondition : public TargetCondition::Item
{
public:
	ElementalCondition();

protected:
	bool check(const Mechanics * m, const battle::Unit * target) const override;
};

class NormalLevelCondition : public TargetCondition::Item
{
public:
	NormalLevelCondition();

protected:
	bool check(const Mechanics * m, const battle::Unit * target) const override;
};

//for Hypnotize
class HealthValueCondition : public TargetCondition::Item
{
public:
	HealthValueCondition();

protected:
	bool check(const Mechanics * m, const battle::Unit * target) const override;
};

TargetCondition::Item::Item()
	: inverted(false),
	exclusive(false)
{

}

TargetCondition::Item::~Item() = default;

bool TargetCondition::Item::isReceptive(const Mechanics * m, const battle::Unit * target) const
{
	bool result = check(m, target);

	if(inverted)
		return !result;
	else
		return result;
}

TargetCondition::TargetCondition() = default;

TargetCondition::~TargetCondition() = default;

bool TargetCondition::isReceptive(const CBattleInfoCallback * cb, const Caster * caster, const Mechanics * m, const battle::Unit * target) const
{
	if(!check(absolute, m, target))
		return false;

	//check receptivity
	if(m->isPositiveSpell() && target->hasBonusOfType(Bonus::RECEPTIVE)) //accept all positive spells
		return true;

	//Orb of vulnerability
	//FIXME: Orb of vulnerability mechanics is not such trivial (issue 1791)
	const bool battleWideNegation = target->hasBonusOfType(Bonus::NEGATE_ALL_NATURAL_IMMUNITIES, 0);
	const bool heroNegation = target->hasBonusOfType(Bonus::NEGATE_ALL_NATURAL_IMMUNITIES, 1);
	//anyone can cast on artifact holder`s stacks
	if(heroNegation)
		return true;
	//this stack is from other player
	//todo: NEGATE_ALL_NATURAL_IMMUNITIES special cases: dispell, chain lightning
	else if(battleWideNegation)
	{
		if(!cb->battleMatchOwner(caster->getOwner(), target, false))
			return true;
	}

	if(!check(normal, m, target))
		return false;

	return true;
}

void TargetCondition::serializeJson(JsonSerializeFormat & handler)
{
	//TODO
	if(handler.saving)
	{
		logGlobal->error("Spell target condition saving is not supported");
		return;
	}

	absolute.clear();
	normal.clear();

	{
		static std::shared_ptr<Item> antimagicCondition = std::make_shared<AnitimagicCondition>();
		absolute.push_back(antimagicCondition);

		static std::shared_ptr<Item> alCondition = std::make_shared<AbsoluteLevelCondition>();
		absolute.push_back(alCondition);

		static std::shared_ptr<Item> elementalCondition = std::make_shared<ElementalCondition>();
		normal.push_back(elementalCondition);

		static std::shared_ptr<Item> nlCondition = std::make_shared<NormalLevelCondition>();
		normal.push_back(nlCondition);
	}

	{
		auto anyOf = handler.enterStruct("anyOf");
		loadConditions(anyOf->getCurrent(), false, false);
	}

	{
		auto allOf = handler.enterStruct("allOf");
		loadConditions(allOf->getCurrent(), true, false);
	}

	{
		auto noneOf = handler.enterStruct("noneOf");
		loadConditions(noneOf->getCurrent(), true, true);
	}
}

bool TargetCondition::check(const ItemVector & condition, const Mechanics * m, const battle::Unit * target) const
{
	bool nonExclusiveCheck = false;
	bool nonExclusiveExits = false;

	for(auto & item : condition)
	{
		if(item->exclusive)
		{
			if(!item->isReceptive(m, target))
				return false;
		}
		else
		{
			if(item->isReceptive(m, target))
				nonExclusiveCheck = true;
			nonExclusiveExits = true;
		}
	}

	if(nonExclusiveExits)
		return nonExclusiveCheck;
	else
		return true;
}

void TargetCondition::loadConditions(const JsonNode & source, bool exclusive, bool inverted)
{
	for(auto & keyValue : source.Struct())
	{
		bool isAbsolute;

		const JsonNode & value = keyValue.second;

		if(value.String() == "absolute")
			isAbsolute = true;
		else if(value.String() == "normal")
			isAbsolute = false;
		else
			continue;

		std::shared_ptr<Item> item;

		std::string scope, type, identifier;

		VLC->modh->parseIdentifier(keyValue.first, scope, type, identifier);

		if(type == "bonus")
		{
			//TODO: support custom bonus types

			auto it = bonusNameMap.find(identifier);
			if(it == bonusNameMap.end())
			{
				logMod->error("Invalid bonus type in spell target condition %s", keyValue.first);
				continue;
			}

			item = std::make_shared<BonusCondition>(it->second);
		}
		else if(type == "creature")
		{
			auto rawId = VLC->modh->identifiers.getIdentifier(scope, type, identifier, true);

			if(!rawId)
			{
				logMod->error("Invalid creature type in spell target condition %s", keyValue.first);
				continue;
			}

			item = std::make_shared<CreatureCondition>(CreatureID(rawId.get()));
		}
		else if(type == "healthValueSpecial")
		{
			item = std::make_shared<HealthValueCondition>();
		}
		else
		{
			logMod->error("Unsupported spell target condition %s", keyValue.first);
			continue;
		}

		item->exclusive = exclusive;
		item->inverted = inverted;

		if(isAbsolute)
			absolute.push_back(item);
		else
			normal.push_back(item);
	}
}

///AbsoluteLevelCondition
AbsoluteLevelCondition::AbsoluteLevelCondition()
{
	inverted = false;
	exclusive = true;
}

bool AbsoluteLevelCondition::check(const Mechanics * m, const battle::Unit * target) const
{
	std::stringstream cachingStr;
	cachingStr << "type_" << Bonus::SPELL_IMMUNITY << "subtype_" << m->getSpellIndex() << "addInfo_1";
	if(target->hasBonus(Selector::typeSubtypeInfo(Bonus::SPELL_IMMUNITY, m->getSpellIndex(), 1), cachingStr.str()))
		return false;
	return true;
}

///AnitimagicCondition
AnitimagicCondition::AnitimagicCondition()
{
	inverted = false;
	exclusive = true;
}

bool AnitimagicCondition::check(const Mechanics * m, const battle::Unit * target) const
{
	std::stringstream cachingStr;
	cachingStr << "type_" << Bonus::LEVEL_SPELL_IMMUNITY << "source_" << Bonus::SPELL_EFFECT;

	TBonusListPtr levelImmunitiesFromSpell = target->getBonuses(Selector::type(Bonus::LEVEL_SPELL_IMMUNITY).And(Selector::sourceType(Bonus::SPELL_EFFECT)), cachingStr.str());

	if(levelImmunitiesFromSpell->size() > 0 && levelImmunitiesFromSpell->totalValue() >= m->getSpellLevel() && m->getSpellLevel() > 0)
		return false;
	return true;
}

///BonusCondition
BonusCondition::BonusCondition(BonusTypeID type_)
	:type(type_)
{
}

bool BonusCondition::check(const Mechanics * m, const battle::Unit * target) const
{
	return target->hasBonus(Selector::type(type));
}

///CreatureCondition
CreatureCondition::CreatureCondition(CreatureID type_)
	:type(type_)
{
}

bool CreatureCondition::check(const Mechanics * m, const battle::Unit * target) const
{
	return target->creatureId() == type;
}

///ElementalCondition
ElementalCondition::ElementalCondition()
{
	inverted = true;
	exclusive = true;
}

bool ElementalCondition::check(const Mechanics * m, const battle::Unit * target) const
{
	bool elementalImmune = false;

	auto filter = m->getElementalImmunity();

	for(auto element : filter)
	{
		if(target->hasBonusOfType(element, 0)) //always resist if immune to all spells altogether
		{
			elementalImmune = true;
			break;
		}
		else if(!m->isPositiveSpell()) //negative or indifferent
		{
			if(target->hasBonusOfType(element, 1))
			{
				elementalImmune = true;
				break;
			}
		}
	}
	return elementalImmune;
}

///NormalLevelCondition
NormalLevelCondition::NormalLevelCondition()
{
	inverted = false;
	exclusive = true;
}

bool NormalLevelCondition::check(const Mechanics * m, const battle::Unit * target) const
{
	TBonusListPtr levelImmunities = target->getBonuses(Selector::type(Bonus::LEVEL_SPELL_IMMUNITY));

	if(target->hasBonusOfType(Bonus::SPELL_IMMUNITY, m->getSpellIndex())
		|| (levelImmunities->size() > 0 && levelImmunities->totalValue() >= m->getSpellLevel() && m->getSpellLevel() > 0))
		return false;
	return true;
}

///HealthValueCondition
HealthValueCondition::HealthValueCondition()
{
}

bool HealthValueCondition::check(const Mechanics * m, const battle::Unit * target) const
{
	//todo: maybe do not resist on passive cast
	//TODO: what with other creatures casting hypnotize, Faerie Dragons style?
	int64_t subjectHealth = target->getAvailableHealth();
	//apply 'damage' bonus for hypnotize, including hero specialty
	auto maxHealth = m->applySpellBonus(m->getEffectValue(), target);
	if(subjectHealth > maxHealth)
		return false;

	return true;
}


} // namespace spells
