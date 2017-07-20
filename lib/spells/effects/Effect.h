/*
 * Effect.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "../Magic.h"

class BattleHex;
class PacketSender;
class CBattleInfoCallback;
class BattleSpellCastParameters;
class ISpellMechanics;
class JsonSerializeFormat;

namespace spells
{

using Mechanics = ::ISpellMechanics;
using CastParameters = ::BattleSpellCastParameters;

namespace effects
{
class Effects;
class IEffect;

//using TargetType = ::CSpell::ETargetType;//todo: use this after ETargetType moved to better place

enum class TargetType
{
	NO_TARGET,
	LOCATION,
	CREATURE
};

class DLL_LINKAGE IEffectFactory
{
public:
	IEffectFactory() = default;
	virtual ~IEffectFactory() = default;

	virtual std::shared_ptr<IEffect> create() const = 0;
};

template<typename E>
class EffectFactory : public IEffectFactory
{
public:
	EffectFactory() = default;
	virtual ~EffectFactory() = default;

	std::shared_ptr<IEffect> create() const override
	{
        return std::make_shared<E>();
	}
};

class DLL_LINKAGE IEffect
{
public:

	virtual ~IEffect() = default;

	virtual void addTo(Effects * where, const int level) = 0;

	virtual bool applicable(Problem & problem, const Mechanics * m) const;

	virtual void serializeJson(JsonSerializeFormat & handler) = 0;
};

//default is location target
template <TargetType TType>
class Effect : public IEffect
{
public:
	virtual ~Effect() = default;

	virtual void apply(const PacketSender * server, const Mechanics * m, const BattleHex & target) const = 0;
};

template<>
class Effect<TargetType::NO_TARGET> : public IEffect
{
public:
	virtual ~Effect() = default;

	virtual void apply(const PacketSender * server, const Mechanics * m, const CastParameters & p) const = 0;

	virtual bool applicable(Problem & problem, const Mechanics * m, const CastParameters & p) const
	{
		return true;
	}
};

template<>
class Effect<TargetType::CREATURE> : public IEffect
{
public:
	virtual ~Effect() = default;

	virtual void apply(const PacketSender * server, const Mechanics * m, const std::vector<const CStack *> & target) const = 0;
};


} // namespace effects
} // namespace spells
