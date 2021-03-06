﻿#ifndef __BSPPHYSICSSTRUCTS_H__
#define __BSPPHYSICSSTRUCTS_H__
#include <gmcommon.h>
#include <linearmath.h>
#include <extensions/bsp/gmbsp.h>
#include <gmphysicsworld.h>
BEGIN_NS

#define PlaneTypeForNormal(x) (x[0] == 1.0 ? PLANE_X : (x[1] == 1.0 ? PLANE_Y : (x[2] == 1.0 ? PLANE_Z : PLANE_NON_AXIAL) ) )

inline GMint32 signbitsForNormal(const GMVec4& normal)
{
	GMint32 bits = 0;
	if (normal.getX() < 0)
		bits |= 1 << 0;
	if (normal.getY() < 0)
		bits |= 1 << 1;
	if (normal.getZ() < 0)
		bits |= 1 << 2;
	return bits;
}

enum PlaneType
{
	PLANE_X = 0,
	PLANE_Y,
	PLANE_Z,
	PLANE_NON_AXIAL,
};

GM_ALIGNED_STRUCT(GMBSPLeafList)
{
	GMVec3 bounds[2];
	AlignedVector<GMint32> list;
	GMint32 lastLeaf;
};

struct BSPTracePlane;
GM_ALIGNED_STRUCT(GMBSP_Physics_BrushSide)
{
	GMBSPBrushSide* side;
	BSPTracePlane* plane;
	GMint32 surfaceFlags;
};

GM_ALIGNED_STRUCT(GMBSP_Physics_Brush)
{
	GMBSP_Physics_Brush()
		: checkcount(0)
	{
	}

	GMBSPBrush* brush;
	GMint32 contents;
	GMVec3 bounds[2];
	GMBSP_Physics_BrushSide *sides;
	GMint32 checkcount;
};

// Begin patches definitions
GM_ALIGNED_STRUCT(GMBSPPatchPlane)
{
	GMVec4 plane;
	GMint32 signbits;		// signx + (signy<<1) + (signz<<2), used as lookup during collision
};

GM_ALIGNED_STRUCT(GMBSPFacet)
{
	GMint32 surfacePlane;
	GMint32 numBorders;		// 3 or four + 6 axial bevels + 4 or 3 * 4 edge bevels
	GMint32 borderPlanes[4 + 6 + 16];
	GMint32 borderInward[4 + 6 + 16];
	bool borderNoAdjust[4 + 6 + 16];
};

GM_ALIGNED_STRUCT(GMBSPPatchCollide)
{
	GMVec3 bounds[2];
	AlignedVector<GMBSPPatchPlane> planes;
	AlignedVector<GMBSPFacet> facets;
};
// End patches definitions

GM_ALIGNED_STRUCT(GMBSP_Physics_Patch)
{
	GMBSPSurface* surface = nullptr;
	GMBSPShader* shader = nullptr;
	GMBSPPatchCollide *pc = nullptr;
	GMint32 checkcount = 0;

	GMBSP_Physics_Patch()
		: checkcount(0)
		, pc(nullptr)
	{
	}

	~GMBSP_Physics_Patch()
	{
		if (pc)
			delete pc;
	}
};

GM_ALIGNED_STRUCT(GMBSPShapeProperties)
{
	GMVec3 bounding[2]; //!< 最小边界和最大边界，用于碰撞检测
	GMfloat stepHeight;
};

GM_PRIVATE_CLASS(GMBSPPhysicsObject);
class GMBSPPhysicsObject : public GMPhysicsObject
{
	GM_DECLARE_PRIVATE(GMBSPPhysicsObject)
	GM_DECLARE_BASE(GMPhysicsObject)
	GM_FRIEND_CLASS(GMBSPPhysicsWorld);

public:
	GMBSPPhysicsObject();
	~GMBSPPhysicsObject();

public:
	//! 形状参数
	/*!
	  用户可以直接修改拿到的形状参数。
	  \return 当前物理对象的形状参数。
	*/
	GMBSPShapeProperties& shapeProperties();
};

END_NS
#endif