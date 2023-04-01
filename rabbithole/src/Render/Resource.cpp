#include "Resource.h"

#include <memory>

uint32_t AllocatedResource::ms_CurrentId = 0;

AllocatedResource::AllocatedResource()
{
	ms_CurrentId++;
	m_Id = ms_CurrentId;
}


