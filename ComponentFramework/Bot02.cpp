#include "Bot02.h"
#include <MMath.h>
#include <glew.h>
#include <cstdlib>
#include <cmath>
#include <iostream>

Bot02::Bot02() :
    bodyMesh    { nullptr },
    cockpitMesh { nullptr },
    finMesh     { nullptr },
    thrustMesh  { nullptr },
    fragmentMesh{ nullptr },
    bulletMesh  { nullptr },
    thrustTimer { 0.0f },
    hoverTimer  { 0.0f }
{
}

Bot02::~Bot02() {}

bool Bot02::OnCreate(const char* bodyFile, const char* cockpitFile,
                     const char* finFile,  const char* thrustFile,
                     const char* fragmentFile, const char* bulletFile) {
    auto load = [](Mesh*& m, const char* path, const char* label) -> bool {
        m = new Mesh(path);
        if (!m->OnCreate()) { std::cout << label << " not found!\n"; return false; }
        return true;
    };
    return load(bodyMesh,     bodyFile,     "Bot02 body")
        && load(cockpitMesh,  cockpitFile,  "Bot02 cockpit")
        && load(finMesh,      finFile,      "Bot02 fin")
        && load(thrustMesh,   thrustFile,   "Bot02 thrust")
        && load(fragmentMesh, fragmentFile, "Bot02 fragment")
        && load(bulletMesh,   bulletFile,   "Bot02 bullet");
}

void Bot02::OnDestroy() {
    auto destroy = [](Mesh*& m) {
        if (m) { m->OnDestroy(); delete m; m = nullptr; }
    };
    destroy(bodyMesh);
    destroy(cockpitMesh);
    destroy(finMesh);
    destroy(thrustMesh);
    destroy(fragmentMesh);
    destroy(bulletMesh);

    positions.clear();
    targetPositions.clear();
    hoverPhases.clear();
    hp.clear();
    hitTimers.clear();
    fireTimers.clear();
    bulletPositions.clear();
    bulletVelocities.clear();
    debris.clear();
}

void Bot02::Spawn(float /*playerY*/) {
    // Top — fixed right-top hover point
    positions      .push_back(Vec3(15.0f,  kHoverYOffset, -10.0f));
    targetPositions.push_back(Vec3(kHoverX, kHoverYOffset, -10.0f));
    hoverPhases    .push_back(0.0f);
    hp             .push_back(kHP);
    hitTimers      .push_back(0.0f);
    fireTimers     .push_back(0.0f);

    // Bottom — fixed right-bottom hover point, fires half-cycle later
    positions      .push_back(Vec3(15.0f, -kHoverYOffset, -10.0f));
    targetPositions.push_back(Vec3(kHoverX, -kHoverYOffset, -10.0f));
    hoverPhases    .push_back(3.14159f);
    hp             .push_back(kHP);
    hitTimers      .push_back(0.0f);
    fireTimers     .push_back(kFireInterval * 0.5f);  // staggered fire

    hoverTimer = 0.0f;
}

void Bot02::Update(float deltaTime, float playerX, float playerY) {
    thrustTimer += deltaTime;
    hoverTimer  += deltaTime;

    for (int i = 0; i < (int)positions.size(); i++) {
        // Approach target X
        float dx = targetPositions[i].x - positions[i].x;
        if (fabsf(dx) > 0.05f)
            positions[i].x += dx * kApproachSpeed * deltaTime;

        // Hover oscillation in Y
        float targetY = targetPositions[i].y
                      + sinf(hoverTimer * kHoverFrequency + hoverPhases[i]) * kHoverAmplitude;
        positions[i].y += (targetY - positions[i].y) * 5.0f * deltaTime;

        // Hit flash
        if (hitTimers[i] > 0.0f) {
            hitTimers[i] -= deltaTime;
            if (hitTimers[i] < 0.0f) hitTimers[i] = 0.0f;
        }

        // Fire once settled near hover position (within 2 units of target X)
        if (fabsf(positions[i].x - targetPositions[i].x) < 2.0f) {
            fireTimers[i] += deltaTime;
            if (fireTimers[i] >= kFireInterval) {
                fireTimers[i] = 0.0f;
                // Aim directly at player position
                float vx = playerX - positions[i].x;
                float vy = playerY - positions[i].y;
                float len = sqrtf(vx * vx + vy * vy);
                if (len > 0.001f) {
                    vx /= len; vy /= len;
                }
                bulletPositions .push_back(positions[i]);
                bulletVelocities.push_back(Vec3(vx * kBulletSpeed, vy * kBulletSpeed, 0.0f));
            }
        }
    }

    // Move bullets, cull off-screen
    for (int i = (int)bulletPositions.size() - 1; i >= 0; i--) {
        bulletPositions[i] += bulletVelocities[i] * deltaTime;
        if (bulletPositions[i].x < -16.0f || bulletPositions[i].x > 16.0f ||
            bulletPositions[i].y < -10.0f || bulletPositions[i].y >  10.0f) {
            bulletPositions .erase(bulletPositions.begin()  + i);
            bulletVelocities.erase(bulletVelocities.begin() + i);
        }
    }

    // Debris
    for (int i = (int)debris.size() - 1; i >= 0; i--) {
        debris[i].pos      += debris[i].vel * deltaTime;
        debris[i].angle    += debris[i].spinSpeed * deltaTime;
        debris[i].lifetime -= deltaTime;
        if (debris[i].lifetime <= 0.0f)
            debris.erase(debris.begin() + i);
    }
}

void Bot02::Render(Shader* shader,
    const Matrix4& projectionMatrix, const Matrix4& viewMatrix) const {
    glUniformMatrix4fv(shader->GetUniformID("projectionMatrix"), 1, GL_FALSE, projectionMatrix);
    glUniformMatrix4fv(shader->GetUniformID("viewMatrix"), 1, GL_FALSE, viewMatrix);

    glUniform1f(shader->GetUniformID("emissive"), 0.0f);

    for (int i = 0; i < (int)positions.size(); i++) {
        Matrix4 m = MMath::translate(positions[i]) *
                    MMath::rotate(180.0f, Vec3(0.0f, 1.0f, 0.0f)) *
                    MMath::scale(0.17f, 0.17f, 0.17f);
        glUniformMatrix4fv(shader->GetUniformID("modelMatrix"), 1, GL_FALSE, m);

        // Body — deep violet, flashes white on hit
        if (hitTimers[i] > 0.0f)
            glUniform4f(shader->GetUniformID("color"), 1.0f, 1.0f, 1.0f, 1.0f);
        else
            glUniform4f(shader->GetUniformID("color"), 0.45f, 0.05f, 0.9f, 1.0f);
        bodyMesh->Render();

        // Cockpit — vivid gold, contrasts the violet hull
        if (hitTimers[i] <= 0.0f)
            glUniform4f(shader->GetUniformID("color"), 1.0f, 0.72f, 0.0f, 1.0f);
        cockpitMesh->Render();

        // Fin — crimson red, contrasts the violet hull
        if (hitTimers[i] <= 0.0f)
            glUniform4f(shader->GetUniformID("color"), 0.85f, 0.05f, 0.1f, 1.0f);
        finMesh->Render();
    }

    // Thrust — additive magenta pulse (hot pink, distinct from Bot01's blue)
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glDepthMask(GL_FALSE);
    glUniform1f(shader->GetUniformID("emissive"), 1.0f);
    float pulse = 0.65f + 0.25f * sinf(thrustTimer * 18.0f)
                        + 0.10f * sinf(thrustTimer * 31.0f);
    glUniform4f(shader->GetUniformID("color"), 1.0f, 0.0f, 0.8f * pulse, pulse);
    for (int i = 0; i < (int)positions.size(); i++) {
        Matrix4 m = MMath::translate(positions[i]) *
                    MMath::rotate(180.0f, Vec3(0.0f, 1.0f, 0.0f)) *
                    MMath::scale(0.17f, 0.17f, 0.17f);
        glUniformMatrix4fv(shader->GetUniformID("modelMatrix"), 1, GL_FALSE, m);
        thrustMesh->Render();
    }

    // Bullets — bright gold orbs (matches cockpit accent)
    glUniform4f(shader->GetUniformID("color"), 1.0f, 0.82f, 0.0f, 1.0f);
    for (int i = 0; i < (int)bulletPositions.size(); i++) {
        Matrix4 m = MMath::translate(bulletPositions[i]) *
                    MMath::scale(kBulletScale, kBulletScale, kBulletScale);
        glUniformMatrix4fv(shader->GetUniformID("modelMatrix"), 1, GL_FALSE, m);
        bulletMesh->Render();
    }

    glUniform1f(shader->GetUniformID("emissive"), 0.0f);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);

    // Debris
    if (!debris.empty()) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        glDepthMask(GL_FALSE);
        glUniform1f(shader->GetUniformID("emissive"), 1.0f);
        for (int i = 0; i < (int)debris.size(); i++) {
            float alpha = debris[i].lifetime / debris[i].maxLifetime;
            glUniform4f(shader->GetUniformID("color"),
                debris[i].color.x, debris[i].color.y, debris[i].color.z, alpha);
            float ps = debris[i].pieceScale;
            Matrix4 m = MMath::translate(debris[i].pos) *
                        MMath::rotate(debris[i].angle, Vec3(0.0f, 0.0f, 1.0f)) *
                        MMath::scale(ps, ps, ps);
            glUniformMatrix4fv(shader->GetUniformID("modelMatrix"), 1, GL_FALSE, m);
            fragmentMesh->Render();
        }
        glUniform1f(shader->GetUniformID("emissive"), 0.0f);
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
    }
}

bool Bot02::DamageBot02(int index, int amount) {
    hp[index] -= amount;
    if (hp[index] <= 0) {
        SpawnKillDebris(positions[index], Vec3(0.6f, 0.0f, 1.0f), amount > 1 ? 14 : 8);
        positions      .erase(positions.begin()       + index);
        targetPositions.erase(targetPositions.begin() + index);
        hoverPhases    .erase(hoverPhases.begin()     + index);
        hp             .erase(hp.begin()              + index);
        hitTimers      .erase(hitTimers.begin()       + index);
        fireTimers     .erase(fireTimers.begin()      + index);
        return true;
    }
    hitTimers[index] = 0.18f;
    SpawnHitDebris(positions[index], Vec3(1.0f, 0.8f, 0.0f), amount > 1 ? 6 : 3);
    return false;
}

void Bot02::RemoveBot02(int index) {
    SpawnKillDebris(positions[index], Vec3(0.6f, 0.0f, 1.0f), 8);
    positions      .erase(positions.begin()       + index);
    targetPositions.erase(targetPositions.begin() + index);
    hoverPhases    .erase(hoverPhases.begin()     + index);
    hp             .erase(hp.begin()              + index);
    hitTimers      .erase(hitTimers.begin()       + index);
    fireTimers     .erase(fireTimers.begin()      + index);
}

void Bot02::RemoveBullet(int index) {
    bulletPositions .erase(bulletPositions.begin()  + index);
    bulletVelocities.erase(bulletVelocities.begin() + index);
}

void Bot02::Reset() {
    positions.clear();
    targetPositions.clear();
    hoverPhases.clear();
    hp.clear();
    hitTimers.clear();
    fireTimers.clear();
    bulletPositions.clear();
    bulletVelocities.clear();
    debris.clear();
    thrustTimer = 0.0f;
    hoverTimer  = 0.0f;
}
