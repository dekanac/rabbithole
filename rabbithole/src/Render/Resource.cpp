#include "Resource.h"
#include <memory>

AllocatedResource::AllocatedResource()
{
	UpdateID();
}

uint32_t AllocatedResource::ms_CurrentId = 0;

void AllocatedResource::UpdateID()
{
	ms_CurrentId++;
	m_Id = ms_CurrentId;
}
