﻿#include "stdafx.h"
#include "gmcamera.h"
#include "gameobjects/gmspritegameobject.h"
#include "foundation/gamemachine.h"
#include <gmdxincludes.h>

void GMFrustum::setOrtho(GMfloat left, GMfloat right, GMfloat bottom, GMfloat top, GMfloat n, GMfloat f)
{
	D(d);
	d->type = GMFrustumType::Orthographic;
	d->parameters.left = left;
	d->parameters.right = right;
	d->parameters.bottom = bottom;
	d->parameters.top = top;
	d->n = n;
	d->f = f;

	d->projectionMatrix = Ortho(d->parameters.left, d->parameters.right, d->parameters.bottom, d->parameters.top, d->n, d->f);
	d->dirty = true;
}

void GMFrustum::setPerspective(GMfloat fovy, GMfloat aspect, GMfloat n, GMfloat f)
{
	D(d);
	d->type = GMFrustumType::Perspective;
	d->parameters.fovy = fovy;
	d->parameters.aspect = aspect;
	d->n = n;
	d->f = f;

	d->projectionMatrix = Perspective(d->parameters.fovy, d->parameters.aspect, d->n, d->f);
	d->dirty = true;
}

void GMFrustum::getPlanes(GMFrustumPlanes& planes) const
{
	D(d);
	const GMMat4& projection = getProjectionMatrix();
	const GMMat4& view = getViewMatrix();
	GMMat4 clipMat = view * projection;

	auto& runningState = GM.getRunningStates();
	GMVec4 f, n, left, right, top, bottom;

	GetFrustumPlanesFromProjectionViewModelMatrix(
		runningState.farZ,
		runningState.nearZ,
		clipMat,
		f,
		n,
		right,
		left,
		top,
		bottom
	);

	planes.rightPlane.normal = GMVec3(right);
	planes.rightPlane.intercept = right.getW();

	planes.leftPlane.normal = GMVec3(left);
	planes.leftPlane.intercept = left.getW();

	planes.topPlane.normal = GMVec3(top);
	planes.topPlane.intercept = top.getW();

	planes.bottomPlane.normal = GMVec3(bottom);
	planes.bottomPlane.intercept = bottom.getW();

	planes.nearPlane.normal = GMVec3(n);
	planes.nearPlane.intercept = n.getW();

	planes.farPlane.normal = GMVec3(f);
	planes.farPlane.intercept = f.getW();
}

//is a bounding box in the Frustum?
bool GMFrustum::isBoundingBoxInside(const GMFrustumPlanes& frustumPlanes, const GMVec3(&vertices)[8])
{
	const GMPlane* planes[] =
	{
		&frustumPlanes.farPlane,
		&frustumPlanes.nearPlane,
		&frustumPlanes.topPlane,
		&frustumPlanes.bottomPlane,
		&frustumPlanes.leftPlane,
		&frustumPlanes.rightPlane
	};

	for (int i = 0; i < 6; ++i)
	{
		//if a point is not behind this plane, try next plane
		if (planes[i]->classifyPoint(vertices[0]) != POINT_BEHIND_PLANE)
			continue;
		if (planes[i]->classifyPoint(vertices[1]) != POINT_BEHIND_PLANE)
			continue;
		if (planes[i]->classifyPoint(vertices[2]) != POINT_BEHIND_PLANE)
			continue;
		if (planes[i]->classifyPoint(vertices[3]) != POINT_BEHIND_PLANE)
			continue;
		if (planes[i]->classifyPoint(vertices[4]) != POINT_BEHIND_PLANE)
			continue;
		if (planes[i]->classifyPoint(vertices[5]) != POINT_BEHIND_PLANE)
			continue;
		if (planes[i]->classifyPoint(vertices[6]) != POINT_BEHIND_PLANE)
			continue;
		if (planes[i]->classifyPoint(vertices[7]) != POINT_BEHIND_PLANE)
			continue;

		//All vertices of the box are behind this plane
		return false;
	}

	return true;
}

void GMFrustum::updateViewMatrix(const GMMat4& viewMatrix)
{
	D(d);
	d->viewMatrix = viewMatrix;
	d->inverseViewMatrix = Inverse(viewMatrix);
	d->dirty = true;
}

const GMMat4& GMFrustum::getProjectionMatrix() const
{
	D(d);
	return d->projectionMatrix;
}

const GMMat4& GMFrustum::getViewMatrix() const
{
	D(d);
	return d->viewMatrix;
}

const GMMat4& GMFrustum::getInverseViewMatrix() const
{
	D(d);
	return d->inverseViewMatrix;
}

//////////////////////////////////////////////////////////////////////////
GMCamera::GMCamera()
{
	D(d);
	d->frustum.setPerspective(Radian(75.f), 1.333f, .1f, 3200);
	d->lookAt.position = GMVec3(0);
	d->lookAt.lookAt = GMVec3(0, 0, 1);
}

void GMCamera::setPerspective(GMfloat fovy, GMfloat aspect, GMfloat n, GMfloat f)
{
	D(d);
	d->frustum.setPerspective(fovy, aspect, n, f);
}

void GMCamera::setOrtho(GMfloat left, GMfloat right, GMfloat bottom, GMfloat top, GMfloat n, GMfloat f)
{
	D(d);
	d->frustum.setOrtho(left, right, bottom, top, n, f);
}

void GMCamera::updateViewMatrix()
{
	D(d);
	d->frustum.updateViewMatrix(::getViewMatrix(d->lookAt));
}

void GMCamera::lookAt(const GMCameraLookAt& lookAt)
{
	D(d);
	d->lookAt = lookAt;
	d->frustum.updateViewMatrix(::getViewMatrix(lookAt));
}

GMVec3 GMCamera::getRayToWorld(const GMRect& renderRect, GMint32 x, GMint32 y) const
{
	D(d);
	GMVec3 world = Unproject(
		GMVec3(x, y, 1),
		0,
		0,
		renderRect.width,
		renderRect.height,
		d->frustum.getProjectionMatrix(),
		d->frustum.getViewMatrix(),
		Identity<GMMat4>()
	);

	return world - d->lookAt.position;
}

void GMCamera::getPlanes(GMFrustumPlanes& planes)
{
	D(d);
	d->frustum.getPlanes(planes);
}

bool GMCamera::isBoundingBoxInside(const GMFrustumPlanes& planes, const GMVec3(&vertices)[8])
{
	return GMFrustum::isBoundingBoxInside(planes, vertices);
}

//////////////////////////////////////////////////////////////////////////
GMCameraUtility::GMCameraUtility(GMCamera* camera)
{
	setCamera(camera);
}

void GMCameraUtility::update(GMfloat yaw, GMfloat pitch)
{
	D(d);
	if (d->camera)
	{
		GMVec3 lookAt = d->lookAt;
		GMFloat4 f4_lookAt;
		lookAt.loadFloat4(f4_lookAt);

		// 不考虑roll，把lookAt投影到世界坐标系平面
		GMVec3 lookAt_z = GMVec3(f4_lookAt[0], 0, f4_lookAt[2]);
		// 计算pitch是否超出范围，不考虑roll
		GMfloat calculatedPitch = Asin(f4_lookAt[1]) + pitch;
		if (-d->limitPitch < calculatedPitch && calculatedPitch <= d->limitPitch)
		{
			// 找到lookAt_z垂直的一个向量，使用与世界坐标相同的坐标系
			GMVec3 lookAt_x = GMVec3(1, 0, 0) * GMQuat(GMVec3(0, 0, 1), lookAt_z);
			GMQuat qPitch = Rotate(-pitch, FastNormalize(lookAt_x));
			lookAt = lookAt * qPitch;
		}

		GMQuat qYaw = Rotate(-yaw, GMVec3(0, 1, 0));
		d->lookAt = FastNormalize(lookAt * qYaw);
		d->camera->lookAt(GMCameraLookAt(d->lookAt, d->position));
	}
	else
	{
		gm_warning(gm_dbg_wrap("No camera in GMCameraUtility instance."));
	}
}

void GMCameraUtility::setCamera(GMCamera* camera)
{
	D(d);
	d->camera = camera;
	if (camera)
	{
		d->position = camera->getLookAt().position;
		d->lookAt = camera->getLookAt().lookAt;
	}
}