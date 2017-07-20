/*
 * {file}.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "Summon.h"
#include "Registry.h"

#include "../ISpellMechanics.h"
#include "../Problem.h"
#include "../../battle/CBattleInfoCallback.h"
#include "../../NetPacks.h"
#include "../../serializer/JsonSerializeFormat.h"

#include "../../CHeroHandler.h"
#include "../../mapObjects/CGHeroInstance.h"

static const std::string EFFECT_NAME = "summon";

namespace spells
{
namespace effects
{

VCMI_REGISTER_SPELL_EFFECT(Summon, EFFECT_NAME);

Summon::Summon()
	: GlobalEffect()
{

}

Summon::~Summon()
{

}

bool Summon::applicable(Problem & problem, const Mechanics * m, const CastParameters & p) const
{
	auto mode = p.mode;
	if(mode == ECastingMode::AFTER_ATTACK_CASTING || mode == ECastingMode::SPELL_LIKE_ATTACK || mode == ECastingMode::MAGIC_MIRROR)
	{
		//should not even try to do it
		MetaString text;
		text.addReplacement("Invalid spell cast attempt: spell %s, mode %d");
		text.addReplacement(m->owner->name);
		text.addReplacement2(mode);
		problem.add(std::move(text), Problem::CRITICAL);
		return false;
	}

	if(!exclusive)
		return true;

	//check if there are summoned elementals of other type

	auto otherSummoned = m->cb->battleGetStacksIf([p, this](const CStack * st)
	{
		return (st->owner == p.caster->getOwner())
			&& (vstd::contains(st->state, EBattleStackState::SUMMONED))
			&& (!st->isClone())
			&& (st->getCreature()->idNumber != creature);
	});

	if(!otherSummoned.empty())
	{
		const CStack * elemental = otherSummoned.front();

		MetaString text;
		text.addTxt(MetaString::GENERAL_TXT, 538);

		const CGHeroInstance * caster = p.casterHero;
		if(caster)
		{
			text.addReplacement(caster->name);

			text.addReplacement(MetaString::CRE_PL_NAMES, elemental->getCreature()->idNumber.toEnum());

			if(caster->type->sex)
				text.addReplacement(MetaString::GENERAL_TXT, 540);
			else
				text.addReplacement(MetaString::GENERAL_TXT, 539);

		}
		problem.add(std::move(text), Problem::NORMAL);
		return false;
	}

	return true;
}

void Summon::apply(const PacketSender * server, const Mechanics * m, const CastParameters & p) const
{
	BattleStackAdded bsa;
	bsa.creID = creature;
	bsa.side = p.casterSide;
	bsa.summoned = !permanent;
	bsa.pos = m->cb->getAvaliableHex(creature, p.casterSide);

	//TODO stack casting -> probably power will be zero; set the proper number of creatures manually
	int percentBonus = p.casterHero ? p.casterHero->valOfBonuses(Bonus::SPECIFIC_SPELL_DAMAGE, m->owner->id.toEnum()) : 0;
	//new feature - percentage bonus
	bsa.amount = m->owner->calculateRawEffectValue(p.spellLvl, 0, p.effectPower * (100 + percentBonus) / 100.0);

	if(bsa.amount)
		server->sendAndApply(&bsa);
	else
		server->complain("Summoning didn't summon any!");
}

void Summon::serializeJson(JsonSerializeFormat & handler)
{
	handler.serializeId("id", creature, CreatureID());
	handler.serializeBool("permanent", permanent);
	handler.serializeBool("exclusive", exclusive);
}

} // namespace effects
} // namespace spells
