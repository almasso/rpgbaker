//
// MIT License
// Copyright (c) 2025 Alejandro Massó Martínez, Miguel Curros García, Alejandro González Sánchez
//

#ifndef RPGBAKER_WELCOMEWINDOWPROJECTMANAGEMENTBUTTONS_H
#define RPGBAKER_WELCOMEWINDOWPROJECTMANAGEMENTBUTTONS_H

#include "render/Subwindow.h"
#include <unordered_map>

namespace editor {
    class Project;
}

namespace editor::render::modals {
    class DeleteProjectModal;
    class RenameProjectModal;
    class CreateProjectModal;
}

namespace editor::render::subwindows {
    class WelcomeWindowProjectManagementButtons : public Subwindow {
    public:
        WelcomeWindowProjectManagementButtons(std::unordered_map<Project*, editor::render::modals::DeleteProjectModal*>* deleteProjects,
                                              std::unordered_map<Project*, editor::render::modals::RenameProjectModal*>* renameProjects,
                                              editor::render::modals::CreateProjectModal* createProject);
    private:
        std::unordered_map<Project*, editor::render::modals::DeleteProjectModal*>* _deleteProjects = nullptr;
        std::unordered_map<Project*, editor::render::modals::RenameProjectModal*>* _renameProjects = nullptr;
        editor::render::modals::CreateProjectModal* _createProject = nullptr;

        bool _showCreateProject = false;

        void beforeRender() override;

        void onRender() override;

        std::string searchProject();
    };
}


#endif //RPGBAKER_WELCOMEWINDOWPROJECTMANAGEMENTBUTTONS_H
