//
// MIT License
// Copyright (c) 2025 Alejandro Massó Martínez, Miguel Curros García, Alejandro González Sánchez
//

#include "Object.h"

#include <io/LuaManager.h>
#include <sol/table.hpp>
#include <Utils/Vector2.h>

#include "events/Event.h"
#include "common/Project.h"

#define localVarsKey "localVariables"
#define xKey "x"
#define yKey "y"
#define layerKey "layer"
#define spriteKey "sprite"
#define collidableKey "collidable"
#define eventsKey "events"

editor::resources::Object::Object(Project* project) :
    _project(project),
    _x(0),
    _y(0),
    _layer(0),
    _collides(false),
    _spriteName("") {
}

editor::resources::Object::Object(Project* project, int x, int y) :
    _project(project),
    _x(x),
    _y(y),
    _layer(0),
    _collides(false),
    _spriteName("") {
}

editor::resources::Object::~Object() {
    for (auto& event : _events) {
        delete event;
        event = nullptr;
    }
    _events.clear();
    _localVariables.clear();
}

bool editor::resources::Object::read(sol::table const& objectTable) {
    sol::optional<int> x = objectTable.get<sol::optional<int>>(xKey);
    if (!x.has_value())
        return false;
    _x = x.value();

    sol::optional<int> y = objectTable.get<sol::optional<int>>(yKey);
    if (!y.has_value())
        return false;
    _y = y.value();

    sol::optional<int> layer = objectTable.get<sol::optional<int>>(layerKey);
    if (!layer.has_value())
        return false;
    _layer = layer.value();

    sol::optional<std::string> sprite = objectTable.get<sol::optional<std::string>>(spriteKey);
    if (!y.has_value())
        return false;
    _spriteName = sprite.value();

    sol::optional<bool> collidable = objectTable.get<sol::optional<bool>>(collidableKey);
    if (!collidable.has_value())
        return false;
    _collides = collidable.value();

    sol::optional<sol::table> events = objectTable.get<sol::optional<sol::table>>(eventsKey);
    if (!events.has_value())
        return false;
    if (!readEvents(*events))
        return false;

    sol::optional<sol::table> localVars = objectTable.get<sol::optional<sol::table>>(localVarsKey);
    if (!localVars.has_value())
        return false;
    if (!readLocalVars(*localVars))
        return false;
    return true;
}

bool editor::resources::Object::write(sol::table& objectTable) {
    objectTable[xKey] = _x;
    objectTable[yKey] = _y;
    objectTable[collidableKey] = _collides;
    objectTable[layerKey] = _layer;
    objectTable[spriteKey] = _spriteName;

    sol::table events = io::LuaManager::GetInstance().getState().create_table();
    if (!writeEvents(events))
        return false;
    objectTable[eventsKey] = events;

    sol::table localVars = io::LuaManager::GetInstance().getState().create_table();
    writeLocalVars(localVars);
    objectTable[localVarsKey] = localVars;

    return true;
}

bool editor::resources::Object::writeToEngine(std::ostream& object, events::EventBuildDependencies& dependencies, std::string const& handler, Vector2 const& center) {
    std::ostringstream events;
    events << "events = {\n";
    for (auto& event : _events) {
        event->writeToEngine(events, dependencies, this);
    }
    events << "}\n";

    object << "handler = \"" << handler << "\",\n";
    object << "children = {\n";
    writeChildrenToEngine(object, dependencies);
    object << "},\n";
    object << "components = {\n";
    writeComponentsToEngine(object, dependencies, events.str(), center);
    object << "}\n";

    return true;
}

const std::string & editor::resources::Object::getSprite() const {
    return _spriteName;
}

void editor::resources::Object::setSprite(const std::string &sprite) {
    _spriteName = sprite;
}

int editor::resources::Object::getX() const {
    return _x;
}

int editor::resources::Object::getY() const {
    return _y;
}

int editor::resources::Object::getLayer() const {
    return _layer;
}

void editor::resources::Object::setX(int x) {
    _x = x;
}

void editor::resources::Object::setY(int y) {
    _y = y;
}

void editor::resources::Object::setLayer(int layer) {
    _layer = layer;
}

bool editor::resources::Object::getCollide() const {
    return _collides;
}

void editor::resources::Object::setCollide(bool collide) {
    _collides = collide;
}

void editor::resources::Object::addVariable(const std::string &key) {
    auto& lua = io::LuaManager::GetInstance().getState();
    _localVariables.insert({key,make_object(lua,"")});
}

void editor::resources::Object::setVariable(const std::string &key, const std::string &value) {
    if (_localVariables.contains(key)) {
        auto& lua = io::LuaManager::GetInstance().getState();
        _localVariables[key] = make_object(lua,value);
    }
}

void editor::resources::Object::removeVariable(const std::string &key) {
    _localVariables.erase(key);
}

const std::unordered_map<std::string, sol::object> & editor::resources::Object::getVariables() const {
    return _localVariables;
}

void editor::resources::Object::addEvent(events::Event *event) {
    _events.push_back(event);
}

std::vector<editor::resources::events::Event *>::iterator editor::resources::Object::removeEvent(
    std::vector<events::Event *>::iterator iter) {
    return _events.erase(iter);
}

std::vector<editor::resources::events::Event *> & editor::resources::Object::getEvents() {
    return _events;
}

bool editor::resources::Object::readEvents(sol::table const& events) {
    for (auto&& [key, event] : events) {
        if (!event.is<std::string>())
            return false;
        auto ev = _project->getEvent(event.as<std::string>());
        if (ev == nullptr)
            return false;
        _events.push_back(ev);
    }
    return true;
}

bool editor::resources::Object::readLocalVars(sol::table const& localVars) {
    for (auto&& [key, value] : localVars) {
        if (!key.is<std::string>() || !value.is<sol::object>())
            return false;
        _localVariables.insert({key.as<std::string>(), value.as<sol::object>()});
    }
    return true;
}

bool editor::resources::Object::writeEvents(sol::table& events) {
    for (auto& event : _events) {
        events.add(event->getName());
    }
    return true;
}

void editor::resources::Object::writeLocalVars(sol::table& localVars) {
    for (auto& [key, variable] : _localVariables) {
        localVars[key] = variable;
    }
}

bool editor::resources::Object::writeChildrenToEngine(std::ostream& children, events::EventBuildDependencies& dependencies) {
    for (auto& child : dependencies.childrenDependencies) {
        children << "{\n"; {
            if (!child.handler.empty())
                children << "handler = \"" << child.handler << "\",\n";

            if (child.components.empty()) {
                children << "},\n";
                continue;
            }

            children << "components = {\n";
            for (auto& component : child.components) {
                children << component.first << " = ";
                if (component.second.empty())
                    children << "{ 0 }";
                else
                    children << io::LuaManager::GetInstance().serializeToString(component.second);
                children << ",\n";
            }
            children << "}\n";

        } children << "},\n";
    }
    return true;
}

bool editor::resources::Object::writeComponentsToEngine(std::ostream& components, events::EventBuildDependencies& dependencies, std::string const& events, Vector2 const& center) {
    components << "EventHandler = {\n";
    components << events;
    components << "},\n";
    if  (const auto& animator = dependencies.componentDependencies.find("Animator");
        animator != dependencies.componentDependencies.end()) {

        components << "Animator = {\n";
        if (!_spriteName.empty()) {
            components << "sprite = \"data/sprites/" << _spriteName << ".lua\",\n";
        }
        components << "layer = " << _layer << "\n";
        components << "},\n";
        dependencies.componentDependencies.erase(animator);
    }
    else if (!_spriteName.empty()) {
        components << "SpriteRenderer = {\n";
        components << "sprite = \"data/sprites/" << _spriteName << ".lua\",\n";
        components << "layer = " << _layer << "\n";
        components << "},\n";
    }
    if (!dependencies.componentDependencies.contains("MovementComponent") && _collides) {
        components << "MovementObstacle = {0},\n";
    }
    for (auto& component : dependencies.componentDependencies) {
        components << component.first << " = ";
        if (!component.second.valid() || component.second.empty())
            components << "{ 0 }";
        else
            components << io::LuaManager::GetInstance().serializeToString(component.second);
        components << ",\n";
    }
    int const* dims = _project->getDimensions();
    components << "Transform = {\n";
    components << "position = {" << (_x - center.getX()) * dims[0] << ", " << (_y - center.getY()) * dims[1] << "}\n";
    components << "},\n";
    components << "LocalVariables = {\n";
    for (auto& [key, value] : _localVariables) {
        components << key << " = \"" << value.as<std::string>() << "\",\n";
    }
    components << "},\n";

    return true;
}

