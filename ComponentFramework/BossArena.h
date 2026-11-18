#ifndef BOSSARENA_H
#define BOSSARENA_H

#include <Vector.h>

using namespace MATH;

// The floor of the boss fight, a steel station deck seen from above with vents and rails.
// You see open space above and below the deck.
// It is drawn with plain glScissor and glClear rectangles before the 3D objects,
// so everything else (the player, the boss, the bullets) always draws on top of it.
class BossArena {
private:
	// what the screen looks like right now, used to turn world units into pixels
	struct View {
		int   x, y, w, h;         // the viewport in pixels
		float pixelsPerUnit;      // pixels in one world unit
		float halfWidth;          // half of the visible width in world units
	};

	float scroll;                 // how far the floor has moved to the left, in world units

	// Tuning
	static constexpr float kScrollSpeed = 0.8f;    // how fast the floor moves left, world units per second
	static constexpr float kDeckHalf = 6.1f;       // the deck covers y from -6.1 to 6.1, the same as the player's limit
	static constexpr float kRailOuter = 6.65f;     // the rails end here, past this you see space
	static constexpr float kPanelWidth = 3.1f;     // width of one steel plate
	static constexpr int   kRows = 4;              // plates from the bottom to the top of the deck
	static constexpr float kSeam = 0.04f;          // half the width of the dark gap between plates
	static constexpr float kVentHalf = 0.7f;       // half the width of a vent
	static constexpr float kSlatPitch = 0.45f;     // distance between the slats of a vent
	static constexpr float kLoopLength = 62.0f;    // scroll goes back to 0 after this, it is 10 vent pairs so nothing jumps

	static View GetView();
	static void FillRect(const View& v, float x0, float y0, float x1, float y1, const Vec3& color);
	void DrawVent(const View& v, float centerX) const;
	void DrawRail(const View& v, bool top) const;

public:
	BossArena();
	~BossArena();

	void Update(float deltaTime);
	void RenderBackground() const;

	float GetScrollSpeed() const { return kScrollSpeed; }

	// The deck covers the stars. This gives the screen rows (pixels from the top) that the deck covers.
	void GetStarMask(float& topY, float& bottomY) const;
};

#endif // BOSSARENA_H
