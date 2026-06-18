#include "SceneSwitcher.h"

GameScene SceneSwitcher::pending    = GameScene::TITLE;
bool      SceneSwitcher::hasPending = false;
