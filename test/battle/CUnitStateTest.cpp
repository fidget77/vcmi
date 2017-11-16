/*
 * CUnitStateTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "mock/mock_BonusBearer.h"
#include "mock/mock_UnitInfo.h"
#include "mock/mock_UnitEnvironment.h"
#include "../../lib/battle/CUnitState.h"
#include "../../lib/CCreatureHandler.h"

static const int32_t DEFAULT_AMOUNT = 100;
static const int32_t DEFAULT_SPEED = 10;
static const BattleHex DEFAULT_POSITION = BattleHex(5, 5);

class UnitStateTest : public testing::Test
{
public:
	UnitInfoMock infoMock;
	UnitEnvironmentMock envMock;
	BonusBearerMock bonusMock;

	const CCreature * pikeman;

	battle::CUnitState subject;

	UnitStateTest()
		:infoMock(),
		envMock(),
		bonusMock(CBonusSystemNode::CREATURE),
		subject(&infoMock, &bonusMock, &envMock)
	{
		pikeman = CreatureID(0).toCreature();
	}

	void setDefaultExpectations()
	{
		using namespace testing;

		bonusMock.addNewBonus(std::make_shared<Bonus>(Bonus::PERMANENT, Bonus::STACKS_SPEED, Bonus::CREATURE_ABILITY, DEFAULT_SPEED, 0));

		EXPECT_CALL(infoMock, unitBaseAmount()).WillRepeatedly(Return(DEFAULT_AMOUNT));
		EXPECT_CALL(infoMock, creatureType()).WillRepeatedly(Return(pikeman));
		EXPECT_CALL(infoMock, unitMaxHealth()).WillRepeatedly(Return(pikeman->MaxHealth()));
	}

	void setRegularUnitExpectations()
	{
		using namespace testing;
		setDefaultExpectations();

		EXPECT_CALL(envMock, unitHasAmmoCart()).WillRepeatedly(Return(false));
	}

	void setShooterUnitExpectations(bool withCart = false)
	{
		using namespace testing;
		setDefaultExpectations();

		EXPECT_CALL(envMock, unitHasAmmoCart()).WillRepeatedly(Return(withCart));
	}

	void makeShooter(int32_t ammo)
	{
		bonusMock.addNewBonus(std::make_shared<Bonus>(Bonus::PERMANENT, Bonus::SHOOTER, Bonus::CREATURE_ABILITY, 1, 0));
		bonusMock.addNewBonus(std::make_shared<Bonus>(Bonus::PERMANENT, Bonus::SHOTS, Bonus::CREATURE_ABILITY, ammo, 0));
	}

	void initUnit()
	{
		subject.localInit();
		subject.position = DEFAULT_POSITION;
	}
};

TEST_F(UnitStateTest, initialRegular)
{
	setRegularUnitExpectations();
	initUnit();

	EXPECT_TRUE(subject.alive());
	EXPECT_TRUE(subject.ableToRetaliate());
	EXPECT_FALSE(subject.isGhost());
	EXPECT_FALSE(subject.isDead());
	EXPECT_FALSE(subject.isTurret());
	EXPECT_TRUE(subject.isValidTarget(true));
	EXPECT_TRUE(subject.isValidTarget(false));

	EXPECT_FALSE(subject.isClone());
	EXPECT_FALSE(subject.hasClone());

	EXPECT_FALSE(subject.canCast());
	EXPECT_FALSE(subject.isCaster());
	EXPECT_FALSE(subject.canShoot());
	EXPECT_FALSE(subject.isShooter());

	EXPECT_EQ(subject.getCount(), DEFAULT_AMOUNT);
	EXPECT_EQ(subject.getFirstHPleft(), pikeman->MaxHealth());
	EXPECT_EQ(subject.getKilled(), 0);
	EXPECT_EQ(subject.getAvailableHealth(), pikeman->MaxHealth() * DEFAULT_AMOUNT);
	EXPECT_EQ(subject.getTotalHealth(), subject.getAvailableHealth());

	EXPECT_EQ(subject.getPosition(), DEFAULT_POSITION);

	EXPECT_EQ(subject.getInitiative(), DEFAULT_SPEED);
	EXPECT_EQ(subject.getInitiative(123456), DEFAULT_SPEED);

	EXPECT_TRUE(subject.canMove());
	EXPECT_TRUE(subject.canMove(123456));
	EXPECT_FALSE(subject.defended());
	EXPECT_FALSE(subject.defended(123456));
	EXPECT_FALSE(subject.moved());
	EXPECT_FALSE(subject.moved(123456));
	EXPECT_TRUE(subject.willMove());
	EXPECT_TRUE(subject.willMove(123456));
	EXPECT_FALSE(subject.waited());
	EXPECT_FALSE(subject.waited(123456));
}

TEST_F(UnitStateTest, canShoot)
{
	setShooterUnitExpectations();
	makeShooter(1);
	initUnit();

	EXPECT_FALSE(subject.canCast());
	EXPECT_FALSE(subject.isCaster());
	EXPECT_TRUE(subject.canShoot());
	EXPECT_TRUE(subject.isShooter());

	subject.afterAttack(true, false);

	EXPECT_FALSE(subject.canShoot());
	EXPECT_TRUE(subject.isShooter());
}

TEST_F(UnitStateTest, canShootWithAmmoCart)
{
	setShooterUnitExpectations(true);
	makeShooter(1);
	initUnit();

	EXPECT_FALSE(subject.canCast());
	EXPECT_FALSE(subject.isCaster());
	EXPECT_TRUE(subject.canShoot());
	EXPECT_TRUE(subject.isShooter());

	subject.afterAttack(true, false);

	EXPECT_TRUE(subject.canShoot());
	EXPECT_TRUE(subject.isShooter());
}
