﻿#include "stdafx.h"
#include "bspgameworld.h"
#include "character.h"
#include "gmengine/controllers/factory.h"
#include "gmengine/controllers/gamemachine.h"
#include "utilities/algorithm.h"
#include "gmengine/controllers/resource_container.h"
#include "gmdatacore/imagereader/imagereader.h"
#include "utilities/path.h"
#include "gmdatacore/imagebuffer.h"
#include "gmdatacore/bsp/bsp_shader_loader.h"

BSPGameWorld::BSPGameWorld()
{
	D(d);
	d.physics.reset(new BSPPhysicsWorld(this));
}

void BSPGameWorld::loadBSP(const char* bspPath)
{
	D(d);
	d.bspWorkingDirectory = Path::directoryName(bspPath);
	d.bsp.loadBsp(bspPath);
	importBSP();
}

void BSPGameWorld::setSky(AUTORELEASE GameObject* sky)
{
	D(d);
	d.sky = sky;
	appendObjectAndInit(sky);
}

GameObject* BSPGameWorld::getSky()
{
	D(d);
	return d.sky;
}

void BSPGameWorld::updateCamera()
{
	D(d);

	IGraphicEngine* engine = getGraphicEngine();
	Character* character = getMajorCharacter();
	character->updateCamera();
	CameraLookAt& lookAt = character->getLookAt();
	engine->updateCameraView(lookAt);
}

void BSPGameWorld::renderGameWorld()
{
	D(d);
	IGraphicEngine* engine = getGraphicEngine();
	engine->newFrame();
	updateCamera();
	drawAll();

	if (!d.ready)
		d.ready = true;
	else
		engine->drawObjects(d.drawingList);
}

PhysicsWorld* BSPGameWorld::physicsWorld()
{
	D(d);
	return d.physics;
}

void BSPGameWorld::setMajorCharacter(Character* character)
{
	D(d);
	d.physics->setCamera(character);
	GameWorld::setMajorCharacter(character);
}

void BSPGameWorld::calculateVisibleFaces()
{
	D(d);
	D_BASE(GameWorld, dbase);
	BSPRenderData& rd = d.render.renderData();

	Character* character = getMajorCharacter();
	PositionState pos = character->getPositionState();
	BSPData& bsp = d.bsp.bspData();

	rd.facesToDraw.clearAll();
	GMint cameraLeaf = calculateLeafNode(pos.position);
	GMint cameraCluster = bsp.leafs[cameraLeaf].cluster;

	for (int i = 0; i < bsp.numleafs; ++i)
	{
		//if the leaf is not in the PVS, continue
		if (!isClusterVisible(cameraCluster, bsp.leafs[i].cluster))
			continue;

		//if this leaf does not lie in the frustum, continue
		Character* character = getMajorCharacter();
		if (!character->getFrustum().isBoundingBoxInside(rd.leafs[i].boundingBoxVertices))
			continue;

		//loop through faces in this leaf and mark them to be drawn
		for (int j = 0; j < bsp.leafs[i].numLeafSurfaces; ++j)
		{
			rd.facesToDraw.set(bsp.leafsurfaces[bsp.leafs[i].firstLeafSurface + j]);
		}
	}
}

GMint BSPGameWorld::calculateLeafNode(const vmath::vec3& position)
{
	D(d);
	BSPData& bsp = d.bsp.bspData();

	int currentNode = 0;

	//loop until we find a negative index
	while (currentNode >= 0)
	{
		//if the camera is in front of the plane for this node, assign i to be the front node
		if (bsp.planes[bsp.nodes[currentNode].planeNum].classifyPoint(position) == POINT_IN_FRONT_OF_PLANE)
			currentNode = bsp.nodes[currentNode].children[0]; //front
		else
			currentNode = bsp.nodes[currentNode].children[1]; //back
	}

	//return leaf index
	return ~currentNode;
}

int BSPGameWorld::isClusterVisible(int cameraCluster, int testCluster)
{
	D(d);
	BSPData& bsp = d.bsp.bspData();
	BSPRenderData& rd = d.render.renderData();

	int index = cameraCluster * rd.visibilityData.bytesPerCluster + testCluster / 8;
	return rd.visibilityData.bitset[index] & (1 << (testCluster & 7));
}

// drawAll将所要需要绘制的对象放入列表
void BSPGameWorld::drawAll()
{
	D(d);
	d.drawingList.clear();
	drawSky();
	if (!DBG_INT(DRAW_ONLY_SKY))
	{
		if (DBG_INT(CALCULATE_BSP_FACE))
			calculateVisibleFaces();
		drawFaces();
	}
}

void BSPGameWorld::drawSky()
{
	D(d);
	if (d.sky)
		d.sky->getReadyForRender(d.drawingList);
}

void BSPGameWorld::drawFaces()
{
	D(d);
	BSPData& bsp = d.bsp.bspData();
	BSPRenderData& rd = d.render.renderData();

	//loop through faces
	for (GMint i = 0; i < bsp.numDrawSurfaces; ++i)
	{
		//if this face is to be drawn, draw it
		if (rd.facesToDraw.isSet(i))
			drawFace(i);
	}
}

void BSPGameWorld::drawFace(GMint idx)
{
	D(d);
	BSPData& bsp = d.bsp.bspData();
	BSPRenderData& rd = d.render.renderData();

	//look this face up in the face directory
	if (rd.faceDirectory[idx].faceType == 0)
		return;

	if (rd.faceDirectory[idx].faceType == MST_PLANAR)
		drawPolygonFace(rd.faceDirectory[idx].typeFaceNumber);

	if (rd.faceDirectory[idx].faceType == MST_TRIANGLE_SOUP)
		drawMeshFace(rd.faceDirectory[idx].typeFaceNumber);
	
	if (rd.faceDirectory[idx].faceType == MST_PATCH)
		drawPatch(rd.faceDirectory[idx].typeFaceNumber);
}

void BSPGameWorld::preparePolygonFace(int polygonFaceNumber)
{
	D(d);
	BSPRenderData& rd = d.render.renderData();

	BSP_Render_Face& polygonFace = rd.polygonFaces[polygonFaceNumber];
	GameObject* obj = nullptr;
	ASSERT(rd.polygonFaceObjects.find(&polygonFace) == rd.polygonFaceObjects.end());

	Material material = { 0 };
	material.Ka[0] = 1.0f; material.Ka[1] = 1.0f; material.Ka[2] = 1.0f;
	if (!setMaterialTexture(polygonFace, material))
	{
		gm_warning("polygon: %d texture missing.", polygonFaceNumber);
		return;
	}
	setMaterialLightmap(polygonFace.lightmapIndex, material);

	Object* coreObj;
	d.render.createObject(polygonFace, material, &coreObj);
	obj = new GameObject(coreObj);

	rd.polygonFaceObjects[&polygonFace] = obj;
	appendObjectAndInit(obj);
}

void BSPGameWorld::prepareMeshFace(int meshFaceNumber)
{
	D(d);
	BSPRenderData& rd = d.render.renderData();

	BSP_Render_Face& meshFace = rd.meshFaces[meshFaceNumber];
	GameObject* obj = nullptr;

	ASSERT(rd.meshFaceObjects.find(&meshFace) == rd.meshFaceObjects.end());
	Material material = { 0 };
	material.Ka[0] = 1.0f; material.Ka[1] = 1.0f; material.Ka[2] = 1.0f;
	if (!setMaterialTexture(meshFace, material))
	{
		gm_warning("mesh: %d texture missing.", meshFaceNumber);
		return;
	}
	setMaterialLightmap(meshFace.lightmapIndex, material);

	Object* coreObj;
	d.render.createObject(meshFace, material, &coreObj);
	obj = new GameObject(coreObj);
	rd.meshFaceObjects[&meshFace] = obj;
	appendObjectAndInit(obj);
}

void BSPGameWorld::preparePatch(int patchNumber)
{
	D(d);
	BSPData& bsp = d.bsp.bspData();
	BSPRenderData& rd = d.render.renderData();

	Material material = { 0 };
	material.Ka[0] = 1.0f; material.Ka[1] = 1.0f; material.Ka[2] = 1.0f;
	if (!setMaterialTexture(rd.patches[patchNumber], material))
	{
		gm_warning("patch: %d texture missing.", patchNumber);
		return;
	}
	setMaterialLightmap(rd.patches[patchNumber].lightmapIndex, material);

	for (int i = 0; i < rd.patches[patchNumber].numQuadraticPatches; ++i)
	{
		BSP_Render_BiquadraticPatch* biqp = &rd.patches[patchNumber].quadraticPatches[i];

		GameObject* obj = nullptr;
		ASSERT(rd.biquadraticPatchObjects.find(biqp) == rd.biquadraticPatchObjects.end());

		Object* coreObj = new Object();
		ChildObject* child = new ChildObject();
		child->setArrangementMode(ChildObject::Triangle_Strip);

		Component* component = new Component(child);
		component->getMaterial() = material;

		int numVertices = 2 * (biqp->tesselation + 1);
		for (int row = 0; row < biqp->tesselation; ++row)
		{
			component->beginFace();
			GLuint* idxStart = &biqp->indices[row * 2 * (biqp->tesselation + 1)];
			for (int i = 0; i < numVertices; i++)
			{
				GMuint idx = *(idxStart + i);
				BSP_Render_Vertex& vertex = biqp->vertices[idx];

				component->vertex(vertex.position[0], vertex.position[1], vertex.position[2]);
				component->uv(vertex.decalS, vertex.decalT);
				component->lightmap(vertex.lightmapS, vertex.lightmapT);
			}
			component->endFace();

		}
		child->appendComponent(component);

		coreObj->append(child);
		obj = new GameObject(coreObj);
		rd.biquadraticPatchObjects[biqp] = obj;
		appendObjectAndInit(obj);
	}
}

void BSPGameWorld::drawPolygonFace(int polygonFaceNumber)
{
	D(d);
	BSPData& bsp = d.bsp.bspData();
	BSPRenderData& rd = d.render.renderData();

	BSP_Render_Face& polygonFace = rd.polygonFaces[polygonFaceNumber];
	GameObject* obj = nullptr;
	auto findResult = rd.polygonFaceObjects.find(&polygonFace);
	if (findResult != rd.polygonFaceObjects.end())
		obj = (*findResult).second;
	else
		return;

	ASSERT(obj);
	obj->getReadyForRender(d.drawingList);
}

void BSPGameWorld::drawMeshFace(int meshFaceNumber)
{
	D(d);
	BSPData& bsp = d.bsp.bspData();
	BSPRenderData& rd = d.render.renderData();

	BSP_Render_Face& meshFace = rd.meshFaces[meshFaceNumber];
	GameObject* obj = nullptr;
	auto findResult = rd.meshFaceObjects.find(&meshFace);
	if (findResult != rd.meshFaceObjects.end())
		obj = (*findResult).second;
	else
		return;

	ASSERT(obj);
	obj->getReadyForRender(d.drawingList);
}

void BSPGameWorld::drawPatch(int patchNumber)
{
	D(d);
	BSPData& bsp = d.bsp.bspData();
	BSPRenderData& rd = d.render.renderData();

	for (int i = 0; i < rd.patches[patchNumber].numQuadraticPatches; ++i)
		draw(rd.patches[patchNumber].quadraticPatches[i]);
}

void BSPGameWorld::draw(BSP_Render_BiquadraticPatch& biqp)
{
	D(d);
	BSPRenderData& rd = d.render.renderData();

	GameObject* obj = nullptr;
	auto findResult = rd.biquadraticPatchObjects.find(&biqp);
	if (findResult != rd.biquadraticPatchObjects.end())
		obj = (*findResult).second;
	else
		return;

	ASSERT(obj);
	obj->getReadyForRender(d.drawingList);
}

template <typename T>
bool BSPGameWorld::setMaterialTexture(T face, REF Material& m)
{
	D(d);
	BSPData& bsp = d.bsp.bspData();
	GMuint textureid = face.textureIndex;
	GMuint lightmapid = face.lightmapIndex;
	const char* name = bsp.shaders[textureid].shader;

	// 先从地图Shaders中找，如果找不到，就直接读取材质
	if (!d.shaderLoader.findItem(name, lightmapid, &m.shader))
	{
		ResourceContainer* rc = getGameMachine()->getGraphicEngine()->getResourceContainer();
		TextureContainer& tc = rc->getTextureContainer();
		const TextureContainer::TextureItem* item = tc.find(bsp.shaders[textureid].shader);
		if (!item)
			return false;
		m.shader.texture.textures[TEXTURE_INDEX_AMBIENT].frames[0] = item->texture;
		m.shader.texture.textures[TEXTURE_INDEX_AMBIENT].frameCount = 1;
	}
	return true;
}

void BSPGameWorld::setMaterialLightmap(GMint lightmapid, REF Material& m)
{
	D(d);
	const GMint WHITE_LIGHTMAP = -1;
	ResourceContainer* rc = getGameMachine()->getGraphicEngine()->getResourceContainer();
	TextureContainer_ID& tc = rc->getLightmapContainer();
	const TextureContainer_ID::TextureItem* item = nullptr;
	if (m.shader.surfaceFlag & SURF_NOLIGHTMAP)
		item = tc.find(WHITE_LIGHTMAP);
	else
		item = lightmapid >= 0 ? tc.find(lightmapid) : tc.find(WHITE_LIGHTMAP);

	m.shader.texture.textures[TEXTURE_INDEX_LIGHTMAP].frames[0] = item->texture;
	m.shader.texture.textures[TEXTURE_INDEX_LIGHTMAP].frameCount = 1;
}

void BSPGameWorld::importBSP()
{
	D(d);
	d.render.generateRenderData(&d.bsp.bspData());
	initShaders();
	initLightmaps();
	initTextures();
	importEntities();
	prepareFaces();
	initialize();
	d.physics->initBSPPhysicsWorld();
}

void BSPGameWorld::initShaders()
{
	D(d);
	//TODO
	std::string scrPath(d.bspWorkingDirectory);
	scrPath.append("scr/");
	d.shaderLoader.init(scrPath.c_str(), this, &d.render.renderData());
	d.shaderLoader.load();
}

void BSPGameWorld::initTextures()
{
	D(d);
	BSPData& bsp = d.bsp.bspData();

	IFactory* factory = getGameMachine()->getFactory();
	ResourceContainer* rc = getGraphicEngine()->getResourceContainer();

	for (GMint i = 0; i < bsp.numShaders; i++)
	{
		BSPShader& shader = bsp.shaders[i];
		// 如果一个texture在shader中已经定义，那么不读取它了，而使用shader中的材质
		if (d.shaderLoader.findItem(shader.shader, 0, nullptr))
			continue;

		Image* tex = nullptr;
		std::string fn = d.bspWorkingDirectory;
		fn.append(shader.shader);

		if (findTexture(fn.c_str(), &tex))
		{
			ITexture* texture;
			factory->createTexture(tex, &texture);

			TextureContainer::TextureItem item;
			item.name = shader.shader;
			item.texture = texture;
			rc->getTextureContainer().insert(item);
		}
		else
		{
			gm_warning("Cannot find texture %s", fn.c_str());
		}
	}
}

bool BSPGameWorld::findTexture(const char* textureFilename, OUT Image** img)
{
	const int maxChars = 128;
	static char priorities[][maxChars] =
	{
		".jpg",
		".tga",
		".png",
		".bmp"
	};
	static GMint dem = sizeof(priorities) / maxChars / sizeof(GMbyte);

	for (GMint i = 0; i < dem; i++)
	{
		std::string fn(textureFilename);
		fn.append(priorities[i]);
		if (ImageReader::load(fn.c_str(), img))
		{
			gm_info("loaded texture %s", fn.c_str());
			return true;
		}
	}
	return false;
}

void BSPGameWorld::initLightmaps()
{
	D(d);
	BSPData& bsp = d.bsp.bspData();
	IFactory* factory = getGameMachine()->getFactory();
	ResourceContainer* rc = getGraphicEngine()->getResourceContainer();

	const int BSP_LIGHTMAP_EXT = 128;
	const int BSP_LIGHTMAP_SIZE = BSP_LIGHTMAP_EXT * BSP_LIGHTMAP_EXT * 3 * sizeof(GMbyte);
	int numLightmaps = bsp.numLightBytes / (BSP_LIGHTMAP_SIZE * sizeof(GMbyte));

	for (GMint i = 0; i < numLightmaps; i++)
	{
		GMbyte* lightmapBytes = bsp.lightBytes.data() + i * BSP_LIGHTMAP_SIZE;
		ImageBuffer* imgBuf = new ImageBuffer(BSP_LIGHTMAP_EXT, BSP_LIGHTMAP_EXT, BSP_LIGHTMAP_SIZE, lightmapBytes);
		ITexture* texture = nullptr;
		factory->createTexture(imgBuf, &texture);

		TextureContainer_ID::TextureItem item;
		item.id = i;
		item.texture = texture;
		rc->getLightmapContainer().insert(item);
	}

	{
		// Create a white lightmap id = -1
		GMbyte white[3] = { 0xff, 0xff, 0xff };
		ImageBuffer* whiteBuf = new ImageBuffer(1, 1, 3 * sizeof(GMbyte), white);
		ITexture* texture = nullptr;
		factory->createTexture(whiteBuf, &texture);

		TextureContainer_ID::TextureItem item;
		item.id = -1;
		item.texture = texture;
		rc->getLightmapContainer().insert(item);
	}
}

void BSPGameWorld::importEntities()
{
	D(d);
	BSPData& bsp = d.bsp.bspData();
	for (auto iter = bsp.entities.begin(); iter != bsp.entities.end(); iter++)
	{
		BSPGameWorldEntityReader::import(*iter, this);
	}
}

void BSPGameWorld::prepareFaces()
{
	D(d);
	BSPData& bsp = d.bsp.bspData();
	BSPRenderData& rd = d.render.renderData();

	//loop through faces
	for (GMint i = 0; i < bsp.numDrawSurfaces; ++i)
	{
		if (rd.faceDirectory[i].faceType == 0)
			return;

		if (rd.faceDirectory[i].faceType == MST_PLANAR)
			preparePolygonFace(rd.faceDirectory[i].typeFaceNumber);

		if (rd.faceDirectory[i].faceType == MST_TRIANGLE_SOUP)
			prepareMeshFace(rd.faceDirectory[i].typeFaceNumber);

		if (rd.faceDirectory[i].faceType == MST_PATCH)
			preparePatch(rd.faceDirectory[i].typeFaceNumber);
	}
}

const char* BSPGameWorld::bspWorkingDirectory()
{
	D(d);
	return d.bspWorkingDirectory.c_str();
}

BSPData& BSPGameWorld::bspData()
{
	D(d);
	return d.bsp.bspData();
}

BSPRenderData& BSPGameWorld::renderData()
{
	D(d);
	return d.render.renderData();
}