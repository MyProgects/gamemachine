﻿#include "stdafx.h"
#include "gmparticleeffects_cocos2d.h"
#include "foundation/gmasync.h"
#include "foundation/gamemachine.h"
#include <gmengine/gmcomputeshadermanager.h>

namespace
{
	const GMVec4 s_rotateStartVector = GMVec4(0, 1, 0, 1);
	static GMString s_gravityCode;
	static GMString s_gravityEntry;
	static GMString s_radialCode;
	static GMString s_radialEntry;
}

GMParticleEffect_Cocos2DImplBase::~GMParticleEffect_Cocos2DImplBase()
{
	D(d);
	D_BASE(db, GMParticleEffect_Cocos2D);
	if (const IRenderContext* context = db->emitter->getParticleSystem()->getContext();)
	{
		auto handles = {
			d->particles,
			d->particlesSRV,
			d->particlesResult,
			d->particlesUAV,
			d->particleCpuResult,
			d->constant,
		};

		for (auto iter = handles.begin(); iter != handles.end(); ++iter)
		{
			if (IComputeShaderProgram* prog = getComputeShaderProgram(context))
				prog->release(*iter);
		}
	}
}


void GMParticleEffect_Cocos2DImplBase::init()
{
	D_BASE(d, GMParticleEffect_Cocos2D);
	GMParticleEffect_Cocos2D::init();

	const IRenderContext* context = d->emitter->getParticleSystem()->getContext();
	getComputeShaderProgram(context);
}

bool GMParticleEffect_Cocos2DImplBase::GPUUpdate(GMDuration dt)
{
	D(d);
	D_BASE(db, GMParticleEffect_Cocos2D);
	// 获取计算着色器
	const IRenderContext* context = db->emitter->getParticleSystem()->getContext();
	IComputeShaderProgram* shaderProgram = getComputeShaderProgram(context);
	if (!shaderProgram)
		return false;

	GM_ALIGNED_16(struct) ConstantBuffer
	{
		GMVec3 emitterPosition;
		GMVec3 gravity;
		GMVec3 rotationAxis;
		GMfloat dt;
		GMint32 mode;
	};

	auto& particles = db->emitter->getParticles();
	if (particles.empty())
		return true;

	auto& progParticles = d->particles;
	auto& progParticlesSRV = d->particlesSRV;
	auto& progParticlesResult = d->particlesResult;
	auto& progParticlesUAV = d->particlesUAV;
	auto& particleCpuResult = d->particleCpuResult;
	auto& constant = d->constant;

	// 粒子信息
	if (!progParticles)
	{
		shaderProgram->createBuffer(sizeof(*particles[0].data()), gm_sizet_to_uint(particles.size()), nullptr, GMComputeBufferType::Structured, &progParticles);
		shaderProgram->createBufferShaderResourceView(progParticles, &progParticlesSRV);

		shaderProgram->createBuffer(sizeof(*particles[0].data()), gm_sizet_to_uint(particles.size()), nullptr, GMComputeBufferType::UnorderedStructured, &progParticlesResult);
		shaderProgram->createBufferUnorderedAccessView(progParticlesResult, &progParticlesUAV);
	}
	else
	{
		// 如果粒子数量变多了，则重新生成buffer
		GMsize_t sz = shaderProgram->getBufferSize(GMComputeBufferType::Structured, progParticles);
		if (sz < sizeof(*particles[0].data()) * (particles.size()))
		{
			shaderProgram->release(progParticles);
			shaderProgram->release(progParticlesSRV);
			shaderProgram->release(progParticlesResult);
			shaderProgram->release(progParticlesUAV);
			shaderProgram->createBuffer(sizeof(*particles[0].data()), gm_sizet_to_uint(particles.size()), particles.data(), GMComputeBufferType::Structured, &progParticles);
			shaderProgram->createBufferShaderResourceView(progParticles, &progParticlesSRV);
			shaderProgram->createBuffer(sizeof(*particles[0].data()), gm_sizet_to_uint(particles.size()), nullptr, GMComputeBufferType::UnorderedStructured, &progParticlesResult);
			shaderProgram->createBufferUnorderedAccessView(progParticlesResult, &progParticlesUAV);
		}
	}
	shaderProgram->setBuffer(progParticles, GMComputeBufferType::Structured, particles.data(), sizeof(*particles[0].data()) * gm_sizet_to_uint(particles.size()));
	shaderProgram->bindShaderResourceView(1, &progParticlesSRV);

	// 传入时间等变量
	ConstantBuffer c = { db->emitter->getEmitPosition(), getGravityMode().getGravity(), db->emitter->getRotationAxis(), dt, static_cast<GMint32>(getMotionMode()) };
	if (!constant)
		shaderProgram->createBuffer(sizeof(ConstantBuffer), 1, &c, GMComputeBufferType::Constant, &constant);
	shaderProgram->setBuffer(constant, GMComputeBufferType::Constant, &c, sizeof(ConstantBuffer));
	shaderProgram->bindConstantBuffer(constant);

	// 绑定结果
	shaderProgram->bindUnorderedAccessView(1, &progParticlesUAV);

	// 开始计算
	shaderProgram->dispatch(gm_sizet_to_uint(particles.size()), 1, 1);

	bool canReadFromGPU = shaderProgram->canRead(progParticlesResult);
	if (!canReadFromGPU)
	{
		if (particleCpuResult)
			shaderProgram->release(particleCpuResult);
		shaderProgram->createReadOnlyBufferFrom(progParticlesResult, &particleCpuResult);
	}

	// 处理结果
	{
		// 更新每个粒子的状态
		GMComputeBufferHandle resultHandle = canReadFromGPU ? progParticlesResult : particleCpuResult;
		if (!canReadFromGPU)
			shaderProgram->copyBuffer(resultHandle, progParticlesResult);
		typedef GM_PRIVATE_NAME(GMParticle_Cocos2D) ParticleData;
		const ParticleData* resultPtr = static_cast<ParticleData*>(shaderProgram->mapBuffer(resultHandle));
		memcpy_s(particles.data(), sizeof(ParticleData) * particles.size(), resultPtr, sizeof(*particles[0].data()) * particles.size());

		// 将存活的粒子放入临时容器，最后交换
		Vector<GMParticle_Cocos2D> tmp;
		tmp.reserve(particles.size());
		for (GMsize_t i = 0; i < particles.size(); ++i)
		{
			auto& particle = particles[i];
			if (particle.getRemainingLife() > 0)
			{
				tmp.push_back(particle);
			}
		}
		particles.swap(tmp);

		shaderProgram->unmapBuffer(resultHandle);
	}
	return true;
}

void GMGravityParticleEffect_Cocos2D::initParticle(GMParticle_Cocos2D* particle)
{
	D_BASE(d, GMParticleEffect_Cocos2D);
	GMParticleEffect_Cocos2D::initParticle(particle);

	GMfloat particleSpeed = d->emitter->getEmitSpeed() + d->emitter->getEmitSpeedV() * GMRandomMt19937::random_real(-1.f, 1.f);
	GMfloat angle = d->emitter->getEmitAngle() + d->emitter->getEmitAngleV() * GMRandomMt19937::random_real(-1.f, 1.f);

	GMQuat rotationQuat = Rotate(Radian(angle), d->emitter->getRotationAxis());
	particle->getGravityModeData().initialVelocity = Inhomogeneous(s_rotateStartVector * rotationQuat) * particleSpeed;
	particle->getGravityModeData().tangentialAcceleration = getGravityMode().getTangentialAcceleration() + getGravityMode().getTangentialAccelerationV() * GMRandomMt19937::random_real(-1.f, 1.f);
	particle->getGravityModeData().radialAcceleration = getGravityMode().getRadialAcceleration() + getGravityMode().getRadialAccelerationV() * GMRandomMt19937::random_real(-1.f, 1.f);
}

void GMGravityParticleEffect_Cocos2D::CPUUpdate(GMDuration dt)
{
	D_BASE(d, GMParticleEffect_Cocos2D);
	auto& particles = d->emitter->getParticles();
	for (auto iter = particles.begin(); iter != particles.end();)
	{
		GMParticle_Cocos2D& particle = *iter;
		particle.setRemainingLife(particle.getRemainingLife() - dt);
		if (particle.getRemainingLife() > 0)
		{
			GMVec3 offset = Zero<GMVec3>();
			GMVec3 radial = Zero<GMVec3>();
			GMVec3 tangential = Zero<GMVec3>();

			// 径向加速度
			if (!FuzzyCompare(particle.getChangePosition().getX(), 0)
				|| !FuzzyCompare(particle.getChangePosition().getY(), 0)
				|| !FuzzyCompare(particle.getChangePosition().getZ(), 0))
			{
				radial = Normalize(particle.getGravityModeData().initialVelocity);
			}
			tangential = radial;
			radial *= particle.getGravityModeData().radialAcceleration;

			GMfloat y = tangential.getX();
			tangential.setX(-tangential.getY());
			tangential.setY(y);
			tangential *= particle.getGravityModeData().tangentialAcceleration;

			// 计算合力
			offset = (radial + tangential + getGravityMode().getGravity()) * dt;
			
			// 移动粒子
			particle.getGravityModeData().initialVelocity += offset;
			particle.setChangePosition(particle.getChangePosition() + particle.getGravityModeData().initialVelocity * dt);

			particle.setColor(particle.getColor() + particle.getDeltaColor() * dt);
			particle.setSize(Max(0, particle.getSize() + particle.getDeltaSize() * dt));
			particle.setRotation(particle.getRotation() + particle.getDeltaRotation() * dt);

			if (getMotionMode() == GMParticleMotionMode::Relative)
			{
				// 跟随发射器
				particle.setPosition(particle.getChangePosition() + d->emitter->getEmitPosition() - particle.getStartPosition());
			}
			else
			{
				GM_ASSERT(getMotionMode() == GMParticleMotionMode::Free);
				particle.setPosition(particle.getChangePosition());
			}
			++iter;
		}
		else
		{
			iter = particles.erase(iter);
		}
	}
}

GMString GMGravityParticleEffect_Cocos2D::getCode()
{
	return s_gravityCode;
}

GMString GMGravityParticleEffect_Cocos2D::getEntry()
{
	return s_gravityEntry;
}

IComputeShaderProgram* GMGravityParticleEffect_Cocos2D::getComputeShaderProgram(const IRenderContext* context)
{
	D(d);
	if (getCode().isEmpty())
		return nullptr;

	return GMComputeShaderManager::instance().getComputeShaderProgram(context, GMCS_PARTICLE_GRAVITY, L".", getCode(), getEntry());
}

void GMGravityParticleEffect_Cocos2D::setDefaultCodeAndEntry(const GMString& code, const GMString& entry)
{
	s_gravityCode = code;
	s_gravityEntry = entry;
}

void GMRadialParticleEffect_Cocos2D::initParticle(GMParticle_Cocos2D* particle)
{
	D_BASE(d, GMParticleEffect_Cocos2D);
	GMParticleEffect_Cocos2D::initParticle(particle);

	GMfloat beginRadius = getRadiusMode().getBeginRadius() + getRadiusMode().getBeginRadiusV() * GMRandomMt19937::random_real(-1.f, 1.f);
	GMfloat endRadius = getRadiusMode().getEndRadius() + getRadiusMode().getEndRadiusV() * GMRandomMt19937::random_real(-1.f, 1.f);

	particle->getRadiusModeData().radius = beginRadius;
	particle->getRadiusModeData().deltaRadius = (endRadius - beginRadius) / particle->getRemainingLife();

	particle->getRadiusModeData().angle = d->emitter->getEmitAngle() + d->emitter->getEmitAngleV() * GMRandomMt19937::random_real(-1.f, 1.f);
	particle->getRadiusModeData().degressPerSecond = Radian(getRadiusMode().getSpinPerSecond() + getRadiusMode().getSpinPerSecondV() * GMRandomMt19937::random_real(-1.f, 1.f));
}

void GMRadialParticleEffect_Cocos2D::CPUUpdate(GMDuration dt)
{
	D_BASE(d, GMParticleEffect_Cocos2D);
	auto& particles = d->emitter->getParticles();
	for (auto iter = particles.begin(); iter != particles.end();)
	{
		GMParticle_Cocos2D& particle = *iter;
		particle.setRemainingLife(particle.getRemainingLife() - dt);
		if (particle.getRemainingLife() > 0)
		{
			particle.getRadiusModeData().angle += particle.getRadiusModeData().degressPerSecond * dt;
			particle.getRadiusModeData().radius += particle.getRadiusModeData().deltaRadius * dt;

			GMQuat rotationQuat = Rotate(particle.getRadiusModeData().angle, d->emitter->getRotationAxis());
			GMVec3 changePosition = particle.getChangePosition();
			changePosition = s_rotateStartVector * rotationQuat * particle.getRadiusModeData().radius;
			particle.setChangePosition(changePosition);

			if (getMotionMode() == GMParticleMotionMode::Relative)
			{
				// 跟随发射器
				particle.setPosition(particle.getChangePosition() + particle.getStartPosition());
			}
			else
			{
				GM_ASSERT(getMotionMode() == GMParticleMotionMode::Free);
				particle.setPosition(particle.getChangePosition() + d->emitter->getEmitPosition());
			}

			particle.setColor(particle.getColor() + particle.getDeltaColor() * dt);
			particle.setSize(Max(0, particle.getSize() + particle.getDeltaSize() * dt));
			particle.setRotation(particle.getRotation() + particle.getDeltaRotation() * dt);
			++iter;
		}
		else
		{
			iter = particles.erase(iter);
		}
	}
}

GMString GMRadialParticleEffect_Cocos2D::getCode()
{
	return s_radialCode;
}

GMString GMRadialParticleEffect_Cocos2D::getEntry()
{
	return s_radialEntry;
}

IComputeShaderProgram* GMRadialParticleEffect_Cocos2D::getComputeShaderProgram(const IRenderContext* context)
{
	D(d);
	if (getCode().isEmpty())
		return nullptr;

	return GMComputeShaderManager::instance().getComputeShaderProgram(context, GMCS_PARTICLE_RADIAL, L".", getCode(), getEntry());
}

void GMRadialParticleEffect_Cocos2D::setDefaultCodeAndEntry(const GMString& code, const GMString& entry)
{
	s_radialCode = code;
	s_radialEntry = entry;
}