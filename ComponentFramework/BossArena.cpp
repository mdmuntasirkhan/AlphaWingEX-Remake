#include "BossArena.h"
#include "BossGate.h"
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

BossArena::BossArena() : scroll{ 0.0f }, extentLeft{ -1000.0f }, extentRight{ 1000.0f }, speed{ kScrollSpeed }, leftCap{ true } {}

BossArena::~BossArena() {}

void BossArena::Update(float deltaTime) {
	scroll += speed * deltaTime;

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
	v.clipLeft = -1000.0f;
	v.clipRight = 1000.0f;
	return v;
}

// Fills a rectangle given in world units. x goes right, y goes up, and 0,0 is the middle of the screen.
// The scissor box limits glClear to the rectangle.
void BossArena::FillRect(const View& v, float x0, float y0, float x1, float y1, const Vec3& color) {
	// keep it inside the part of the screen that is allowed
	if (x0 < v.clipLeft) x0 = v.clipLeft;
	if (x1 > v.clipRight) x1 = v.clipRight;
	if (x1 <= x0) return;

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

	// while the floor slides in or out, only the part between the two edges is drawn
	v.clipLeft = extentLeft;
	v.clipRight = extentRight;

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

	// a dark end cap with a thin cyan light where the floor begins or ends, so the edge looks built
	View whole = GetView();
	if (extentLeft > left && leftCap) {
		FillRect(whole, extentLeft, -kRailOuter, extentLeft + 0.35f, kRailOuter, kRailLipColor);
		FillRect(whole, extentLeft + 0.35f, -kRailOuter, extentLeft + 0.42f, kRailOuter, kLightColor);
	}
	if (extentRight < right) {
		FillRect(whole, extentRight - 0.35f, -kRailOuter, extentRight, kRailOuter, kRailLipColor);
		FillRect(whole, extentRight - 0.42f, -kRailOuter, extentRight - 0.35f, kRailOuter, kLightColor);
	}

	glDisable(GL_SCISSOR_TEST);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
}

void BossArena::GetStarMask(float& topY, float& bottomY, float& leftX, float& rightX) const {
	View v = GetView();
	topY = v.h * 0.5f - kRailOuter * v.pixelsPerUnit;
	bottomY = v.h * 0.5f + kRailOuter * v.pixelsPerUnit;

	// the floor only exists between the two extents, so the mask does too
	leftX = v.w * 0.5f + extentLeft * v.pixelsPerUnit;
	rightX = v.w * 0.5f + extentRight * v.pixelsPerUnit;
}

// The hull of the arena entrance (the stepped corridor), drawn on top of the floor. It lives in this file because
// this file already has the drawing code and is part of the project.
// The corridor is open: you see the floor plates through it. Everything else is solid hull, with yellow and
// black stripes along the floors and ceilings of the corridor, and cyan lights on the walls that face you.
void BossGate::RenderBackground() const {
	if (!started) return;

	BossArena::View v = BossArena::GetView();
	if (leadX > v.halfWidth + 1.0f) return;   // still off the right edge

	glEnable(GL_SCISSOR_TEST);

	const Vec3 hull(0.07f, 0.09f, 0.11f);
	const Vec3 hullLight(0.11f, 0.14f, 0.17f);
	const Vec3 stripeYellow(0.80f, 0.62f, 0.12f);
	const Vec3 stripeDark(0.05f, 0.05f, 0.06f);
	const Vec3 cyan(0.15f, 0.85f, 0.95f);

	const float panel = 1.6f;       // width of one hull plate, they take turns dark and light
	const float stripePitch = 0.5f; // length of one stripe
	const float stripeThick = 0.14f;
	const float lightThick = 0.07f;
	const float screenLeft = -v.halfWidth - 1.0f;
	const float screenRight = v.halfWidth + 1.0f;

	for (int c = 0; c < kColumns; c++) {
		float x0 = leadX + ColumnStart(c);
		float x1 = leadX + ColumnEnd(c);
		if (x1 < screenLeft || x0 > screenRight) continue;

		float lo = ColumnLo(c);
		float hi = ColumnHi(c);

		// the solid hull above and below the corridor, in plates
		int firstPlate = (int)floorf((x0 - leadX) / panel);
		int lastPlate = (int)floorf((x1 - leadX) / panel);
		for (int g = firstPlate; g <= lastPlate; g++) {
			float px0 = fmaxf(x0, leadX + g * panel);
			float px1 = fminf(x1, leadX + (g + 1) * panel);
			if (px1 <= px0 || px1 < screenLeft || px0 > screenRight) continue;
			const Vec3& color = (g & 1) ? hullLight : hull;
			BossArena::FillRect(v, px0, -kTop, px1, lo, color);
			BossArena::FillRect(v, px0, hi, px1, kTop, color);
		}

		// hazard stripes on the floor and on the ceiling of the corridor
		int firstStripe = (int)floorf((x0 - leadX) / stripePitch);
		int lastStripe = (int)floorf((x1 - leadX) / stripePitch);
		for (int g = firstStripe; g <= lastStripe; g++) {
			float sx0 = fmaxf(x0, leadX + g * stripePitch);
			float sx1 = fminf(x1, leadX + (g + 1) * stripePitch);
			if (sx1 <= sx0 || sx1 < screenLeft || sx0 > screenRight) continue;
			const Vec3& color = (g & 1) ? stripeDark : stripeYellow;
			BossArena::FillRect(v, sx0, lo, sx1, lo + stripeThick, color);
			BossArena::FillRect(v, sx0, hi - stripeThick, sx1, hi, color);
		}
	}

	// cyan lights on the walls that face the player. Between two columns the band gets taller or shorter,
	// and the part that changes is a wall.
	for (int c = 0; c + 1 < kColumns; c++) {
		float x = leadX + ColumnEnd(c);
		if (x < screenLeft || x > screenRight) continue;

		float loA = ColumnLo(c), loB = ColumnLo(c + 1);
		float hiA = ColumnHi(c), hiB = ColumnHi(c + 1);

		// the floor steps down: the wall is the end of the floor on the front side
		if (loB < loA) BossArena::FillRect(v, x - lightThick, loB, x, loA, cyan);
		// the floor steps up: the wall is the start of the floor on the back side
		if (loB > loA) BossArena::FillRect(v, x, loA, x + lightThick, loB, cyan);
		// the ceiling steps down: the wall is the start of the ceiling on the back side
		if (hiB < hiA) BossArena::FillRect(v, x, hiB, x + lightThick, hiA, cyan);
		// the ceiling steps up: the wall is the end of the ceiling on the front side
		if (hiB > hiA) BossArena::FillRect(v, x - lightThick, hiA, x, hiB, cyan);
	}

	// the front face, so you can see where the section starts and where the mouth is
	BossArena::FillRect(v, leadX, -kTop, leadX + 0.3f, ColumnLo(0), hullLight);
	BossArena::FillRect(v, leadX, ColumnHi(0), leadX + 0.3f, kTop, hullLight);
	BossArena::FillRect(v, leadX, -kTop, leadX + lightThick, ColumnLo(0), cyan);
	BossArena::FillRect(v, leadX, ColumnHi(0), leadX + lightThick, kTop, cyan);

	// the back end, where the corridor opens into the arena
	float endX = leadX + kEnd;
	BossArena::FillRect(v, endX - lightThick, -kTop, endX, ColumnLo(kColumns - 1), cyan);
	BossArena::FillRect(v, endX - lightThick, ColumnHi(kColumns - 1), endX, kTop, cyan);

	glDisable(GL_SCISSOR_TEST);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
}

// The hull covers the whole height of the screen between its front and its back, so the stars are hidden there.
void BossGate::GetStarMask(float& topY, float& bottomY, float& leftX, float& rightX) const {
	BossArena::View v = BossArena::GetView();
	topY = 0.0f;
	bottomY = (float)v.h;
	leftX = v.w * 0.5f + leadX * v.pixelsPerUnit;
	rightX = v.w * 0.5f + (leadX + kEnd) * v.pixelsPerUnit;
}
