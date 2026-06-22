#include "Window.h"
#include "Debug.h"






#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_opengl3.h"








Window::Window(): window{nullptr}, context{nullptr},  width{0}, height{0} {}

Window::~Window() {
	OnDestroy();
}

bool Window::OnCreate(std::string name_, int width_, int height_) {
	if (!SDL_Init(SDL_INIT_VIDEO)) {
		Debug::FatalError("Failed to initialize SDL", __FILE__, __LINE__);
		return false;
	}

	if (!SDL_Init(SDL_INIT_AUDIO)) {
		Debug::FatalError("Failed to initialize SDL", __FILE__, __LINE__);
		return false;
	}

	this->width = width_;
	this->height = height_;
	window = SDL_CreateWindow(name_.c_str(),width, height,SDL_WINDOW_OPENGL);

	if (window == nullptr) {
		Debug::FatalError("Failed to create a window", __FILE__, __LINE__);
		return false;
	}
	context = SDL_GL_CreateContext(window);
	int major, minor;
	getInstalledOpenGLInfo(&major,&minor);
	setAttributes(major,minor);

	/// Fire up the GL Extension Wrangler (GLEW)
	GLenum err = glewInit();
	if (err != GLEW_OK) {
		Debug::FatalError("Glew initialization failed", __FILE__, __LINE__);
		return false;
	}
	glViewport(0, 0, width, height);
	return true;
}

void Window::SetFullscreen(bool full) {
    SDL_SetWindowFullscreen(window, full);
    if (!full)
        CenterInWorkArea();
    int pw = 0, ph = 0;
    SDL_GetWindowSizeInPixels(window, &pw, &ph);
    width = pw; height = ph;
    glViewport(0, 0, pw, ph);
}

void Window::SetSize(int w, int h) {
    SDL_SetWindowSize(window, w, h);
    CenterInWorkArea();
    int pw = 0, ph = 0;
    SDL_GetWindowSizeInPixels(window, &pw, &ph);
    width = pw; height = ph;
    glViewport(0, 0, pw, ph);
}

void Window::CenterInWorkArea() {
    // Use the usable display bounds (excludes taskbar, dock, etc.) so the
    // window title bar is always visible when returning from fullscreen.
    SDL_DisplayID display = SDL_GetDisplayForWindow(window);
    SDL_Rect usable = {};
    if (SDL_GetDisplayUsableBounds(display, &usable)) {
        int winW = 0, winH = 0;
        SDL_GetWindowSize(window, &winW, &winH);
        int x = usable.x + (usable.w - winW) / 2;
        int y = usable.y + (usable.h - winH) / 2;
        // Clamp so the title bar never goes above the top of the work area
        if (y < usable.y) y = usable.y;
        SDL_SetWindowPosition(window, x, y);
    } else {
        SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    }
}

void Window::OnDestroy() {
	SDL_GL_DestroyContext(context);
	SDL_DestroyWindow(window);
	window = nullptr;
}





void Window::getInstalledOpenGLInfo(int *major, int *minor) {
	/// You can to get some info regarding versions and manufacturer
	const GLubyte *version = glGetString(GL_VERSION);
	/// You can also get the version as ints	
	const GLubyte *vendor = glGetString(GL_VENDOR);
	const GLubyte *renderer = glGetString(GL_RENDERER);
	const GLubyte *glslVersion = glGetString(GL_SHADING_LANGUAGE_VERSION);

	
	glGetIntegerv(GL_MAJOR_VERSION, major);
	glGetIntegerv(GL_MINOR_VERSION, minor);
	Debug::Info("OpenGL version: " + std::string((char*)glGetString(GL_VERSION)), __FILE__, __LINE__);
	Debug::Info("Graphics card vendor " + std::string((char*)vendor), __FILE__, __LINE__);
	Debug::Info("Graphics card name " + std::string((char*)renderer), __FILE__, __LINE__);
	Debug::Info("GLSL Version " + std::string((char*)glslVersion), __FILE__, __LINE__);
	return;
}

void Window::setAttributes(int major_, int minor_) {
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, major_);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, minor_);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, true);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 32);

	SDL_GL_SetSwapInterval(1);
	glewExperimental = GL_TRUE;
	return;
}


