﻿#include "stdafx.h"
#include "image.h"

Image::Image()
	: m_needFlip(false)
{
	memset(&m_data, 0, sizeof(m_data));
#ifdef USE_OPENGL
	m_data.magFilter = GL_LINEAR;
	m_data.minFilter = GL_LINEAR_MIPMAP_LINEAR;
	m_data.wrapS = GL_REPEAT;
	m_data.wrapT = GL_REPEAT;
#endif
}

Image::~Image()
{
	dispose();
}

ImageData& Image::getData()
{
	return m_data;
}

void Image::dispose()
{
	if (m_data.mip[0].data)
	{
		delete[] reinterpret_cast<GMbyte *>(m_data.mip[0].data);
		m_data.mip[0].data = nullptr;
	}
}

// 以下函数表示，作为材质时是否需要翻转y轴
bool Image::isNeedFlip()
{
	return m_needFlip;
}

void Image::setNeedFlip(bool b)
{
	m_needFlip = b;
}