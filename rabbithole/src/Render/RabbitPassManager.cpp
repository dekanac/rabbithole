#include "RabbitPassManager.h"

#include "Render/RabbitPass.h"
#include "Render/RabbitPasses/AmbientOcclusion.h"
#include "Render/RabbitPasses/GBuffer.h"
#include "Render/RabbitPasses/Lighting.h"
#include "Render/RabbitPasses/Postprocessing.h"
#include "Render/RabbitPasses/Shadows.h"
#include "Render/RabbitPasses/Tools.h"
#include "Render/RabbitPasses/Upscaling.h"
#include "Render/RabbitPasses/Volumetric.h"

//pass scheduler
void RabbitPassManager::SchedulePasses(Renderer& renderer)
{
	AddPass(new Create3DNoiseTexturePass(renderer), true);
	AddPass(new GBufferPass(renderer));
	AddPass(new SkyboxPass(renderer));
	AddPass(new CopyDepthPass(renderer));
	AddPass(new RTShadowsPass(renderer));
	AddPass(new ShadowDenoisePrePass(renderer));
	AddPass(new ShadowDenoiseTileClassificationPass(renderer));
	AddPass(new ShadowDenoiseFilterPass(renderer));
	AddPass(new SSAOPass(renderer));
	AddPass(new SSAOBlurPass(renderer));
	AddPass(new VolumetricPass(renderer));
	AddPass(new ComputeScatteringPass(renderer));
	AddPass(new LightingPass(renderer));
	AddPass(new ApplyVolumetricFogPass(renderer));
	AddPass(new TextureDebugPass(renderer));
	AddPass(new FSR2Pass(renderer));
	//AddPass(new BloomCompute(renderer));
	AddPass(new TonemappingPass(renderer));
	AddPass(new CopyToSwapchainPass(renderer));
}

void RabbitPassManager::DeclareResources()
{
	for (auto pass : m_RabbitPassesToExecute)
	{
		pass->DeclareResources();
	}
}

void RabbitPassManager::ExecutePasses(Renderer& renderer)
{
	for (auto pass : m_RabbitPassesToExecute)
	{
		renderer.BeginLabel(pass->GetName());

		pass->Setup();

		pass->Render();

		renderer.RecordGPUTimeStamp(pass->GetName());

		renderer.EndLabel();
	}
}

void RabbitPassManager::ExecuteOneTimePasses(Renderer& renderer)
{
	for (auto pass : m_RabbitPassesOneTimeExecute)
	{
		renderer.BeginLabel(pass->GetName());

		pass->Setup();

		pass->Render();

		renderer.RecordGPUTimeStamp(pass->GetName());

		renderer.EndLabel();
	}
}

void RabbitPassManager::Destroy()
{

}

void RabbitPassManager::AddPass(RabbitPass* pass, bool executeOnce /*= false*/)
{
	m_RabbitPasses[pass->GetName()] = pass;
	executeOnce ? m_RabbitPassesOneTimeExecute.push_back(pass) : m_RabbitPassesToExecute.push_back(pass);
}
