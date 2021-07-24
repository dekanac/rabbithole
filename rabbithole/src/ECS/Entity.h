#pragma once
#include "ECS/Component.h" //pitaj krasica
#include "Logger/Logger.h"

#include <vector>
#include <memory>

class Entity
{
public:
    unsigned int GetId() const { return m_Id; }

protected:
    inline static unsigned int m_CurrentId = 0;
    unsigned int m_Id;
    std::vector<std::unique_ptr<Component>> m_Components;

public:
    Entity() { m_Id = m_CurrentId++; }

    Entity(const Entity& other) {


    }

    template <typename TComponent>
    TComponent* GetComponent() const
    {
        for (const auto& component : m_Components)
        {
            if (component->m_TypeId == Component::GetComponentTypeID<TComponent>())
            {
                return static_cast<TComponent*>(component.get());
            }
        }

        return nullptr;
    }

    template <typename TComponent>
    bool HasComponent() const
    {
        for (auto& component : m_Components)
        {
            if (component->m_TypeId == Component::GetComponentTypeID<TComponent>())
            {
                return true;
            }
        }

        return false;
    }

    template <typename... TComponent>
    bool HasComponents() const
    {
        if ( (HasComponent<TComponent>() && ...) )
        {
            return true;
        }

        return false;
    }

    template <typename TComponent, typename... TArgs>
    TComponent& AddComponent(TArgs&&... _mArgs)
    {
        if (HasComponent<TComponent>())
        {
            ASSERT(false, "Attempting to add a component twice! Entity ID: {}, ComponentType: {}", m_Id, Component::GetComponentTypeID<TComponent>());
        }

        auto component = std::make_unique<TComponent>(std::forward<TArgs>(_mArgs)...);

        component->m_TypeId = Component::GetComponentTypeID<TComponent>();

        m_Components.push_back(std::move(component));

        return *(static_cast<TComponent*>(m_Components.back().get()));
    }

    template <typename TComponent>
    void RemoveComponent()
    {
        if (!HasComponent<TComponent>())
        {
            LOG_WARNING("Attempting to remove a component that does not exist! Entity ID : {}, ComponentType : {}", m_Id, Component::GetComponentTypeID<TComponent>());
        }

        m_Components.erase(std::remove_if(begin(m_Components),
                                            end(m_Components),
                                            [](const Component* component) { return component->GetType() == Component::GetComponentTypeID<TComponent>(); }));
    }
};

