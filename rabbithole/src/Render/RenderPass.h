#pragma once

class Renderer;
class RenderPass
{
public:
	virtual void DeclareResources(Renderer* renderer) = 0;
	virtual void Setup(Renderer* renderer) = 0;
	virtual void Render(Renderer* renderer) = 0;
};

#define DECLARE_RENDERPASS(name) \
class name : public RenderPass \
{ \
public: \
	virtual void DeclareResources(Renderer* renderer); \
	virtual void Setup(Renderer* renderer); \
	virtual void Render(Renderer* renderer); \
};

DECLARE_RENDERPASS(GBufferPass)
DECLARE_RENDERPASS(LightingPass)
DECLARE_RENDERPASS(CopyToSwapchainPass)