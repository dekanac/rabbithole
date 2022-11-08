#pragma once

class Renderer;
class RabbitPass;

#include <list>
#include <unordered_map>

class RabbitPassManager
{

public:
	void SchedulePasses(Renderer& renderer);
	void DeclareResources();
	void ExecutePasses(Renderer& renderer);
	void ExecuteOneTimePasses(Renderer& renderer);
	void Destroy();

public:
	void AddPass(RabbitPass* pass, bool executeOnce = false);

private:
	std::unordered_map<const char*, RabbitPass*> m_RabbitPasses;
	std::list<RabbitPass*> m_RabbitPassesToExecute;
	std::list<RabbitPass*> m_RabbitPassesOneTimeExecute;
};