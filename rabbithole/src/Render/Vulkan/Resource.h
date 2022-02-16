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