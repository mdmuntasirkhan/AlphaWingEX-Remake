#include "SceneSwitcher.h"

GameScene SceneSwitcher::pending    = GameScene::TITLE;
bool      SceneSwitcher::hasPending = false;

bool SceneSwitcher::hasVideoRequest = false;
bool SceneSwitcher::videoFullscreen = false;
int  SceneSwitcher::videoWidth      = 1280;
int  SceneSwitcher::videoHeight     = 720;
int  SceneSwitcher::videoVsync      = -1;
