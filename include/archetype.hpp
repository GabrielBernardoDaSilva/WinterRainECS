#pragma once
#include "component.hpp"
#include "errors.hpp"

#include <cstddef>
#include <vector>
#include <map>
#include <memory>
#include <expected>
#include <print>
#include <string>
#include <algorithm>

namespace winter_rain_ecs
{
    class Component;
    using EntityId = std::size_t;

    struct Entity
    {
        EntityId id;
        std::size_t location;
    };

    class Archetype
    {
    private:
        std::vector<EntityId> entities;
        std::map<ComponentId, std::unique_ptr<ComponentList>> components;
        std::string name{};

        template <class T>
        void add_component(T component)
        {
            const std::size_t hash = typeid(T).hash_code();
            if (components.contains(hash))
            {
                std::unique_ptr<BaseComponentWrapper> comp = std::make_unique<ComponentWrapper<T>>(component);
                components[hash]->add_component(std::move(comp));
                return;
            }
            // append name
            name.append(typeid(T).name());

            auto list = std::make_unique<ComponentList>();
            std::unique_ptr<BaseComponentWrapper> comp = std::make_unique<ComponentWrapper<T>>(component);
            list->add_component(std::move(comp));
            components.insert({hash, std::move(list)});
        }

        template <class... T>
        void add_components(T... component)
        {
            (add_component(component), ...);
        }

    public:
        Archetype() = default;

        template <typename... T>
        explicit Archetype(const Entity entity, T... components)
        {
            entities.push_back(entity.id);
            add_components<T...>(components...);
        }

        explicit Archetype(
            std::tuple<Entity, std::map<std::size_t, std::unique_ptr<BaseComponentWrapper>>> moved_entity);

        Archetype(const Archetype &archetype) = delete;

        Archetype(Archetype &&archetype) noexcept
        {
            entities = std::move(archetype.entities);
            components = std::move(archetype.components);
            name = std::move(archetype.name);
        }

        // move assignment operator
        Archetype &operator=(Archetype &&archetype) noexcept
        {
            if (this != &archetype)
            {
                entities = std::move(archetype.entities);
                components = std::move(archetype.components);
                name = std::move(archetype.name);
            }
            return *this;
        }

        ~Archetype() = default;

        template <class U>
        bool has_component()
        {
            const std::size_t hash = typeid(U).hash_code();
            return components.contains(hash);
        }

        template <class... U>
        bool has_components()
        {
            return (has_component<U> && ...);
        }

        template <typename... T>
        void add_entity(const Entity entity, T... components)
        {
            entities.push_back(entity.id);
            add_components<T...>(components...);
        }

        std::expected<Entity, ArchetypeError> remove_entity(Entity entity);

        template <class U>
        std::expected<std::tuple<Entity, std::map<std::size_t, std::unique_ptr<BaseComponentWrapper>>>,
                      ArchetypeError>
        remove_component(Entity entity)
        {
            const std::size_t hash = typeid(U).hash_code();

            const auto entity_it = std::ranges::find(entities, entity.id);
            if (this->has_component<U>() && entity_it != entities.end())
            {
                // find component list
                const auto it = components.find(hash);
                // find entity in archetype
                std::println("Removing component with hash: {}", it->first);
                const auto index = std::distance(entities.begin(), entity_it);
                it->second->remove_component(index);
                // now we need to prepare other components to migrate this entity to other archetype
                std::map<std::size_t, std::unique_ptr<BaseComponentWrapper>> component_to_migrate;
                for (auto &[key, value] : components)
                {
                    if (key != hash)
                    {
                        component_to_migrate.insert({key, value->remove_component(index)});
                    }
                }
                // remove entity from this archetype
                entities.erase(entity_it);
                // make a tuple with the components to migrate and entity
                std::tuple<Entity, std::map<std::size_t, std::unique_ptr<BaseComponentWrapper>>> tuple =
                    std::make_tuple(
                        entity, std::move(component_to_migrate));

                return tuple;
            }
            return std::unexpected(ArchetypeError::ComponentNotFound);
        }

        [[nodiscard]] bool has_component_by_hash(std::size_t hash) const;

        [[nodiscard]] bool has_components_by_hash(const std::vector<std::size_t> &hashes) const;

        void get_archetype_hash(std::vector<std::size_t> &hashes);

        [[nodiscard]] bool is_empty() const;

        std::expected<std::tuple<Entity, std::map<std::size_t, std::unique_ptr<BaseComponentWrapper>>>,
                      ArchetypeError>
        move_entity(Entity entity);

        void migrate_entity_to_itself(
            std::tuple<Entity, std::map<std::size_t, std::unique_ptr<BaseComponentWrapper>>> tuple);

        void list_all_components_hash() const;

        std::map<std::size_t, std::unique_ptr<ComponentList>> &get_components();

        [[nodiscard]] const std::vector<EntityId> &get_entities() const;

        std::string get_name();

        void set_name(const char *name);
    };
}
