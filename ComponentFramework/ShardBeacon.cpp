#include "ShardBeacon.h"
#include <glew.h>
#include <MMath.h>
#include <cmath>

ShardBeacon::ShardBeacon()
    : dishMesh(nullptr)
    , computerMesh(nullptr)
    , solarMesh(nullptr)
    , pos{}
    , count(0)
    , active(false)
    , pulseTimer(0.0f)
    , spinAngle(0.0f)
{}

ShardBeacon::~ShardBeacon() {}

bool ShardBeacon::OnCreate() {
    dishMesh = new Mesh("meshes/Temp_AlphaWingEX_Satalite_Beacon_DishModule.obj");
    if (!dishMesh->OnCreate()) return false;

    computerMesh = new Mesh("meshes/Temp_AlphaWingEX_Satalite_Beacon_ComputerModule.obj");
    if (!computerMesh->OnCreate()) return false;

    solarMesh = new Mesh("meshes/Temp_AlphaWingEX_Satalite_Beacon_SolarPowerModule.obj");
    if (!solarMesh->OnCreate()) return false;

    return true;
}

void ShardBeacon::OnDestroy() {
    if (dishMesh)     { dishMesh->OnDestroy();     delete dishMesh;     dishMesh     = nullptr; }
    if (computerMesh) { computerMesh->OnDestroy(); delete computerMesh; computerMesh = nullptr; }
    if (solarMesh)    { solarMesh->OnDestroy();    delete solarMesh;    solarMesh    = nullptr; }
}

void ShardBeacon::Place(const Vec3& position, int shardCount) {
    pos        = Vec3(position.x, position.y, -10.0f);
    count      = shardCount;
    active     = true;
    pulseTimer = 0.0f;
    spinAngle  = 0.0f;
}

void ShardBeacon::Clear() {
    active = false;
    count  = 0;
}

void ShardBeacon::Update(float deltaTime) {
    if (!active) return;
    pulseTimer += deltaTime;
    spinAngle  += kSpinSpeed * deltaTime;
    if (spinAngle >= 360.0f) spinAngle -= 360.0f;
}

int ShardBeacon::TryCollect(const Vec3& playerPos) {
    if (!active) return 0;
    float dx = playerPos.x - pos.x;
    float dy = playerPos.y - pos.y;
    if (dx * dx + dy * dy < kPickupRadius * kPickupRadius) {
        active = false;
        return count;
    }
    return 0;
}

void ShardBeacon::Render(Shader* shader,
                         const Matrix4& projection,
                         const Matrix4& view) const {
    if (!active) return;

    glUniformMatrix4fv(shader->GetUniformID("projectionMatrix"), 1, GL_FALSE, projection);
    glUniformMatrix4fv(shader->GetUniformID("viewMatrix"),       1, GL_FALSE, view);

    // Slow Y-axis tumble gives the satellite-in-orbit read
    Matrix4 model = MMath::translate(pos)
                  * MMath::rotate(spinAngle, Vec3(0.0f, 1.0f, 0.0f))
                  * MMath::scale(kBeaconScale, kBeaconScale, kBeaconScale);

    glUniformMatrix4fv(shader->GetUniformID("modelMatrix"), 1, GL_FALSE, model);
    glUniform1f(shader->GetUniformID("emissive"), 0.0f);

    // Computer module — dark hull panel
    glUniform4f(shader->GetUniformID("color"), 0.20f, 0.22f, 0.28f, 1.0f);
    computerMesh->Render();

    // Dish module — brushed silver
    glUniform4f(shader->GetUniformID("color"), 0.75f, 0.78f, 0.86f, 1.0f);
    dishMesh->Render();

    // Solar panels — deep gold, Phong-lit
    glUniform4f(shader->GetUniformID("color"), 0.82f, 0.68f, 0.08f, 1.0f);
    solarMesh->Render();

    // Beacon glow — additive emissive pulse on the dish so it reads as a signal light
    float pulse = 0.55f + 0.45f * sinf(pulseTimer * 4.5f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glDepthMask(GL_FALSE);
    glUniform1f(shader->GetUniformID("emissive"), 1.0f);
    glUniform4f(shader->GetUniformID("color"), 1.0f, 0.88f, 0.25f, pulse * 0.65f);
    dishMesh->Render();
    glUniform1f(shader->GetUniformID("emissive"), 0.0f);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}
