#ifndef BOSSARENA_H
#define BOSSARENA_H

#include <Vector.h>

using namespace MATH;

// The floor of the boss fight, a steel station deck seen from above with vents and rails.
// You see open space above and below the deck.
// It is drawn with plain glScissor and glClear rectangles before the 3D objects,
// so everything else (the player, the boss, the bullets) always draws on top of it.
class BossArena {
public:
	// what the screen looks like right now, used to turn world units into pixels
	struct View {
		int   x, y, w, h;         // the viewport in pixels
		float pixelsPerUnit;      // pixels in one world unit
		float halfWidth;          // half of the visible width in world units
		float clipLeft;           // nothing is drawn left of this (world units), the arena uses it to slide in
		float clipRight;          // nothing is drawn right of this
	};

	// These two are public so the gate can draw with the same scissor rectangles
	static View GetView();
	static void FillRect(const View& v, float x0, float y0, float x1, float y1, const Vec3& color);

private:
	float scroll;                 // how far the floor has moved to the left, in world units
	float extentLeft;             // the floor only exists between these two x values
	float extentRight;

	float speed;                  // how fast the floor moves left right now
	bool  leftCap;                // draw the dark end cap with the cyan light at the left end of the floor

	// Tuning
	static constexpr float kDeckHalf = 6.1f;       // the deck covers y from -6.1 to 6.1, the same as the player's limit
	static constexpr float kRailOuter = 6.65f;     // the rails end here, past this you see space
	static constexpr float kPanelWidth = 3.1f;     // width of one steel plate
	static constexpr int   kRows = 4;              // plates from the bottom to the top of the deck
	static constexpr float kSeam = 0.04f;          // half the width of the dark gap between plates
	static constexpr float kVentHalf = 0.7f;       // half the width of a vent
	static constexpr float kSlatPitch = 0.45f;     // distance between the slats of a vent
	static constexpr float kLoopLength = 62.0f;    // scroll goes back to 0 after this, it is 10 vent pairs so nothing jumps

	void DrawVent(const View& v, float centerX) const;
	void DrawRail(const View& v, bool top) const;

public:
	static constexpr float kScrollSpeed = 0.8f;    // the normal speed of the floor during the fight, world units per second

	BossArena();
	~BossArena();

	void Update(float deltaTime);
	void RenderBackground() const;

	// The floor goes faster while it slides in, the encounter sets this every frame
	void SetScrollSpeed(float s) { speed = s; }
	float GetScrollSpeed() const { return speed; }

	// Where the floor starts and ends. The encounter moves these to slide the floor in and out.
	// A very small left and a very big right means the whole screen is floor.
	void SetExtent(float left, float right) { extentLeft = left; extentRight = right; }

	// The corridor hull covers the left end of the floor, so the end cap is turned off while the corridor is on screen
	void SetLeftCap(bool on) { leftCap = on; }

	// The deck covers the stars. This gives the screen rows (pixels from the top) that the deck covers,
	// and the screen columns (pixels from the left) it covers right now.
	void GetStarMask(float& topY, float& bottomY, float& leftX, float& rightX) const;
};

#endif // BOSSARENA_H
