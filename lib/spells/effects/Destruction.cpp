/*
 * Destruction.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "Destruction.h"

#include "Registry.h"
#include "../ISpellMechanics.h"

#include "../../NetPacksBase.h"
#include "../../battle/Unit.h"
#include "../../serializer/JsonSerializeFormat.h"

static const std::string EFFECT_NAME = "core:destruction";

namespace spells
{
namespace effects
{

VCMI_REGISTER_SPELL_EFFECT(Destruction, EFFECT_NAME);

Destruction::Destruction(const int level)
	: Damage(level),
	killByPercentage(false)
{

}

Destruction::~Destruction() = default;

int64_t Destruction::damageForTarget(size_t targetIndex, const Mechanics * m, const battle::Unit * target) const
{
	//effect value = amount or percent of amount

	UNUSED(targetIndex);

	int64_t amountToKill = 0;

	if(killByPercentage)
	{
		amountToKill = target->getCount() * m->getEffectValue() / 100;
	}
	else
	{
		amountToKill = m->getEffectValue();
	}

	return amountToKill * target->MaxHealth();
}

void Destruction::describeEffect(std::vector<MetaString> & log, const Mechanics * m, const battle::Unit * firstTarget, uint32_t kills, int64_t damage, bool multiple) const
{
	if(m->getSpellIndex() == SpellID::DEATH_STARE)
	{
		MetaString line;
		if(kills > 1)
		{
			line.addTxt(MetaString::GENERAL_TXT, 119); //%d %s die under the terrible gaze of the %s.
			line.addReplacement(kills);
			firstTarget->addNameReplacement(line, true);
		}
		else
		{
			line.addTxt(MetaString::GENERAL_TXT, 118); //One %s dies under the terrible gaze of the %s.
			firstTarget->addNameReplacement(line, false);
		}
		m->caster->getCasterName(line);
		log.push_back(line);
	}
	else
	{
		Damage::describeEffect(log, m, firstTarget, kills, damage, multiple);
	}
}

void Destruction::serializeJsonDamageEffect(JsonSerializeFormat & handler)
{
	handler.serializeBool("killByPercentage", killByPercentage);
}

} // namespace effects
} // namespace spells
