﻿#ifndef __GMASSETS_H__
#define __GMASSETS_H__
#include <gmcommon.h>
BEGIN_NS

struct ITexture;
class GMModel;
class GMScene;
class GMPhysicsShape;

// 默认的一些资产路径
#define GM_ASSET_TEXTURES	GMString(L"/textures/")
#define GM_ASSET_LIGHTMAPS	GMString(L"/lightmaps/")
#define GM_ASSET_MODELS		GMString(L"/models/")

//! 游戏资产的类型
/*!
  表示某资产的类型。
*/
enum class GMAssetType
{
	Invalid, //!< 无类型，绝对不会用到
	Texture, //!< 纹理贴图类型
	Model, //!< 模型类型
	Scene, //!< 一个模型场景，可能包含动画
	PhysicsShape, //!< 物理形状类型
};

#define GM_DECLARE_ASSET_GETTER(retType, funcName) \
	retType get##funcName(); \
	bool is##funcName();

#define GM_DEFINE_ASSET_GETTER(retType, funcName, predictType)		\
	retType GMAsset::get##funcName() {								\
		if (getType() == predictType)								\
			return static_cast<retType>(getAsset());				\
		GM_ASSERT(GMAssetType::Invalid == getType());				\
		return nullptr;												\
	}																\
	bool GMAsset::is##funcName() {									\
		return getType() == predictType;							\
	}

GM_PRIVATE_CLASS(GMAsset);
class GM_EXPORT GMAsset : public IDestroyObject
{
	GM_DECLARE_PRIVATE(GMAsset)

public:
	GM_DECLARE_ASSET_GETTER(ITexture*, Texture);
	GM_DECLARE_ASSET_GETTER(GMModel*, Model);
	GM_DECLARE_ASSET_GETTER(GMScene*, Scene);
	GM_DECLARE_ASSET_GETTER(GMPhysicsShape*, PhysicsShape);

public:
	GMAsset();
	GMAsset(GMAssetType type, void* asset);
	GMAsset(const GMAsset& asset);
	GMAsset(GMAsset&& asset) GM_NOEXCEPT;
	~GMAsset();

	GMAsset& operator=(const GMAsset& asset);
	GMAsset& operator=(GMAsset&& asset) GM_NOEXCEPT;

public:
	void* getAsset() const GM_NOEXCEPT;

	bool isEmpty() const GM_NOEXCEPT;

	GMAssetType getType() const GM_NOEXCEPT;

	void setType(GMAssetType type) GM_NOEXCEPT;

	template <typename T>
	T get()
	{
		return static_cast<T>(getAsset());
	}

private:
	void addRef();
	void release();
	void removeData();

public:
	static const GMAsset& invalidAsset();
};

inline bool operator ==(const GMAsset& a, const GMAsset& b) GM_NOEXCEPT
{
	return a.getType() == b.getType() && a.getAsset() == b.getAsset();
}

inline bool operator !=(const GMAsset& a, const GMAsset& b) GM_NOEXCEPT
{
	return !(a == b);
}

GM_PRIVATE_CLASS(GMAssets);
class GM_EXPORT GMAssets
{
	GM_DECLARE_PRIVATE(GMAssets)

public:
	GMAssets();
	~GMAssets();

public:
	GMAsset addAsset(GMAsset asset);
	GMAsset addAsset(const GMString& name, GMAsset asset);
	GMAsset getAsset(GMsize_t index);
	GMAsset getAsset(const GMString& name);
};

END_NS
#endif