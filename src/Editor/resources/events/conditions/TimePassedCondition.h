//
// MIT License
// Copyright (c) 2025 Alejandro Massó Martínez, Miguel Curros García, Alejandro González Sánchez
//

#ifndef TIMEPASSEDCONDITION_H
#define TIMEPASSEDCONDITION_H

#include "../EventCondition.h"

namespace editor::resources::events {

    class EventConditionClass(TimePassedCondition, "TimePassed") {
    public:
        TimePassedCondition(Event* event);
        ~TimePassedCondition() override;
        bool read(sol::table const& params) override;
        bool render() override;
    protected:
        bool writeParamsToEngine(std::ostream& condition, EventBuildDependencies& dependencies, Object const* container) override;
        bool writeParams(sol::table& params) override;
    private:
        float _seconds;
    };

}

#endif //TIMEPASSEDCONDITION_H
