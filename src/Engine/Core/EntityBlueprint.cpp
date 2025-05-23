#include "EntityBlueprint.h"

#include <Load/LuaReader.h>
#include <sol/sol.hpp>

#include "ComponentData.h"

void EntityBlueprint::readChildren(sol::table const& childrenBlueprint) {
    for (auto const& [key,child] : childrenBlueprint) {
        if (child.is<sol::table>())
            _children.emplace_back(child.as<sol::table>());
    }
}

void EntityBlueprint::readComponents(sol::table const& componentBlueprints) {
    for (auto const& [id,component] : componentBlueprints) {
        if (!id.is<std::string>() || !component.is<sol::table>())
            continue;
        _components.push_back(ComponentData(id.as<std::string>(), component.as<sol::table>()));
    }
}

void EntityBlueprint::read(sol::table const& blueprint) {
    sol::table childrenBlueprints = LuaReader::GetTable(blueprint, "children");
    if (childrenBlueprints.valid())
        readChildren(childrenBlueprints);

    sol::table componentBlueprints = LuaReader::GetTable(blueprint,"components");
    if (componentBlueprints.valid())
        readComponents(componentBlueprints);

    _handler = blueprint.get_or<std::string>("handler", "");

    _active = blueprint.get_or<bool, std::string, bool>("active", true);
}

void EntityBlueprint::clear() {
    _children.clear();
    _components.clear();
    _active = false;
    _handler = "";
}

EntityBlueprint::EntityBlueprint() = default;

EntityBlueprint::~EntityBlueprint() {
    clear();
}

const std::vector<EntityBlueprint> & EntityBlueprint::getChildren() const{
    return _children;
}

const std::vector<ComponentData> & EntityBlueprint::getComponents() const{
    return _components;
}

bool EntityBlueprint::getActive() const{
    return _active;
}

const std::string & EntityBlueprint::getHandler() const{
    return _handler;
}

EntityBlueprint::EntityBlueprint(sol::table const& blueprint) {
    read(blueprint);
}
