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
class CBattleInfoCallback;
class JsonSerializeFormat;
class CRandomGenerator;

namespace spells
{

namespace effects
{
using RNG = ::CRandomGenerator;
class Effects;
class IEffect;
class Registry;
template<typename F>
class RegisterEffect;

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
	bool automatic;
	bool optional;

	IEffect();
	virtual ~IEffect() = default;

	virtual void addTo(Effects * where, const int level) = 0;

	virtual bool applicable(Problem & problem, const Mechanics * m) const;
	virtual bool applicable(Problem & problem, const Mechanics * m, const Target & aimPoint, const EffectTarget & target) const;

	virtual void apply(const PacketSender * server, RNG & rng, const Mechanics * m, const BattleCast & p, const EffectTarget & target) const = 0;

	virtual EffectTarget filterTarget(const Mechanics * m, const BattleCast & p, const EffectTarget & target) const = 0;

	virtual EffectTarget transformTarget(const Mechanics * m, const Target & aimPoint, const Target & spellTarget) const = 0;

	void serializeJson(JsonSerializeFormat & handler);

protected:
	int spellLevel;

	virtual void serializeJsonEffect(JsonSerializeFormat & handler) = 0;
};

//default is location target
template <TargetType TType>
class Effect : public IEffect
{
public:
};

template<>
class Effect<TargetType::NO_TARGET> : public IEffect
{
public:
};

template<>
class Effect<TargetType::CREATURE> : public IEffect
{
public:
};


} // namespace effects
} // namespace spells
