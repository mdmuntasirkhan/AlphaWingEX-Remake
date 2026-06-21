#include "LevelDirector.h"
#include <MMath.h>
#include <glew.h>
#include <algorithm>
#include <iostream>

LevelDirector::LevelDirector() :
    levelTime{ 0.0f },
    nextEvent{ 0 }
{
}

void LevelDirector::AddScript(LevelScript* script, float timeOffset) {
    std::vector<LevelEvent> events = script->GetEvents();

    for (LevelEvent& e : events) {
        // Shift local timestamp to absolute time on the master timeline
        e.time += timeOffset;

        // Pre-load the mesh now so there is no stutter when the event fires at runtime
        if (e.type == EventType::SPAWN_ENV_CHUNK && e.meshFile != nullptr) {
            std::string key(e.meshFile);
            if (meshPool.find(key) == meshPool.end()) {
                Mesh* m = new Mesh(e.meshFile);
                if (m->OnCreate()) {
                    meshPool[key] = m;
                } else {
                    std::cout << "LevelDirector: failed to load mesh: " << e.meshFile << "\n";
                    delete m;
                }
            }
        }

        timeline.push_back(e);
    }

    // Keep timeline sorted so Update() can walk it in order
    std::sort(timeline.begin(), timeline.end(),
        [](const LevelEvent& a, const LevelEvent& b) { return a.time < b.time; });

    delete script;
}

void LevelDirector::Update(float deltaTime) {
    levelTime += deltaTime;

    // Fire any events whose time has arrived
    while (nextEvent < (int)timeline.size() && timeline[nextEvent].time <= levelTime) {
        FireEvent(timeline[nextEvent]);
        nextEvent++;
    }

    // Scroll all active chunks left and cull anything that has passed the left edge
    for (int i = (int)activeChunks.size() - 1; i >= 0; i--) {
        activeChunks[i].pos.x -= activeChunks[i].scrollSpeed * deltaTime;
        if (activeChunks[i].pos.x < -18.0f) {
            activeChunks.erase(activeChunks.begin() + i);
        }
    }
}

void LevelDirector::Render(Shader* shader,
                            const Matrix4& projectionMatrix,
                            const Matrix4& viewMatrix) const {
    if (activeChunks.empty()) return;

    glUniformMatrix4fv(shader->GetUniformID("projectionMatrix"), 1, GL_FALSE, projectionMatrix);
    glUniformMatrix4fv(shader->GetUniformID("viewMatrix"),       1, GL_FALSE, viewMatrix);
    glUniform1f(shader->GetUniformID("emissive"), 0.0f); // standard Phong, no glow

    for (const ActiveChunk& chunk : activeChunks) {
        glUniform4f(shader->GetUniformID("color"),
                    chunk.color.x, chunk.color.y, chunk.color.z, 1.0f);
        Matrix4 model = MMath::translate(chunk.pos) *
                        MMath::scale(chunk.scale, chunk.scale, chunk.scale);
        glUniformMatrix4fv(shader->GetUniformID("modelMatrix"), 1, GL_FALSE, model);
        chunk.mesh->Render();
    }
}

void LevelDirector::OnDestroy() {
    activeChunks.clear();
    for (auto& pair : meshPool) {
        pair.second->OnDestroy();
        delete pair.second;
    }
    meshPool.clear();
    timeline.clear();
    nextEvent = 0;
}

void LevelDirector::FireEvent(const LevelEvent& e) {
    if (e.type == EventType::SPAWN_ENV_CHUNK && e.meshFile != nullptr) {
        auto it = meshPool.find(std::string(e.meshFile));
        if (it != meshPool.end()) {
            ActiveChunk chunk;
            chunk.pos         = e.position;
            chunk.mesh        = it->second;
            chunk.color       = e.color;
            chunk.scale       = e.scale;
            chunk.scrollSpeed = e.scrollSpeed;
            activeChunks.push_back(chunk);
        }
    }
}
