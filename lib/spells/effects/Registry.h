/*
 * Registry.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "Effect.h"

#define VCMI_REGISTER_SPELL_EFFECT(Type, Name) \
namespace\
{\
RegisterEffect<Type> register ## Type(Name);\
}\
\

namespace spells
{
namespace effects
{

class DLL_LINKAGE Registry
{
	using DataPtr = std::unique_ptr<IEffectFactory>;
public:
	Registry();
	virtual ~Registry();

	const IEffectFactory * find(const std::string & name) const;

    template<typename F>
    void add(const std::string & name)
    {
		data[name].reset(new EffectFactory<F>());
    }

    static Registry * get();
protected:

private:
	std::map<std::string, DataPtr> data;
};

template<typename F>
class RegisterEffect
{
public:
	RegisterEffect(const std::string & name)
	{
		Registry::get()->add<F>(name);
	}
};

} // namespace effects
} // namespace spells
