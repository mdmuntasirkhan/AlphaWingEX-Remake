#ifndef BOSSGATE_H
#define BOSSGATE_H

#include "Player.h"
#include "GameConstants.h"
#include <cmath>

// The entrance of the boss arena: a stepped corridor.
// The whole section comes in from the right. Everything in it is solid hull except the corridor.
//
//   The mouth is at the very top. The corridor runs straight, drops down, runs straight, drops down again,
//   and then the hull ends and you are in the big arena.
//
//        mouth -> ====================.
//                                     |
//                          ==========.|
//                                    ||
//                             =========== -> the arena
//
// The corridor is described with 5 columns (slices from front to back). In every column the open part is
// one band from ColumnLo to ColumnHi, and everything above and below it is solid. The drops are the
// columns where the band is tall. The same table is used for the collision and for the drawing.
//
// The hull pushes the player. The section moves left, so the faces that look at the player push him left.
// If he is pushed into the left edge of the screen, he is crushed (the scene takes a life).
//
// This class is all in the header. The drawing (RenderBackground) is in BossArena.cpp.
class BossGate {
private:
	float leadX;     // the x of the front face of the section, it moves left
	bool  started;

public:
	// Tuning
	static constexpr int   kColumns = 5;
	static constexpr float kShaft = 2.6f;      // width of a drop. The ship is 1.3 wide, so there is room
	static constexpr float kTop = 8.0f;        // the hull goes past the top and bottom of the screen
	static constexpr float kPlayerHalfX = 0.65f;   // the size the hull uses for the ship
	static constexpr float kPlayerHalfY = 0.30f;

	// The three straight parts of the corridor (the open band, y from low to high). Each one is 2.3 tall.
	static constexpr float kAHi = 6.1f;    // top level, it touches the top of the deck
	static constexpr float kALo = 3.8f;
	static constexpr float kBHi = 2.8f;    // middle level
	static constexpr float kBLo = 0.5f;
	static constexpr float kCHi = -0.5f;   // low level, this one leads out into the arena
	static constexpr float kCLo = -2.8f;

	// Where things are, in world units behind the front face
	static constexpr float kShaft1 = 6.0f;    // the first drop starts here
	static constexpr float kShaft2 = 11.5f;   // the second drop starts here
	static constexpr float kEnd = 21.5f;      // the hull ends here, after this it is the open arena

	// Column c goes from ColumnStart(c) to ColumnEnd(c) behind the front face.
	//   0: top level straight    1: first drop    2: middle level straight    3: second drop    4: low level straight
	static float ColumnStart(int c) {
		switch (c) {
		case 0: return 0.0f;
		case 1: return kShaft1;
		case 2: return kShaft1 + kShaft;
		case 3: return kShaft2;
		default: return kShaft2 + kShaft;
		}
	}
	static float ColumnEnd(int c) { return (c >= kColumns - 1) ? kEnd : ColumnStart(c + 1); }

	// The open band of the column. Below ColumnLo and above ColumnHi is solid.
	static float ColumnLo(int c) {
		switch (c) {
		case 0: return kALo;
		case 1: return kBLo;
		case 2: return kBLo;
		default: return kCLo;
		}
	}
	static float ColumnHi(int c) {
		switch (c) {
		case 0: return kAHi;
		case 1: return kAHi;
		case 2: return kBHi;
		case 3: return kBHi;
		default: return kCHi;
		}
	}

	BossGate() : leadX{ 0.0f }, started{ false } {}

	// the section starts just off the right edge of the screen
	void Start() {
		leadX = GameConst::kSpawnX;
		started = true;
	}
	void Reset() { started = false; }

	void Update(float deltaTime, float speed) {
		if (started) leadX -= speed * deltaTime;
	}

	bool IsStarted() const { return started; }
	float GetLeadX() const { return leadX; }

	// the back end of the hull is past the left edge of the screen
	bool IsFinished() const { return started && leadX + kEnd < -(GameConst::kWorldBoundX + 1.5f); }

	// Pushes the player out of one solid block. The block is x0..x1, y0..y1. The ship is a box.
	// It goes out the way that needs the least movement, but never to the right: the hull moves left,
	// so the faces push the ship to the left, and the floors and ceilings push it up or down.
	static bool PushOut(float& px, float& py, float x0, float x1, float y0, float y1) {
		float l = px - kPlayerHalfX, r = px + kPlayerHalfX;
		float b = py - kPlayerHalfY, t = py + kPlayerHalfY;
		if (r <= x0 || l >= x1 || t <= y0 || b >= y1) return false;

		float pushLeft = r - x0;
		bool above = py > (y0 + y1) * 0.5f;
		float pushVert = above ? (y1 - b) : (t - y0);

		if (pushLeft < pushVert) px -= pushLeft;
		else if (above)          py += pushVert;
		else                     py -= pushVert;
		return true;
	}

	// Pushes the player out of the hull. Returns true if he got crushed against the left edge.
	bool PushPlayer(Player* player) const {
		if (!started) return false;

		float px = player->GetPosition().x;
		float py = player->GetPosition().y;
		bool moved = false;

		// two passes, because a push out of one block can put the ship into the next one
		for (int pass = 0; pass < 2; pass++) {
			for (int c = 0; c < kColumns; c++) {
				float x0 = leadX + ColumnStart(c);
				float x1 = leadX + ColumnEnd(c);
				if (px + kPlayerHalfX <= x0 || px - kPlayerHalfX >= x1) continue;   // not in this column

				if (PushOut(px, py, x0, x1, -kTop, ColumnLo(c))) moved = true;   // the solid under the band
				if (PushOut(px, py, x0, x1, ColumnHi(c), kTop)) moved = true;     // the solid above the band
			}
		}

		if (moved) player->MoveTo(px, py);

		// The player can not fly out of the screen by himself, so if the hull still wants him farther left
		// than the edge allows, he is stuck between the hull and the edge.
		return moved && (px < -GameConst::kWorldBoundX);
	}

	void RenderBackground() const;   // in BossArena.cpp

	// the screen area the hull covers, in pixels, for hiding the stars behind it (also in BossArena.cpp)
	void GetStarMask(float& topY, float& bottomY, float& leftX, float& rightX) const;
};

#endif // BOSSGATE_H
