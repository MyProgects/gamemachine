﻿#ifndef __GMSPRITEGAMEOBJECT_H__
#define __GMSPRITEGAMEOBJECT_H__
#include "common.h"
#include "gmphysics/gmphysicsstructs.h"
#include "gmgameobject.h"

BEGIN_NS

typedef GMbyte GMMovement;

// Movements
#define MC_NONE 0
#define MC_FORWARD 1
#define MC_BACKWARD 2
#define MC_LEFT 4
#define MC_RIGHT 8
#define MC_MOVEMENT_COUNT 5
#define MC_JUMP 16

GM_PRIVATE_OBJECT(GMMoveRate)
{
	GMfloat moveRate[MC_MOVEMENT_COUNT];
};

class GMMoveRate : public GMObject
{
	DECLARE_PRIVATE(GMMoveRate)

public:
	GMMoveRate()
	{
		D(d);
		for (GMuint i = 0; i < MC_MOVEMENT_COUNT; i++)
		{
			d->moveRate[i] = 1;
		}
	}

	void setMoveRate(GMMovement action, GMfloat rate)
	{
		D(d);
		d->moveRate[(GMint)log((GMfloat)action)] = rate;
	}

	GMfloat getMoveRate(GMMovement action)
	{
		D(d);
		return d->moveRate[(GMint)log((GMfloat)action)];
	}

	void clear()
	{
		D(d);
		memset(&d->moveRate, sizeof(d->moveRate), 0);
	}
};

// GMSpriteGameObject
// GMSpriteGameObject是一个可受玩家控制的角色
// 它拥有体积、步长、视角等属性
// 游戏中的主人公是GMSpriteGameObject，GMCamera需要每一帧将自己同步到主人公的位置上来
GM_PRIVATE_OBJECT(GMSpriteGameObject)
{
	GMfloat radius;
	Frustum frustum;
	GMMovement moveDirection;
	GMMoveRate moveRate;
	GMfloat pitchLimitRadius;
	PositionState state;
	GMCommandVector3 moveCmdArgFB;
	GMCommandVector3 moveCmdArgLR;
};

class GMSpriteGameObject : public GMGameObject
{
	DECLARE_PRIVATE(GMSpriteGameObject)

public:
	GMSpriteGameObject(GMfloat radius);

public:
	virtual GMGameObjectType getType() { return GMGameObjectType::Sprite; }
	virtual void simulate() override;

public:
	void setJumpSpeed(const linear_math::Vector3& jumpSpeed);
	void setMoveSpeed(GMfloat moveSpeed);
	void action(GMMovement movement, const GMMoveRate& rate);
	void lookUp(GMfloat degree);
	void lookRight(GMfloat degree);
	void setPitchLimitDegree(GMfloat deg);
	const PositionState& getPositionState();

public:
	virtual void onAppendingObjectToWorld() override;

private:
	void moveForwardOrBackward(bool forward);
	void moveLeftOrRight(bool left);
	void update();
	void sendMoveCommand();
	void clearMoveArgs();
};


END_NS
#endif