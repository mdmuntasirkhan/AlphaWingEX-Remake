#include "Texture.h"

Texture::Texture() {
	textureID = 0;
}

bool Texture::LoadImage(const char* filename) {
	glGenTextures(1, &textureID); // assign an ID to texture
	glBindTexture(GL_TEXTURE_2D, textureID); // bind the texture
	SDL_Surface *textureSurface = IMG_Load(filename);
	if (textureSurface == nullptr) {
		return false;
	}
	int mode = SDL_BYTESPERPIXEL(textureSurface->format) == 4 ? GL_RGBA : GL_RGB; // check for JPG / PNG
	glTexImage2D(GL_TEXTURE_2D, 0, mode, textureSurface->w, textureSurface->h, 0, mode, GL_UNSIGNED_BYTE, textureSurface->pixels);
	SDL_DestroySurface(textureSurface);
	/// Wrapping and filtering options
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); // simular to min / max 
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glBindTexture(GL_TEXTURE_2D, 0); /// Unbind the texture
	return true;
}


Texture::~Texture() {
	glDeleteTextures(1, &textureID);
}