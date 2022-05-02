#pragma once

class AllocatedResource
{
public:
	AllocatedResource();
	virtual uint32_t GetID() { return m_Id; }

	static uint32_t ms_CurrentId;
protected:
	uint32_t m_Id;
	void UpdateID();
};

class ManagableResource
{
public:

	virtual ResourceState			GetResourceState() const = 0;
	virtual void					SetResourceState(ResourceState state) = 0;

	virtual ResourceState			GetShouldBeResourceState() const = 0;
	virtual void					SetShouldBeResourceState(ResourceState state) = 0;

};