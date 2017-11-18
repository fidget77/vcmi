/*
 * Effects.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "Effects.h"
#include "Registry.h"

#include "../ISpellMechanics.h"

#include "../../serializer/JsonSerializeFormat.h"


namespace spells
{
namespace effects
{

Effects::Effects() = default;

Effects::~Effects() = default;

void Effects::add(const std::string & name, std::shared_ptr<Effect> effect, const int level)
{
	effect->name = name;
	data.at(level)[name] = effect;
}

bool Effects::applicable(Problem & problem, const Mechanics * m) const
{
	//stop on first problem
	//require all not optional effects to be applicable

	bool res = true;
	bool res2 = false;

	auto callback = [&](const Effect * e, bool & stop)
	{
		if(e->applicable(problem, m))
		{
			res2 = true;
		}
		else if(!e->optional)
		{
			res = false;
			stop = true;
		}
	};

	forEachEffect(m->getEffectLevel(), callback);

	return res && res2;
}

bool Effects::applicable(Problem & problem, const Mechanics * m, const Target & aimPoint, const Target & spellTarget) const
{
	//stop on first problem
	//require all not optional effects to be applicable

	bool res = true;
	bool res2 = false;
	auto callback = [&](const Effect * e, bool & stop)
	{
		if(!e->automatic)
			return;

		EffectTarget target = e->transformTarget(m, aimPoint, spellTarget);

		if(e->applicable(problem, m, aimPoint, target))
		{
			res2 = true;
		}
		else if(!e->optional)
		{
			res = false;
			stop = true;
		}
	};

	forEachEffect(m->getEffectLevel(), callback);

	return res && res2;
}

void Effects::forEachEffect(const int level, const std::function<void(const Effect *, bool &)> & callback) const
{
	bool stop = false;
	for(auto one : data.at(level))
	{
		callback(one.second.get(), stop);
		if(stop)
			return;
	}
}

Effects::EffectsToApply Effects::prepare(const Mechanics * m, const Target & aimPoint, const Target & spellTarget) const
{
	EffectsToApply effectsToApply;

	auto callback = [&](const Effect * e, bool & stop)
	{
		bool applyThis = false;

		//todo: find a better way to handle such special cases

		if(m->getSpellIndex() == SpellID::RESURRECTION && e->name == "cure")
			applyThis = m->mode == Mode::CREATURE_ACTIVE;
		else
			applyThis = e->automatic;

		if(applyThis)
		{
			EffectTarget target = e->transformTarget(m, aimPoint, spellTarget);
			effectsToApply.push_back(std::make_pair(e, target));
		}
	};

	forEachEffect(m->getEffectLevel(), callback);

	return effectsToApply;
}

void Effects::serializeJson(JsonSerializeFormat & handler, const int level)
{
	assert(!handler.saving);

	const JsonNode & effectMap = handler.getCurrent();

	for(auto & p : effectMap.Struct())
	{
		const std::string & name = p.first;

		auto guard = handler.enterStruct(name);

		std::string type;
		handler.serializeString("type", type);

		auto factory = Registry::get()->find(type);
		if(!factory)
		{
			logGlobal->error("Unknown effect type '%s'", type);
			continue;
		}

		auto effect = std::shared_ptr<Effect>(factory->create(level));
		effect->serializeJson(handler);
		add(name, effect, level);
	}
}

} // namespace effects
} // namespace spells
