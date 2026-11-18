#include "BossArena.h"
#include "GameConstants.h"
#include <glew.h>
#include <cmath>

// Colors, red green blue from 0 to 1
static const Vec3 kSeamColor(0.05f, 0.09f, 0.10f);
static const Vec3 kPlateLight(0.15f, 0.25f, 0.26f);
static const Vec3 kPlateDark(0.12f, 0.21f, 0.22f);
static const Vec3 kVentColor(0.05f, 0.06f, 0.08f);
static const Vec3 kSlatColor(0.16f, 0.18f, 0.22f);
static const Vec3 kLightColor(0.15f, 0.85f, 0.95f);
static const Vec3 kRailColor(0.09f, 0.10f, 0.13f);
static const Vec3 kRailLipColor(0.03f, 0.04f, 0.05f);
static const Vec3 kDashColor(0.80f, 0.62f, 0.12f);

BossArena::BossArena() : scroll{ 0.0f } {}

BossArena::~BossArena() {}

void BossArena::Update(float deltaTime) {
	scroll += kScrollSpeed * deltaTime;

	// start over before the number gets big. kLoopLength is a whole number of vent pairs, so the floor does not jump.
	if (scroll > kLoopLength) scroll -= kLoopLength;
}

BossArena::View BossArena::GetView() {
	GLint vp[4];
	glGetIntegerv(GL_VIEWPORT, vp);

	View v;
	v.x = vp[0];
	v.y = vp[1];
	v.w = vp[2];
	v.h = vp[3];

	// how much of the world the camera sees at the depth where the objects are (the same math as ComputeWorldBounds)
	float halfHeight = 0.44523f * (GameConst::kCameraZ - GameConst::kWorldZ);
	v.pixelsPerUnit = v.h / (2.0f * halfHeight);
	v.halfWidth = (v.w * 0.5f) / v.pixelsPerUnit;
	return v;
}

// Fills a rectangle given in world units. x goes right, y goes up, and 0,0 is the middle of the screen.
// The scissor box limits glClear to the rectangle.
void BossArena::FillRect(const View& v, float x0, float y0, float x1, float y1, const Vec3& color) {
	int px0 = (int)floorf(v.x + v.w * 0.5f + x0 * v.pixelsPerUnit);
	int px1 = (int)ceilf(v.x + v.w * 0.5f + x1 * v.pixelsPerUnit);
	int py0 = (int)floorf(v.y + v.h * 0.5f + y0 * v.pixelsPerUnit);
	int py1 = (int)ceilf(v.y + v.h * 0.5f + y1 * v.pixelsPerUnit);

	// keep it inside the screen
	if (px0 < v.x) px0 = v.x;
	if (py0 < v.y) py0 = v.y;
	if (px1 > v.x + v.w) px1 = v.x + v.w;
	if (py1 > v.y + v.h) py1 = v.y + v.h;
	if (px1 <= px0 || py1 <= py0) return;

	glScissor(px0, py0, px1 - px0, py1 - py0);
	glClearColor(color.x, color.y, color.z, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
}

// A vent is a dark strip with slats, and a row of small cyan lights on each side.
void BossArena::DrawVent(const View& v, float centerX) const {
	FillRect(v, centerX - kVentHalf, -kDeckHalf, centerX + kVentHalf, kDeckHalf, kVentColor);

	for (float y = -kDeckHalf + 0.25f; y < kDeckHalf - 0.2f; y += kSlatPitch) {
		FillRect(v, centerX - kVentHalf + 0.08f, y, centerX + kVentHalf - 0.08f, y + 0.12f, kSlatColor);
	}

	for (float y = -kDeckHalf + 0.8f; y < kDeckHalf; y += 1.5f) {
		float lx = kVentHalf + 0.18f;
		FillRect(v, centerX - lx - 0.05f, y, centerX - lx + 0.05f, y + 0.1f, kLightColor);
		FillRect(v, centerX + lx - 0.05f, y, centerX + lx + 0.05f, y + 0.1f, kLightColor);
	}
}

// The rail along the edge of the deck, with yellow dashes that move with the floor.
void BossArena::DrawRail(const View& v, bool top) const {
	float sign = top ? 1.0f : -1.0f;
	float inner = sign * kDeckHalf;
	float outer = sign * kRailOuter;

	// the rail itself, and a dark line where it meets the deck
	FillRect(v, -v.halfWidth - 1.0f, fminf(inner, outer), v.halfWidth + 1.0f, fmaxf(inner, outer), kRailColor);
	FillRect(v, -v.halfWidth - 1.0f, fminf(inner, inner + sign * 0.05f), v.halfWidth + 1.0f, fmaxf(inner, inner + sign * 0.05f), kRailLipColor);

	// the dashes
	float middle = (inner + outer) * 0.5f;
	const float dashPitch = 1.9f;
	const float dashLength = 1.3f;
	int first = (int)floorf((scroll - v.halfWidth) / dashPitch) - 1;
	int last = (int)ceilf((scroll + v.halfWidth) / dashPitch) + 1;
	for (int k = first; k <= last; k++) {
		float x0 = k * dashPitch - scroll;
		FillRect(v, x0, middle - 0.035f, x0 + dashLength, middle + 0.035f, kDashColor);
	}
}

void BossArena::RenderBackground() const {
	View v = GetView();
	float left = -v.halfWidth - 1.0f;
	float right = v.halfWidth + 1.0f;

	glEnable(GL_SCISSOR_TEST);

	// the whole deck in the seam color first, the plates go on top and leave thin gaps
	FillRect(v, left, -kDeckHalf, right, kDeckHalf, kSeamColor);

	// the plates. Column numbers are fixed to the floor, so they move left with it.
	float rowHeight = 2.0f * kDeckHalf / kRows;
	int firstCol = (int)floorf((scroll - v.halfWidth) / kPanelWidth) - 1;
	int lastCol = (int)ceilf((scroll + v.halfWidth) / kPanelWidth) + 1;
	for (int col = firstCol; col <= lastCol; col++) {
		float x0 = col * kPanelWidth - scroll;
		float x1 = x0 + kPanelWidth;
		for (int row = 0; row < kRows; row++) {
			float y0 = -kDeckHalf + row * rowHeight;
			float y1 = y0 + rowHeight;
			bool light = ((col + row) & 1) == 0;   // the plates take turns, light and dark
			FillRect(v, x0 + kSeam, y0 + kSeam, x1 - kSeam, y1 - kSeam, light ? kPlateLight : kPlateDark);
		}
	}

	// a vent on every second seam
	for (int col = firstCol; col <= lastCol; col++) {
		if ((col & 1) == 0) DrawVent(v, col * kPanelWidth - scroll);
	}

	DrawRail(v, true);
	DrawRail(v, false);

	glDisable(GL_SCISSOR_TEST);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
}

void BossArena::GetStarMask(float& topY, float& bottomY) const {
	View v = GetView();
	topY = v.h * 0.5f - kRailOuter * v.pixelsPerUnit;
	bottomY = v.h * 0.5f + kRailOuter * v.pixelsPerUnit;
}
