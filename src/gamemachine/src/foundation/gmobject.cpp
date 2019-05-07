﻿#include "stdafx.h"
#include "gmobject.h"
#include "interfaces.h"
#include <utility>

template <typename ContainerType>
static size_t removeIf(ContainerType& container, std::function<bool(typename ContainerType::iterator)> pred)
{
	size_t cnt = 0;
	auto iter = container.begin();
	for (; iter != container.end(); iter++)
	{
		if (pred(iter))
		{
			iter = container.erase(iter);
			cnt++;
			if (iter == container.end())
				break;
		}
	}
	return cnt;
}

GMObject::~GMObject()
{
	releaseConnections();
}

const GMMeta* GMObject::meta() const
{
	D(d);
	if (!d->metaRegistered)
	{
		d->metaRegistered = true;
		if (!const_cast<GMObject*>(this)->registerMeta())
			return nullptr;
	}
	return &d->meta;
}

void GMObject::connect(GMObject& sender, GMSignal sig, const GMEventCallback& callback)
{
	sender.addConnection(std::move(sig), *this, callback);
}

void GMObject::disconnect(GMObject& sender, GMSignal sig)
{
	sender.removeSignalAndConnection(std::move(sig), *this);
}

void GMObject::addConnection(GMSignal sig, GMObject& receiver, GMEventCallback callback)
{
	D(d);
	GMCallbackTarget target = { &receiver, callback };
	d->objSlots[sig].push_back(target);
	receiver.addConnection(this, std::move(sig));
}

void GMObject::removeSignalAndConnection(GMSignal sig, GMObject& receiver)
{
	D(d);
	// 移除自己的接收者
	removeSignal(sig, receiver);

	// 移除自己连接到的信号
	receiver.removeConnection(this, std::move(sig));
}

void GMObject::emitSignal(GMSignal sig)
{
	D(d);
	if (d->objSlots.empty())
		return;

	auto& targets = d->objSlots[std::move(sig)];
	for (auto& target : targets)
	{
		target.callback(this, target.receiver);
	}
}

void GMObject::removeSignal(GMSignal sig, GMObject& receiver)
{
	D(d);
	auto& targets = d->objSlots[std::move(sig)];
	removeIf(targets, [&](auto iter) {
		return iter->receiver == &receiver;
	});
}

void GMObject::releaseConnections()
{
	D(d);
	// 释放连接到此对象的信号
	if (!d->objSlots.empty())
	{
		for (auto& event : d->objSlots)
		{
			GMSignal name = event.first;
			for (auto& target : event.second)
			{
				target.receiver->removeConnection(this, name);
			}
		}
		GMClearSTLContainer(d->objSlots);
	}

	// 释放此对象连接的所有信号
	if (!d->connectionTargets.empty())
	{
		for (auto& conns : d->connectionTargets)
		{
			conns.host->removeSignal(conns.name, *this);
		}
	}
}

void GMObject::addConnection(GMObject* host, GMSignal sig)
{
	D(d);
	GMConnectionTarget c;
	c.host = host;
	c.name = std::move(sig);
	d->connectionTargets.push_back(std::move(c));
}

void GMObject::removeConnection(GMObject* host, GMSignal sig)
{
	D(d);
	removeIf(d->connectionTargets, [&](auto iter) {
		return iter->name == sig && iter->host == host;
	});
}

// GMBuffer
GMBuffer::~GMBuffer()
{
	if (needRelease)
	{
		GM_delete_array(buffer);
	}
}

GMBuffer::GMBuffer(const GMBuffer& rhs)
{
	*this = rhs;
}

GMBuffer::GMBuffer(GMBuffer&& rhs) GM_NOEXCEPT
{
	swap(rhs);
}

GMBuffer& GMBuffer::operator =(GMBuffer&& rhs) GM_NOEXCEPT
{
	swap(rhs);
	return *this;
}

GMBuffer& GMBuffer::operator =(const GMBuffer& rhs)
{
	this->needRelease = rhs.needRelease;
	if (rhs.needRelease)
	{
		this->size = rhs.size;
		buffer = new GMbyte[this->size];
		memcpy_s(buffer, size, rhs.buffer, this->size);
	}
	else
	{
		this->size = rhs.size;
		this->buffer = rhs.buffer;
	}
	return *this;
}

void GMBuffer::convertToStringBuffer()
{
	GMbyte* newBuffer = new GMbyte[size + 1];
	memcpy_s(newBuffer, size, buffer, size);
	newBuffer[size] = 0;
	size++;
	if (needRelease && buffer)
		GM_delete_array(buffer);
	needRelease = true;
	buffer = newBuffer;
}

void GMBuffer::convertToStringBufferW()
{
	GMwchar* newBuffer = new GMwchar[size + 1];
	memcpy_s(newBuffer, size, buffer, size);
	newBuffer[size] = 0;
	size += sizeof(GMwchar);
	if (needRelease && buffer)
		GM_delete_array(buffer);
	needRelease = true;
	buffer = reinterpret_cast<GMbyte*>(newBuffer);
}

void GMBuffer::swap(GMBuffer& rhs)
{
	GM_SWAP(buffer, rhs.buffer);
	GM_SWAP(size, rhs.size);
	GM_SWAP(needRelease, rhs.needRelease);
}

GMBufferView::GMBufferView(const GMBuffer& rhs, GMsize_t offset)
{
	buffer = rhs.buffer + offset;
	size = rhs.size - offset;
	needRelease = false;
}

GMBufferView::~GMBufferView()
{
	GM_ASSERT(!needRelease);
}