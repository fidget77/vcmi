/*
 * TargetCondition.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

class JsonNode;
class JsonSerializeFormat;
class CBattleInfoCallback;

namespace battle
{
	class Unit;
}

namespace spells
{
class Caster;
class Mechanics;

class TargetCondition
{
public:
	class Item
	{
	public:
		bool inverted;
		bool exclusive;

		Item();
		virtual ~Item();

		virtual bool isReceptive(const Mechanics * m, const battle::Unit * target) const;
	protected:
		virtual bool check(const Mechanics * m, const battle::Unit * target) const = 0;
	};

	using ItemVector = std::vector<std::shared_ptr<Item>>;

	ItemVector normal;
	ItemVector absolute;

	TargetCondition();
	virtual ~TargetCondition();

	bool isReceptive(const CBattleInfoCallback * cb, const Caster * caster, const Mechanics * m, const battle::Unit * target) const;

	void serializeJson(JsonSerializeFormat & handler);
protected:

private:
	bool check(const ItemVector & condition, const Mechanics * m, const battle::Unit * target) const;

	void loadConditions(const JsonNode & source, bool exclusive, bool inverted);
};

} // namespace spells
