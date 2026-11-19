#ifndef BOSSENCOUNTER_H
#define BOSSENCOUNTER_H

#include "BossGate.h"
#include "BossArena.h"
#include "GameConstants.h"

// The flow of the whole boss section, from the level event to the end.
// It only keeps track of the stage and the timers. The scene reads it every frame and does the real work
// (starting the boss, moving the floor, saving). All of it is in this header.
//
//   LEAD_IN   a short quiet moment, the level is over and the enemies stop coming
//   ENTRY     the arena section slides in from the right. Its front is hull with a corridor in it. The corridor
//             starts at the very top, runs straight, drops, runs straight, drops again and opens into the
//             big arena (see BossGate). The player flies through it.
//   WARNING   the player is out of the corridor and sees the big arena. 3 seconds of WARNING on the screen.
//   FIGHT     the spider walks in, then the fight. It lasts until the boss is dead.
//   ARENA_OUT the floor opens up at the right end and slides away to the left
//   FADE_OUT  the screen goes black
//   BLACK     a moment of black, the scene saves the checkpoint here
//   FADE_IN   the screen comes back, with the banner
//   DONE      finished, the level carries on
class BossEncounter {
public:
	enum class Stage { WAITING, LEAD_IN, ENTRY, WARNING, FIGHT, ARENA_OUT, FADE_OUT, BLACK, FADE_IN, DONE };

private:
	Stage stage;
	float timer;               // seconds since this stage started
	float leadInTime;
	float floorSpeed;          // how fast the floor (and the wall on it) moves left right now
	float settleFrom;          // the floor speed the slowdown in FIGHT starts from
	BossGate gate;

	// one-time messages for the scene
	bool spawnBossRequested;
	bool checkpointRequested;
	bool finishedRequested;
	bool bossSpawned;

	// Tuning
	static constexpr float kWarningTime = 3.0f;
	static constexpr float kEntrySpeed = 3.4f;       // how fast the arena section slides in
	static constexpr float kWarningAt = 0.0f;        // the WARNING starts when the back end of the corridor passes this x
	static constexpr float kSettleTime = 1.5f;       // seconds the floor takes to slow down to its normal speed
	static constexpr float kArenaOutTime = 3.0f;
	static constexpr float kFadeTime = 1.2f;
	static constexpr float kBlackTime = 0.8f;

	void EnterStage(Stage next) { stage = next; timer = 0.0f; }

	// starts fast and settles, t goes from 0 to 1
	static float Ease(float t) {
		if (t < 0.0f) t = 0.0f;
		if (t > 1.0f) t = 1.0f;
		return 1.0f - (1.0f - t) * (1.0f - t);
	}

public:
	BossEncounter() :
		stage{ Stage::WAITING }, timer{ 0.0f }, leadInTime{ 0.0f },
		floorSpeed{ BossArena::kScrollSpeed }, settleFrom{ kEntrySpeed },
		spawnBossRequested{ false }, checkpointRequested{ false }, finishedRequested{ false }, bossSpawned{ false } {
	}

	// Starts the section. leadIn is the quiet time before the warning.
	// Returns false and does nothing if it already started or already finished.
	bool Start(float leadIn) {
		if (stage != Stage::WAITING) return false;
		leadInTime = leadIn;
		EnterStage(Stage::LEAD_IN);
		return true;
	}

	// For testing: jumps straight to the fight, with the boss walking in
	void SkipToFight() {
		if (stage == Stage::WAITING || stage == Stage::LEAD_IN || stage == Stage::ENTRY || stage == Stage::WARNING) {
			gate.Reset();
			if (!bossSpawned) { spawnBossRequested = true; bossSpawned = true; }
			settleFrom = BossArena::kScrollSpeed;
			EnterStage(Stage::FIGHT);
		}
	}

	// The player was crushed by the hull. The section goes back to the right edge and the corridor starts again.
	// The scene sends the boss back too (it is not out yet before the WARNING ends).
	void RestartEntry() {
		if (stage != Stage::ENTRY && stage != Stage::WARNING) return;
		gate.Start();
		bossSpawned = false;
		spawnBossRequested = false;
		EnterStage(Stage::ENTRY);
	}

	// Back to the very start, for Try Again
	void Reset() {
		stage = Stage::WAITING;
		timer = 0.0f;
		floorSpeed = BossArena::kScrollSpeed;
		settleFrom = kEntrySpeed;
		gate.Reset();
		spawnBossRequested = false;
		checkpointRequested = false;
		finishedRequested = false;
		bossSpawned = false;
	}

	// The scene calls this when the boss is dead
	void OnBossDefeated() {
		// it counts even if the boss dies while the floor is still sliding in (the F7 dev key does that)
		if (stage == Stage::ENTRY || stage == Stage::WARNING || stage == Stage::FIGHT) {
			gate.Reset();
			EnterStage(Stage::ARENA_OUT);
		}
	}

	void Update(float deltaTime) {
		timer += deltaTime;

		switch (stage) {
		case Stage::LEAD_IN:
			if (timer >= leadInTime) {
				gate.Start();
				floorSpeed = kEntrySpeed;
				settleFrom = kEntrySpeed;
				EnterStage(Stage::ENTRY);
			}
			break;

		case Stage::ENTRY:
			floorSpeed = kEntrySpeed;
			gate.Update(deltaTime, floorSpeed);

			// the back end of the corridor has passed the player, the big arena is open in front of him
			if (gate.GetLeadX() + BossGate::kEnd < kWarningAt) EnterStage(Stage::WARNING);
			break;

		case Stage::WARNING:
			floorSpeed = kEntrySpeed;
			gate.Update(deltaTime, floorSpeed);

			// after the WARNING the spider walks in
			if (timer >= kWarningTime) {
				if (!bossSpawned) { bossSpawned = true; spawnBossRequested = true; }
				settleFrom = kEntrySpeed;
				EnterStage(Stage::FIGHT);
			}
			break;

		case Stage::FIGHT:
			// the floor slows down to its normal speed
			floorSpeed = settleFrom + (BossArena::kScrollSpeed - settleFrom) * Ease(timer / kSettleTime);

			// the last of the hull is still sliding out to the left
			if (gate.IsStarted()) {
				gate.Update(deltaTime, floorSpeed);
				if (gate.IsFinished()) gate.Reset();
			}
			break;

		case Stage::ARENA_OUT:
			floorSpeed = BossArena::kScrollSpeed;
			if (timer >= kArenaOutTime) EnterStage(Stage::FADE_OUT);
			break;

		case Stage::FADE_OUT:
			if (timer >= kFadeTime) {
				EnterStage(Stage::BLACK);
				checkpointRequested = true;
			}
			break;

		case Stage::BLACK:
			if (timer >= kBlackTime) EnterStage(Stage::FADE_IN);
			break;

		case Stage::FADE_IN:
			if (timer >= kFadeTime) {
				EnterStage(Stage::DONE);
				finishedRequested = true;
			}
			break;

		default:
			break;   // WAITING and DONE just wait
		}
	}

	// Each of these is true once, when the thing happens
	bool PopSpawnBoss() { bool v = spawnBossRequested; spawnBossRequested = false; return v; }
	bool PopCheckpoint() { bool v = checkpointRequested; checkpointRequested = false; return v; }
	bool PopFinished() { bool v = finishedRequested; finishedRequested = false; return v; }

	Stage GetStage() const { return stage; }
	bool IsActive() const { return stage != Stage::WAITING && stage != Stage::DONE; }
	bool IsGateVisible() const { return gate.IsStarted(); }   // the corridor hull is on the screen
	bool IsArenaVisible() const { return stage == Stage::ENTRY || stage == Stage::WARNING || stage == Stage::FIGHT || stage == Stage::ARENA_OUT; }

	BossGate& GetGate() { return gate; }
	const BossGate& GetGate() const { return gate; }

	// how fast the floor moves left right now, the scene gives this to the arena and the boss's legs
	float GetFloorSpeed() const { return floorSpeed; }

	// where the floor starts and ends right now, in world units
	void GetArenaExtent(float& left, float& right) const {
		// half the width of the screen in world units
		float halfX = GameConst::kSpawnX - 2.55f;
		float farRight = halfX + 1.0f;
		float farLeft = -halfX - 1.0f;

		left = -1000.0f;
		right = 1000.0f;

		if (gate.IsStarted()) {
			// the floor starts at the front face of the section
			left = gate.GetLeadX();
		}
		if (stage == Stage::ARENA_OUT) {
			// the right end of the floor runs across the screen to the left, so the floor ends and opens up behind it
			right = farRight + (farLeft - farRight) * Ease(timer / kArenaOutTime);
		}
	}

	float GetWarningTime() const { return timer; }   // seconds since the WARNING started, the HUD flashes with it

	float GetFade() const {   // 0 is clear, 1 is black
		if (stage == Stage::FADE_OUT) return timer / kFadeTime;
		if (stage == Stage::BLACK) return 1.0f;
		if (stage == Stage::FADE_IN) return 1.0f - timer / kFadeTime;
		return 0.0f;
	}

	float GetBannerAlpha() const {   // the "boss defeated" text, 0 is hidden
		if (stage == Stage::BLACK) return 1.0f;
		if (stage == Stage::FADE_IN) return 1.0f - timer / kFadeTime;
		return 0.0f;
	}
};

#endif // BOSSENCOUNTER_H
