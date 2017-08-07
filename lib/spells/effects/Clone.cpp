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

#include "Clone.h"
#include "Registry.h"
#include "../ISpellMechanics.h"
#include "../../CCreatureHandler.h"
#include "../../CStack.h"
#include "../../NetPacks.h"
#include "../../battle/CBattleInfoCallback.h"
#include "../../serializer/JsonSerializeFormat.h"

static const std::string EFFECT_NAME = "core:clone";

namespace spells
{
namespace effects
{

VCMI_REGISTER_SPELL_EFFECT(Clone, EFFECT_NAME);

Clone::Clone(const int level)
	: StackEffect(level), maxTier(0)
{
}

Clone::~Clone() = default;

void Clone::apply(const PacketSender * server, RNG & rng, const Mechanics * m, const BattleCast & p, const EffectTarget & target) const
{
	for(const Destination & dest : target)
	{
		const CStack * clonedStack = dest.stackValue;

		//we shall have all targets to be stacks
		if(!clonedStack)
		{
			server->complain("No target stack to clone! Invalid effect target transformation.");
			continue;
		}

		//should not happen, but in theory we might have stack took damage from other effects
		if(clonedStack->getCount() < 1)
			continue;

		auto hex = m->cb->getAvaliableHex(clonedStack->type->idNumber, m->casterSide);

		if(!hex.isValid())
		{
			server->complain("No place to put new clone!");
			break;
		}

		//TODO: generate stack ID before apply

		BattleStackAdded bsa;
		bsa.creID = clonedStack->type->idNumber;
		bsa.side = m->casterSide;
		bsa.summoned = true;
		bsa.pos = hex;
		bsa.amount = clonedStack->getCount();
		server->sendAndApply(&bsa);

		BattleSetStackProperty ssp;
		ssp.stackID = bsa.newStackID;//we know stack ID after apply
		ssp.which = BattleSetStackProperty::CLONED;
		ssp.val = 0;
		ssp.absolute = 1;
		server->sendAndApply(&ssp);

		ssp.stackID = clonedStack->ID;
		ssp.which = BattleSetStackProperty::HAS_CLONE;
		ssp.val = bsa.newStackID;
		ssp.absolute = 1;
		server->sendAndApply(&ssp);

		SetStackEffect sse;
		sse.stacks.push_back(bsa.newStackID);
		Bonus lifeTimeMarker(Bonus::N_TURNS, Bonus::NONE, Bonus::SPELL_EFFECT, 0, m->owner->id.num);
		lifeTimeMarker.turnsRemain = p.effectDuration;
		sse.effect.push_back(lifeTimeMarker);
		server->sendAndApply(&sse);
	}
}

bool Clone::isReceptive(const Mechanics * m, const CStack * s) const
{
	int creLevel = s->getCreature()->level;
	if(creLevel > maxTier)
		return false;

	//use default algorithm only if there is no mechanics-related problem
	return StackEffect::isReceptive(m, s);
}
bool Clone::isValidTarget(const Mechanics * m, const CStack * s) const
{
	//can't clone already cloned creature
	if(s->isClone())
		return false;
	//can`t clone if old clone still alive
	if(s->cloneID != -1)
		return false;

	return StackEffect::isValidTarget(m, s);
}

void Clone::serializeJsonEffect(JsonSerializeFormat & handler)
{
	handler.serializeInt("maxTier", maxTier);
}

} // namespace effects
} // namespace spells
