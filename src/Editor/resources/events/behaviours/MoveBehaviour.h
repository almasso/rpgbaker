//
// MIT License
// Copyright (c) 2025 Alejandro Massó Martínez, Miguel Curros García, Alejandro González Sánchez
//

#ifndef MOVEBEHAVIOUR_H
#define MOVEBEHAVIOUR_H

#include "../EventBehaviour.h"

namespace editor::resources::events {

    class EventBehaviourClass(MoveBehaviour) {
    public:
        MoveBehaviour(Event* event);
        ~MoveBehaviour() override;
        bool read(sol::table const& params) override;
        bool writeParamsToEngine(std::ostream& behaviour, EventBuildDependencies& dependencies, Object const* container) override;
        bool render() override;
    protected:
        bool writeParams(sol::table& params) override;
    private:
        int _xTarget, _yTarget;
    };

}

#endif //MOVEBEHAVIOUR_H
