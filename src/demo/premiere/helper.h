﻿#ifndef __HELPER_H__
#define __HELPER_H__
#include <gamemachine.h>
#include <gmanimation.h>
#include <gm2dgameobject.h>
#include <extensions/bsp/gmbspgameworld.h>

using namespace gm;

struct Helper
{
	static GMRect getMiddleRectOfWindow(const GMRect& rc, IWindow* window);
};

GM_ALIGNED_16(class) FontColorAnimationKeyframe : public GMAnimationKeyframe
{
	GM_DECLARE_ALIGNED_ALLOCATOR()

public:
	FontColorAnimationKeyframe(GMTextGameObject* textObject, const GMVec4& color, GMfloat timePoint);

public:
	virtual void reset(IDestroyObject* object) override;
	virtual void beginFrame(IDestroyObject* object, GMfloat timeStart) override;
	virtual void endFrame(IDestroyObject* object) override;
	virtual void update(IDestroyObject* object, GMfloat time) override;

private:
	GMTextGameObject* m_textObject;
	GMfloat m_timeStart;
	GMVec4 m_defaultColor;
	GMVec4 m_targetColor;
	Map<GMGameObject*, GMVec4, std::less<GMGameObject*>, AlignedAllocator<Pair<GMGameObject*, GMVec4>> > m_colorMap;
};

class GameObjectColorKeyframe : public GMAnimationKeyframe
{

public:
	GameObjectColorKeyframe(Map<GMModel*, GMVertices>& cache, const Array<GMfloat, 4>& color, GMfloat timePoint);

public:
	virtual void reset(IDestroyObject* object) override;
	virtual void beginFrame(IDestroyObject* object, GMfloat timeStart) override;
	virtual void endFrame(IDestroyObject* object) override;
	virtual void update(IDestroyObject* object, GMfloat time) override;

private:
	void update(GMModel*, const GMVertices&, const GMfloat color[4]);

private:
	Array<GMfloat, 4> m_color;
	Map<GMModel*, GMVertices>& m_cache;
	Map<GMModel*, Array<GMfloat, 4>> m_colorMap;
	GMfloat m_timeStart;
};

class BSPGameWorld : public GMBSPGameWorld
{
public:
	using GMBSPGameWorld::GMBSPGameWorld;

	virtual void setDefaultLights() override {} //do nothing
};

GM_ALIGNED_16(class) BSPGameObject : public GMGameObject
{
	GM_DECLARE_ALIGNED_ALLOCATOR()

public:
	BSPGameObject(const IRenderContext* context);

public:
	void load(const GMBuffer& buffer);
	bool isValid();

public:
	virtual void draw() override;
	virtual void update(GMDuration dt) override;
	virtual void onAppendingObjectToWorld() override;

private:
	GMOwnedPtr<BSPGameWorld> m_world;
	GMCameraLookAt m_lookAtCache;
};

class ScreenObject : public GMGameObject
{
public:
	struct TextOptions
	{
		GMFontHandle font;
		GMFontSizePt fontSize;
		GMFloat4 fontColor;
	};

public:
	ScreenObject(const IRenderContext* context);

public:
	virtual void draw() override;
	virtual void update(GMDuration dt) override;
	virtual void play() override;
	virtual void pause() override;
	virtual void reset(bool) override;
	virtual void onAppendingObjectToWorld() override;

public:
	void setSpacing(GMint32);
	void setSpeed(GMint32);
	void addText(const GMString& text, const TextOptions& options);
	void addImage(const GMBuffer& buffer);
	void addSpacing(GMint32);

private:
	GMOwnedPtr<GMGameWorld> m_world;
	Vector<GMGameObject*> m_objects;
	const IRenderContext* m_context;
	GMint32 m_spacing;
	GMint32 m_baseline;
	GMint32 m_maxWidth;

	bool m_playing;
	GMfloat m_rollingSpeed;
	GMfloat m_positionY;
	GMfloat m_finalPosition;
};
#endif