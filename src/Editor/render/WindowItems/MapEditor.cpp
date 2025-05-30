//
// MIT License
// Copyright (c) 2025 Alejandro Massó Martínez, Miguel Curros García, Alejandro González Sánchez
//

#include "MapEditor.h"
#include "io/LocalizationManager.h"
#include "common/Project.h"
#include "render/RenderManager.h"
#include <imgui_internal.h>
#include <resources/Object.h>
#include <resources/events/Event.h>

#include "resources/Tile.h"
#include "render/Modals/MainWindow/TilesetWizard.h"
#include "render/WindowStack.h"
#include "resources/Map.h"
#include "resources/Tileset.h"
#include "render/Modals/MainWindow/MapWizard.h"
#include <SDL3/SDL_filesystem.h>


#ifdef __APPLE__
#define GetCurrentDir strdup(SDL_GetBasePath())
#else
#define GetCurrentDir SDL_GetCurrentDirectory()
#endif

editor::render::tabs::MapEditor::MapEditor(editor::Project* project) :
WindowItem(io::LocalizationManager::GetInstance().getString("window.mainwindow.mapeditor") + ""), _project(project) {

    _currentDirectory = GetCurrentDir;
    _uiTextures.push_back(RenderManager::GetInstance().loadTexture(std::string(_currentDirectory) + "/settings/assets/map/toplayer.png"));
    _buttonTooltips.push_back(io::LocalizationManager::GetInstance().getString("window.mainwindow.mapeditor.selectedlayer"));
    _uiTextures.push_back(RenderManager::GetInstance().loadTexture(std::string(_currentDirectory) + "/settings/assets/map/currentandbelowtp.png"));
    _buttonTooltips.push_back(io::LocalizationManager::GetInstance().getString("window.mainwindow.mapeditor.selectedandbelowtransparent"));
    _uiTextures.push_back(RenderManager::GetInstance().loadTexture(std::string(_currentDirectory) + "/settings/assets/map/currentandbelow.png"));
    _buttonTooltips.push_back(io::LocalizationManager::GetInstance().getString("window.mainwindow.mapeditor.selectedandbelowopaque"));
    _uiTextures.push_back(RenderManager::GetInstance().loadTexture(std::string(_currentDirectory) + "/settings/assets/map/alllayers.png"));
    _buttonTooltips.push_back(io::LocalizationManager::GetInstance().getString("window.mainwindow.mapeditor.alllayers"));
    _uiTextures.push_back(RenderManager::GetInstance().loadTexture(std::string(_currentDirectory) + "/settings/assets/map/zoomin.png"));
    _buttonTooltips.push_back(io::LocalizationManager::GetInstance().getString("window.mainwindow.mapeditor.zoomin"));
    _uiTextures.push_back(RenderManager::GetInstance().loadTexture(std::string(_currentDirectory) + "/settings/assets/map/zoomout.png"));
    _buttonTooltips.push_back(io::LocalizationManager::GetInstance().getString("window.mainwindow.mapeditor.zoomout"));
    _uiTextures.push_back(RenderManager::GetInstance().loadTexture(std::string(_currentDirectory) + "/settings/assets/map/grid.png"));
    _buttonTooltips.push_back(io::LocalizationManager::GetInstance().getString("window.mainwindow.mapeditor.grid"));
    _uiTextures.push_back(RenderManager::GetInstance().loadTexture(std::string(_currentDirectory) + "/settings/assets/map/collisions.png"));
    _buttonTooltips.push_back(io::LocalizationManager::GetInstance().getString("window.mainwindow.mapeditor.collisions"));
    _uiTextures.push_back(RenderManager::GetInstance().loadTexture(std::string(_currentDirectory) + "/settings/assets/map/object.png"));
    _buttonTooltips.push_back(io::LocalizationManager::GetInstance().getString("window.mainwindow.mapeditor.objectMode"));

    _tilesetWizard = new render::modals::TilesetWizard(project);
    WindowStack::addWindowToStack(_tilesetWizard);
    _mapWizard = new render::modals::MapWizard(project);
    WindowStack::addWindowToStack(_mapWizard);
}

editor::render::tabs::MapEditor::~MapEditor() {
    SDL_free(_currentDirectory);
    for(int i = 0; i < _uiTextures.size(); ++i) {
        RenderManager::GetInstance().destroyTexture(_uiTextures[i]);
    }
    _uiTextures.clear();
    WindowStack::removeWindowFromStack(_tilesetWizard);
    WindowStack::removeWindowFromStack(_mapWizard);
    delete _tilesetWizard;
    _tilesetWizard = nullptr;
    delete _mapWizard;
    _mapWizard = nullptr;
}


void editor::render::tabs::MapEditor::beforeRender() {
    _itemFlags = _somethingModified ? ImGuiTabItemFlags_UnsavedDocument : 0;
}

void editor::render::tabs::MapEditor::onRender() {
    drawTileSelector();
    ImGui::SameLine();
    ImGui::BeginChild("Toolbar+Grid", ImVec2(render::RenderManager::GetInstance().getWidth() / 2, 0));
    drawToolbar();
    drawGrid();
    ImGui::EndChild();
    ImGui::SameLine();
    drawObjectInspector();
}

void editor::render::tabs::MapEditor::drawGrid() {
    const int* dimensions = _project->getDimensions();
    float scaledSize = dimensions[0] * _zoom;

    int mapWidth = 32, mapHeight = 32;
    if(_selectedMap != nullptr) {
        mapWidth = _selectedMap->getMapWidth();
        mapHeight = _selectedMap->getMapHeight();
    }

    ImVec2 cursorStart = ImGui::GetCursorScreenPos();
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    ImGui::SetNextWindowContentSize(ImVec2(mapWidth * scaledSize, mapHeight * scaledSize));
    ImGui::BeginChild("##mapGrid", ImVec2(RenderManager::GetInstance().getWidth()/2,0), 0, ImGuiWindowFlags_HorizontalScrollbar);
    {
        float scrollX = ImGui::GetScrollX();
        float scrollY = ImGui::GetScrollY();
        float mouseX = ImGui::GetMousePos().x;
        float mouseY = ImGui::GetMousePos().y;

        bool isDragging = ImGui::IsMouseDown(ImGuiMouseButton_Left);

        for (int j = 0; j < mapHeight; ++j) {
            for (int i = 0; i < mapWidth; ++i) {
                ImVec2 tilePos = {
                        cursorStart.x + i * scaledSize - scrollX,
                        cursorStart.y + j * scaledSize - scrollY
                };
                ImVec2 tileEnd = {tilePos.x + scaledSize, tilePos.y + scaledSize};

                if(tilePos.y < 100) continue;
                if(tileEnd.x > 3 * RenderManager::GetInstance().getWidth()/4) continue;

                std::string buttonID = "tile_" + std::to_string(i) + "_" + std::to_string(j);
                ImGui::SetCursorScreenPos(tilePos);
                if (ImGui::InvisibleButton(buttonID.c_str(), ImVec2(scaledSize, scaledSize))) {
                    if (_selectedMap != nullptr && _objectMode) {
                        _selectedObject = i + mapWidth * j;
                    }
                    else if (_selectedMap != nullptr && _collisionsShown) {
                        _selectedMap->getCollisions()[i + mapWidth * j] = !_selectedMap->getCollisions()[i + mapWidth * j];
                        _somethingModified = true;
                    }
                    else {
                        if(_project->totalTilesets() > 0 && _selectedTileset != nullptr && _selectedMap != nullptr && _selectedTile != -1) {
                            _selectedMap->getTiles()[_selectedLayer][i + mapWidth * j] = _selectedTileset->getTiles()[_selectedTile];
                            _selectedMap->getCollisions()[i + mapWidth * j] = _selectedMap->getCollisions()[i + mapWidth * j] || _selectedTileset->getCollisions()[_selectedTile];
                            _somethingModified = true;
                        }
                    }
                }

                bool hovered = mouseX >= tilePos.x && mouseX < tileEnd.x && mouseY >= tilePos.y && mouseY < tileEnd.y;

                if(hovered && ImGui::IsMouseDown(ImGuiMouseButton_Right) && !(_collisionsShown || _objectMode)) {
                    if(_project->totalTilesets() > 0 && _selectedMap != nullptr) {
                        _selectedMap->getTiles()[_selectedLayer][i + mapWidth * j] = nullptr;
                        _somethingModified = true;
                    }
                }

                if(isDragging && hovered && !(_collisionsShown || _objectMode)) {
                    if(_project->totalTilesets() > 0 && _selectedTileset != nullptr && _selectedMap != nullptr && _selectedTile != -1) {
                        _selectedMap->getTiles()[_selectedLayer][i + mapWidth * j] = _selectedTileset->getTiles()[_selectedTile];
                        _selectedMap->getCollisions()[i + mapWidth * j] = _selectedMap->getCollisions()[i + mapWidth * j] || _selectedTileset->getCollisions()[_selectedTile];
                        _somethingModified = true;
                    }
                }

                _selectedGridMode = _objectMode ? GridDrawingMode::DRAW_OBJECTS : (_collisionsShown ? GridDrawingMode::DRAW_COLLISIONS : _tmpTileMode);
                if(_selectedMap != nullptr) drawTileInGrid(_selectedMap, mapWidth, i, j, tilePos, tileEnd, drawList);
                if (_isGridShown) drawList->AddRect(tilePos, tileEnd, IM_COL32(255, 255, 255, 50));
            }
        }

        if(_somethingModified && ImGui::IsKeyDown(ImGuiMod_Ctrl) && ImGui::IsKeyPressed(ImGuiKey_S, false)) {
            save();
        }
    }
    ImGui::EndChild();
}

void editor::render::tabs::MapEditor::drawTileInGrid(resources::Map* currentMap, int mapWidth, int i, int j, ImVec2 tilePos, ImVec2 tileEnd, ImDrawList* drawList) {
    switch(_selectedGridMode) {
        case GridDrawingMode::DRAW_SELECTED_LAYER_ONLY: {
            resources::Tile* tile = currentMap->getTiles()[_selectedLayer][i + mapWidth * j];
            if(tile != nullptr)
                drawList->AddImage(tile->texture, tilePos, tileEnd, tile->rect.Min, tile->rect.Max);
        } break;
        case GridDrawingMode::DRAW_SELECTED_LAYER_BELOW_TRANSPARENT: {
            for(int x = 0; x < _selectedLayer; ++x) {
                resources::Tile* tile = currentMap->getTiles()[x][i + mapWidth * j];
                if(tile != nullptr)
                    drawList->AddImage(tile->texture, tilePos, tileEnd, tile->rect.Min, tile->rect.Max);
            }
            drawList->AddRectFilled(tilePos, tileEnd, IM_COL32(0, 0, 0, 100));
            resources::Tile* tile = currentMap->getTiles()[_selectedLayer][i + mapWidth * j];
            if(tile != nullptr)
                drawList->AddImage(tile->texture, tilePos, tileEnd, tile->rect.Min, tile->rect.Max);
        } break;
        case GridDrawingMode::DRAW_SELECTED_LAYER_BELOW_OPAQUE: {
            for(int x = 0; x < _selectedLayer; ++x) {
                resources::Tile* tile = currentMap->getTiles()[x][i + mapWidth * j];
                if(tile != nullptr)
                    drawList->AddImage(tile->texture, tilePos, tileEnd, tile->rect.Min, tile->rect.Max);
            }
            resources::Tile* tile = currentMap->getTiles()[_selectedLayer][i + mapWidth * j];
            if(tile != nullptr)
                drawList->AddImage(tile->texture, tilePos, tileEnd, tile->rect.Min, tile->rect.Max);
        } break;
        case GridDrawingMode::DRAW_ALL: {
            for(int x = 0; x <= currentMap->getLayers() - 1; ++x) {
                resources::Tile* tile = currentMap->getTiles()[x][i + mapWidth * j];
                if(tile != nullptr)
                    drawList->AddImage(tile->texture, tilePos, tileEnd, tile->rect.Min, tile->rect.Max);
            }
        } break;
        case GridDrawingMode::DRAW_COLLISIONS: {
            if (_selectedMap != nullptr) {
                for(int x = 0; x <= currentMap->getLayers() - 1; ++x) {
                    resources::Tile* tile = currentMap->getTiles()[x][i + mapWidth * j];
                    if(tile != nullptr)
                        drawList->AddImage(tile->texture, tilePos, tileEnd, tile->rect.Min, tile->rect.Max);
                }
                drawList->AddRectFilled(tilePos, tileEnd, IM_COL32(0, 0, 0, 100));
                _selectedMap->getCollisions()[i + mapWidth * j] ? drawX(tilePos, tileEnd, drawList) : drawList->AddCircle(ImVec2((tilePos.x + tileEnd.x) / 2, (tilePos.y + tileEnd.y) / 2),(tileEnd.x - tilePos.x) / 4,IM_COL32(0, 0, 255, 255), 0, 2);
            }
        } break;
        case GridDrawingMode::DRAW_OBJECTS: {
            if (_selectedMap != nullptr) {
                for(int x = 0; x <= currentMap->getLayers() - 1; ++x) {
                    resources::Tile* tile = currentMap->getTiles()[x][i + mapWidth * j];
                    if(tile != nullptr)
                        drawList->AddImage(tile->texture, tilePos, tileEnd, tile->rect.Min, tile->rect.Max);
                }
                drawList->AddRectFilled(tilePos, tileEnd, IM_COL32(0, 0, 0, 100));
                if (_selectedObject == i + mapWidth * j) {
                    drawList->AddRect(tilePos, tileEnd, IM_COL32(90, 255, 47, 255));
                }
                else if (_selectedMap->getObject(i + mapWidth * j) != nullptr) {
                    drawList->AddRect(tilePos, tileEnd, IM_COL32(255, 255, 51, 255));
                }
            }
        } break;
    }
}

void editor::render::tabs::MapEditor::drawX(const ImVec2& tilePos, const ImVec2& tileEnd, ImDrawList* drawList) {
    drawList->AddLine(ImVec2(tilePos.x * 1.015f, tilePos.y * 1.015f), ImVec2(tileEnd.x * 0.985f, tileEnd.y * 0.985f), IM_COL32(255, 0, 0, 255), 3);
    drawList->AddLine(ImVec2(tileEnd.x * 0.985f, tilePos.y * 1.015f), ImVec2(tilePos.x * 1.015f, tileEnd.y * 0.985f), IM_COL32(255, 0, 0, 255), 3);
}

void editor::render::tabs::MapEditor::drawToolbar() {
    ImGui::BeginChild("##mapEditorToolbar", ImVec2(render::RenderManager::GetInstance().getWidth() / 2, 60), true, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6,6));

    // Botones de layers
    for(int i = 0; i < 4; ++i) {
        if(i > 0) ImGui::SameLine();
        ImVec4 bg = _tmpTileMode == i ? ImVec4(1,0,1,1) : ImVec4(0,0,0,0);

        if(ImGui::ImageButton(std::string("but" + std::to_string(i)).c_str(), _uiTextures[i], ImVec2(32, 32), ImVec2(0,0), ImVec2(1,1), bg)) {
            _tmpTileMode = i;
        }
        if(ImGui::IsItemHovered()) {
            ImGui::SetTooltip(_buttonTooltips[i].c_str());
        }
    }

    ImGui::SameLine();
    ImGui::Spacing();
    ImGui::SameLine();
    ImGui::Spacing();
    ImGui::SameLine();
    if(ImGui::ImageButton("butZoomin", _uiTextures[4], ImVec2(32, 32))) {
        _zoom *= 1.1f;
        _zoom = ImClamp(_zoom, 0.5f, 3.0f);
    }
    if(ImGui::IsItemHovered()) {
        ImGui::SetTooltip(_buttonTooltips[4].c_str());
    }
    ImGui::SameLine();
    if(ImGui::ImageButton("butZoomout", _uiTextures[5], ImVec2(32, 32))) {
        _zoom *= 1.0f/1.1f;
        _zoom = ImClamp(_zoom, 0.5f, 3.0f);
    }
    if(ImGui::IsItemHovered()) {
        ImGui::SetTooltip(_buttonTooltips[5].c_str());
    }
    ImGui::SameLine();
    ImGui::Spacing();
    ImGui::SameLine();
    ImGui::Spacing();
    ImGui::SameLine();
    ImVec4 bg = _isGridShown ? ImVec4(1,0,1,1) : ImVec4(0,0,0,0);
    if(ImGui::ImageButton("butGrid", _uiTextures[6], ImVec2(32, 32), ImVec2(0,0), ImVec2(1,1), bg)) {
        _isGridShown = !_isGridShown;
    }
    if(ImGui::IsItemHovered()) {
        ImGui::SetTooltip(_buttonTooltips[6].c_str());
    }

    ImGui::SameLine();
    bg = _collisionsShown ? ImVec4(1, 0, 1, 1) : ImVec4(0, 0, 0, 0);
    if (ImGui::ImageButton("butCollisions", _uiTextures[7], ImVec2(32, 32), ImVec2(0,0), ImVec2(1,1), bg)) {
        _collisionsShown = !_collisionsShown;
        if (_collisionsShown) _objectMode = false;
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip(_buttonTooltips[7].c_str());
    }
    ImGui::SameLine();
    bg = _objectMode ? ImVec4(1, 0, 1, 1) : ImVec4(0, 0, 0, 0);
    if (ImGui::ImageButton("butObjects", _uiTextures[8], ImVec2(32, 32), ImVec2(0,0), ImVec2(1,1), bg)) {
        _objectMode = !_objectMode;
        if (_objectMode) _collisionsShown = false;
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip(_buttonTooltips[8].c_str());
    }


    ImGui::SameLine();
    ImGui::Spacing();
    ImGui::SameLine();
    ImGui::Spacing();
    ImGui::SameLine();
    ImGui::BeginChild("##dropdowns");
    {
        ImGui::SetNextItemWidth(250);
        resources::Map* mapToDelete = nullptr;
        std::string mapToDeleteName;
        if (ImGui::BeginCombo("##mapDropdown", _selectedMap != nullptr ? _selectedMap->getName().c_str() : io::LocalizationManager::GetInstance().getString("window.mainwindow.mapeditor.mapselector").c_str())) {
            for(auto it = _project->getMaps().begin(); it != _project->getMaps().end();) {
                auto map = *it;
                bool isSelected = (map.second == _selectedMap);
                if(ImGui::Selectable(map.second->getName().c_str(), isSelected)) {
                    _selectedMap = map.second;
                    _selectedLayer = 0;
                    if(ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled) && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
                        ImGui::OpenPopup((map.second->getName() + "mapoptions").c_str());
                    }
                }
                if(ImGui::BeginPopupContextItem((map.second->getName() + "mapoptions").c_str())) {
                    if(ImGui::MenuItem(io::LocalizationManager::GetInstance().getString("action.edit").c_str())) {
                        _mapWizard->setMapToModify(map.second, true);
                        _mapWizard->show();
                        _selectedLayer = 0;
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
                    if (ImGui::MenuItem(io::LocalizationManager::GetInstance().getString("action.delete").c_str())) {
                        mapToDelete = map.second;
                        mapToDeleteName = mapToDelete->getName();
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::PopStyleColor();
                    ImGui::EndPopup();
                }
                ++it;
            }
            if(mapToDelete) {
                _project->removeMap(mapToDeleteName);
                if(_selectedMap == mapToDelete) {
                    _selectedMap = nullptr;
                    _selectedLayer = -1;
                }
                delete mapToDelete;
                mapToDelete = nullptr;
                _somethingModified = true;
            }
            ImGui::Separator();
            if (ImGui::Selectable(io::LocalizationManager::GetInstance().getString("window.mainwindow.mapeditor.createmap").c_str())) {
                _createdMap = new editor::resources::Map(_project);
                _mapWizard->setMapToModify(_createdMap);
                _mapWizard->show();
            }
            ImGui::EndCombo();
        }
        ImGui::SameLine();
        ImGui::SetNextItemWidth(250);
        if (ImGui::BeginCombo("##layerDropdown", _selectedLayer >= 0 ? (io::LocalizationManager::GetInstance().getString("window.mainwindow.mapeditor.layer") + " " + std::to_string(_selectedLayer)).c_str() : io::LocalizationManager::GetInstance().getString("window.mainwindow.mapeditor.layer").c_str())) {
            if(_selectedMap != nullptr) {
                for(int i = _selectedMap->getLayers() - 1; i >= 0; --i) {
                    bool isSelected = (i == _selectedLayer);
                    if (ImGui::Selectable((io::LocalizationManager::GetInstance().getString("window.mainwindow.mapeditor.layer") + " " + std::to_string(i)).c_str(), isSelected)) {
                        _selectedLayer = i;
                    }
                }
            }
            ImGui::Separator();
            ImGui::BeginDisabled(_selectedMap == nullptr);
            if (ImGui::Selectable(io::LocalizationManager::GetInstance().getString("window.mainwindow.mapeditor.createlayer").c_str())) {
                _selectedMap->addLayer();
                _somethingModified = true;
            }
            ImGui::EndDisabled();
            ImGui::EndCombo();
        }
    }
    ImGui::EndChild();

    ImGui::PopStyleVar();
    ImGui::EndChild();

    if(_mapOpened && !_mapWizard->isOpen()) {
        if(_createdMap != nullptr) {
            if(_createdMap->isInitialized()) {
                _project->addMap(_createdMap);
                _createdMap = nullptr;
                _somethingModified = true;
            }
            else {
                delete _createdMap;
                _createdMap = nullptr;
            }
            _mapOpened = false;
        }
        else {
            _mapOpened = false;
            _project->refreshMaps();
            _somethingModified = true;
        }
    }

    if(_mapWizard->hasBeenCalled()) _mapOpened = true;
}

void editor::render::tabs::MapEditor::drawTileSelector() {
    ImGui::BeginChild("##tileSelector", ImVec2(RenderManager::GetInstance().getWidth()/4, 0), true);
    {
        resources::Tileset* tilesetToDelete = nullptr;
        std::string tilesetToDeleteName;
        if (ImGui::BeginCombo("##tilesetDropdown", _selectedTileset != nullptr ? _selectedTileset->getName().c_str() : io::LocalizationManager::GetInstance().getString("window.mainwindow.mapeditor.tilesetselector").c_str())) {
            for(auto it = _project->getTilesets().begin(); it != _project->getTilesets().end();) {
                auto tileset = *it;
                bool isSelected = (_selectedTileset == tileset.second);
                if(ImGui::Selectable(tileset.second->getName().c_str(), isSelected)) {
                    _selectedTileset = tileset.second;
                    _selectedTile = -1;
                    if(ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled) && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
                        ImGui::OpenPopup((tileset.second->getName() + "tilesetoptions").c_str());
                    }
                }
                if(ImGui::BeginPopupContextItem((tileset.second->getName() + "tilesetoptions").c_str())) {
                    if(ImGui::MenuItem(io::LocalizationManager::GetInstance().getString("action.edit").c_str())) {
                        _tilesetWizard->setTilesetToModify(tileset.second, true);
                        _tilesetWizard->show();
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
                    if (ImGui::MenuItem(io::LocalizationManager::GetInstance().getString("action.delete").c_str())) {
                        tilesetToDelete = tileset.second;
                        tilesetToDeleteName = tilesetToDelete->getName();
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::PopStyleColor();
                    ImGui::EndPopup();
                }

                ++it;
            }

            if(tilesetToDelete) {
                _project->removeTileset(tilesetToDeleteName);
                if(_selectedTileset == tilesetToDelete) {
                    _selectedTileset = nullptr;
                    _selectedTile = -1;
                }
                delete tilesetToDelete;
                tilesetToDelete = nullptr;
                _somethingModified = true;
            }
            ImGui::Separator();
            if (ImGui::Selectable(io::LocalizationManager::GetInstance().getString("window.mainwindow.mapeditor.createtileset").c_str())) {
                _createdTileset = new editor::resources::Tileset(_project);
                _tilesetWizard->setTilesetToModify(_createdTileset);
                _tilesetWizard->show();
            }
            ImGui::EndCombo();
        }

        ImGui::Separator();

        ImGui::BeginChild("##tilesetGrid", ImVec2(0, 0), true, ImGuiWindowFlags_AlwaysVerticalScrollbar | ImGuiWindowFlags_HorizontalScrollbar);
        if(_selectedTileset != nullptr) {
            int i = 0;
            for(auto tile : _selectedTileset->getTiles()) {
                if(i % _selectedTileset->getXTiles() != 0) ImGui::SameLine();
                bool isSelected = (!_collisionsShown && !_objectMode && i == _selectedTile);
                if (isSelected) {
                    ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(100, 200, 255, 255));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(130, 230, 255, 255));
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(80, 170, 230, 255));
                }
                if(ImGui::ImageButton(("tile" + std::to_string(i)).c_str(), tile->texture, ImVec2(32, 32), tile->rect.Min, tile->rect.Max)) {
                    if(!_collisionsShown) _selectedTile = i;
                    else {
                        _selectedTileset->getCollisions()[i] = !_selectedTileset->getCollisions()[i];
                        _somethingModified = true;
                    }
                }
                if (isSelected) {
                    ImGui::PopStyleColor(3);
                }
                if(_collisionsShown) {
                    ImVec2 min = ImGui::GetItemRectMin();
                    ImVec2 max = ImGui::GetItemRectMax();
                    ImDrawList* drawList = ImGui::GetWindowDrawList();
                    _selectedTileset->getCollisions()[i] ? drawX(min, max, drawList) : drawList->AddCircle(ImVec2((min.x + max.x) / 2, (min.y + max.y) / 2), (max.x - min.x) / 4, IM_COL32(0, 0, 255, 255), 0, 2);
                }
                ++i;
            }
        }
        ImGui::EndChild();
    }
    ImGui::EndChild();

    if(_tilesetOpened && !_tilesetWizard->isOpen()) {
        if (_createdTileset != nullptr) {
            if(_createdTileset->isInitialized()) {
                _project->addTileset(_createdTileset);
                _createdTileset = nullptr;
                _somethingModified = true;
            }
            else {
                delete _createdTileset;
                _createdTileset = nullptr;
            }
            _tilesetOpened = false;
        }
        else {
            _tilesetOpened = false;
            _project->refreshTilesets();
            _somethingModified = true;
        }
    }

    if(_tilesetWizard->hasBeenCalled()) _tilesetOpened = true;
}

void editor::render::tabs::MapEditor::save() {
    _somethingModified = false;
    _project->saveEverything();
}

void editor::render::tabs::MapEditor::drawObjectInspector() {
    ImGui::BeginChild("##objectInspector", ImVec2(0, 0), true);
    if (_selectedMap && _objectMode) {
        editor::resources::Object* selectedObject = _selectedMap->getObject(_selectedObject);
        if (selectedObject != nullptr) {
            ImGui::Text("%s", ("object_" + _selectedMap->getName() + "_" + std::to_string(_selectedObject)).c_str());
            ImGui::SameLine();
            bool deleteObject = ImGui::Button(io::LocalizationManager::GetInstance().getString("window.mainwindow.mapeditor.deleteObject").c_str());
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            if (ImGui::BeginCombo("##spriteDropdown", selectedObject->getSprite() != "" ? selectedObject->getSprite().c_str() :
                io::LocalizationManager::GetInstance().getString("window.mainwindow.mapeditor.spriteSelector").c_str())) {
                for(auto it = _project->getSprites().begin(); it != _project->getSprites().end(); ++it) {
                    auto sprite = *it;
                    bool isSelected = (selectedObject->getSprite() == sprite.first);
                    if(ImGui::Selectable(sprite.first.c_str(), isSelected)) {
                        selectedObject->setSprite(sprite.first);
                        _somethingModified = true;
                    }
                }
                if(ImGui::Selectable(io::LocalizationManager::GetInstance().getString("window.mainwindow.mapeditor.spriteSelectorNone").c_str(),
                    selectedObject->getSprite() == "")) {
                    selectedObject->setSprite("");;
                    _somethingModified = true;
                }
                ImGui::EndCombo();
            }
            ImGui::Spacing();
            ImGui::Spacing();
            ImVec2 spriteSize(128, 128);
            float windowWidth = ImGui::GetContentRegionAvail().x;
            float offsetX = (windowWidth - spriteSize.x) * 0.5f;
            if (offsetX < 0) offsetX = 0;
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offsetX);
            std::string spriteName = selectedObject->getSprite();
            if (!spriteName.empty() && _project->getSprite(spriteName)) {
                resources::Sprite* sprite = _project->getSprite(spriteName);
                ImTextureID texId = sprite->getTextureID();
                ImVec2 p = ImGui::GetCursorScreenPos();
                ImGui::GetWindowDrawList()->AddRectFilled(p, ImVec2(p.x + spriteSize.x, p.y + spriteSize.y), IM_COL32(150, 150, 150, 180));
                ImGui::Image(texId, spriteSize, sprite->getSpriteCoordsMin(), sprite->getSpriteCoordsMax());
            } else {
                ImVec2 p = ImGui::GetCursorScreenPos();
                ImGui::GetWindowDrawList()->AddRectFilled(p, ImVec2(p.x + spriteSize.x, p.y + spriteSize.y), IM_COL32(150, 150, 150, 180));
                ImGui::Dummy(spriteSize);
            }
            ImGui::Spacing();
            ImGui::Spacing();
            int layer = selectedObject->getLayer();
            if (ImGui::InputInt(io::LocalizationManager::GetInstance().getString("window.mainwindow.mapeditor.spriteLayer").c_str(), &layer)) {
                selectedObject->setLayer(layer);
                _somethingModified = true;
            };
            int pos[2] = { selectedObject->getX(),selectedObject->getY()};
            ImGui::Text("%s", io::LocalizationManager::GetInstance().getString("window.mainwindow.mapeditor.objectPosition").c_str());
            if (ImGui::InputInt2("##objectPosition", pos)) {
                if (selectedObject->getX() != pos[0] || selectedObject->getY() != pos[1]) {
                    _selectedMap->removeObject(_selectedObject);
                    selectedObject->setX(pos[0]);
                    selectedObject->setY(pos[1]);
                    _selectedObject = selectedObject->getX() + _selectedMap->getMapWidth() * selectedObject->getY();
                    _selectedMap->addObject(_selectedObject, selectedObject);
                    _somethingModified = true;
                }
            }
            bool collides = selectedObject->getCollide();
            if (ImGui::Checkbox(io::LocalizationManager::GetInstance().getString("window.mainwindow.mapeditor.objectCollides").c_str(), &collides)) {
                selectedObject->setCollide(collides);
                _somethingModified = true;
            }
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            ImGui::Text("%s", io::LocalizationManager::GetInstance().getString("window.mainwindow.mapeditor.objectVariables").c_str());
            for (auto [name, value] : selectedObject->getVariables()) {
                char variableValue[256];
                std::strncpy(variableValue, value.as<std::string>().c_str(), sizeof(variableValue) - 1);
                if (ImGui::InputText(name.c_str(),
                    variableValue, IM_ARRAYSIZE(variableValue), ImGuiInputTextFlags_EnterReturnsTrue)) {
                    selectedObject->setVariable(name, variableValue);
                    _somethingModified = true;
                }
            }
            char newVariableName[256] = {0};
            if (ImGui::InputText(io::LocalizationManager::GetInstance().getString("window.mainwindow.mapeditor.addVariable").c_str(),
                newVariableName, IM_ARRAYSIZE(newVariableName), ImGuiInputTextFlags_EnterReturnsTrue)) {
                selectedObject->addVariable(newVariableName);
                _somethingModified = true;
            }
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            ImGui::Text("%s", io::LocalizationManager::GetInstance().getString("window.mainwindow.mapeditor.events").c_str());
            for (auto event = selectedObject->getEvents().begin(); event != selectedObject->getEvents().end();) {
                ImGui::Text((*event)->getName().c_str());
                ImGui::SameLine();
                if (ImGui::Button(io::LocalizationManager::GetInstance().getString("window.mainwindow.mapeditor.removeEvent").c_str())) {
                    event = selectedObject->removeEvent(event);
                    _somethingModified = true;
                }
                else ++event;
                ImGui::Spacing();
            }
            if (ImGui::BeginCombo("##addEventDropdown",io::LocalizationManager::GetInstance().getString("window.mainwindow.mapeditor.addEvent").c_str())) {
                for (auto [name, event] : _project->getEvents()) {
                    if(ImGui::Selectable((name).c_str(), false)) {
                        selectedObject->addEvent(event);
                        _somethingModified = true;
                    }
                }
                ImGui::EndCombo();
            }
            if (deleteObject) {
                _selectedMap->removeObject(selectedObject->getX() + _selectedMap->getMapWidth() * selectedObject->getY());
                _somethingModified = true;
                delete selectedObject;
            }
        }
        else if (_selectedObject >= 0) {
            const char* buttonLabel = io::LocalizationManager::GetInstance().getString("window.mainwindow.mapeditor.createObject").c_str();
            ImVec2 buttonSize = ImVec2(256, 64);
            ImVec2  avail = ImGui::GetContentRegionAvail();
            ImVec2 cursorPos;
            cursorPos.x = (avail.x - buttonSize.x) * 0.5f;
            cursorPos.y = (avail.y - buttonSize.y) * 0.5f;
            if (cursorPos.x < 0) cursorPos.x = 0;
            if (cursorPos.y < 0) cursorPos.y = 0;
            ImGui::SetCursorPos(cursorPos);

            if (ImGui::Button(buttonLabel, buttonSize)) {
                int y = _selectedObject / _selectedMap->getMapWidth();
                int x = _selectedObject % _selectedMap->getMapWidth();
                _selectedMap->addObject(_selectedObject, new resources::Object(_project, x, y));
                _somethingModified = true;
            }
        }
    }
    ImGui::EndChild();
}