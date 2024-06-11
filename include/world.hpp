#pragma once

#include "archetype.hpp"
#include "system.hpp"
#include "custom_concepts.hpp"
#include "errors.hpp"
#include "system.hpp"
#include "event.hpp"
#include "plugin.hpp"
#include "scheduler.hpp"
#include "generator.hpp"
#include "accessor.hpp"
#include "entity_manager.hpp"

#include <vector>
#include <memory>
#include <ranges>
#include <expected>
#include <functional>

namespace darkriver
{
    class World
    {
    private:
        std::vector<std::unique_ptr<Plugin>> m_plugins{};
        Accessor m_accessor{*this};

        ExecutorManager m_executor_manager{};
        TaskManager m_task_manager{};
        EntityManager m_entity_manager;
        EventManager m_event_manager{m_accessor};

    public:
        World() = default;

        ~World() = default;

        /// archetypes
#pragma region archetypes

        EntityManager &get_entity_manager();

        template <typename... T>
        Entity add_entity(T... components)
        {
            auto entity = m_entity_manager.add_entity(components...);
            return entity;
        };

        template <typename T>
        std::expected<Success, ArchetypeError> add_component_to_entity(Entity entity, T component)
        {
            return m_entity_manager.add_component_to_entity<T>(entity, component);
        }

        template <typename T>
        std::expected<Success, ArchetypeError> remove_component_from_entity(Entity entity)
        {
            return m_entity_manager.remove_component_from_entity<T>(entity);
        }

        std::expected<Success, ArchetypeError> remove_entity(Entity entity);

/// archetypes end
#pragma endregion
#pragma region executors
        /// executors

        template <req_system_args... Args>
        void add_executor(ExecutorType executor_type, std::function<void(Args...)> system)
        {
            m_executor_manager.add_executor(executor_type, system, m_accessor);
        }

        template <req_system_args... Args>
        void add_executor(ExecutorType executor_type, void (*system)(Args...))
        {
            m_executor_manager.add_executor(executor_type, system, m_accessor);
        }

        template <function_pointer Lambda>
        void add_executor(ExecutorType executor_type, Lambda &&lambda)
        {
            std::function func{lambda};
            m_executor_manager.add_executor(executor_type, func, m_accessor);
        }

        template <function_pointer... Executors>
        void add_executors(ExecutorType execute_type, Executors &&...executors)
        {
            (add_executor(execute_type, executors), ...);
        }

        template <typename WithComponents, typename WithoutComponents>
        Query<WithComponents, WithoutComponents> query()
        {
            return Query<WithComponents, WithoutComponents>(m_accessor);
        }

        template <typename WithComponents>
        Query<WithComponents> query()
        {
            return Query<WithComponents>(m_accessor);
        }

        ExecutorManager &get_executor_manager();

        void run() noexcept;

        ///
#pragma endregion
#pragma region events

        /// events

        template <typename Ev>
        void emit(Ev event)
        {
            m_event_manager.emit(event);
        }

        template <typename Ev>
        void subscribe(std::function<void(World &, Ev)> subscriber)
        {
            m_event_manager.subscribe<Ev>(subscriber);
        }

        template <typename Ev>
        void subscribe(void (*subscriber)(World &, Ev))
        {
            m_event_manager.subscribe<Ev>(subscriber);
        }

        template <typename Ev>
        void unsubscribe(std::function<void(Ev)> subscriber)
        {
            m_event_manager.unsubscribe<Ev>(subscriber);
        }

        template <typename Ev>
        void unsubscribe(void (*subscriber)(Ev))
        {
            m_event_manager.unsubscribe<Ev>(subscriber);
        }

        EventManager &get_event_manager();
#pragma endregion
#pragma region plugins
        // plugins
        template <req_plugin T>
        void add_plugin()
        {
            m_plugins.push_back(std::make_unique<T>());
        }

        void build_plugins();
#pragma endregion
        //
#pragma region tasks
        // tasks
        template <typename... Args>
        TaskId add_task(std::function<generator<WaitAmountOfSeconds>(Args...)> task, Args... args)
        {
            return m_task_manager.add_task(TaskScheduler(task, args...));
        }

        template <typename... Args>
        TaskId add_task(generator<WaitAmountOfSeconds> (*task)(Args...), Args... args)
        {
            return m_task_manager.add_task(TaskScheduler(task, args...));
        }

        void run_tasks(float dt);

        std::expected<Success, SchedulerError> resume_task(TaskId id);

        std::expected<Success, SchedulerError> stop_task(TaskId id);

        std::expected<Success, SchedulerError> remove_task(TaskId id);

        void stop_all_tasks();

        void resume_all_tasks();

        TaskManager &get_task_manager();
#pragma endregion

        std::vector<Archetype>& create_archetype_ref();

        void show_archetypes() const;

        void show_entities() const;
    };
}
