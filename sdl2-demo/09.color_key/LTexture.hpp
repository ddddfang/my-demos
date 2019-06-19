#ifndef __LTEXTURE_HPP__
#define __LTEXTURE_HPP__

#include <SDL.h>
#include <SDL_image.h>
#include <string> //c_str()

//Texture wrapper class
class LTexture {
public:
	//Initializes variables 
	LTexture();
	//Deallocates memory
	~LTexture();

	//Loads image at specified path 
	bool loadFromFile(SDL_Renderer *renderer, std::string path );
	//Deallocates texture 
	void free();
	//Renders texture at given point 
	void render( SDL_Renderer *renderer,int x, int y, SDL_Rect* clip = NULL);
	//Set color modulation
	void setColor( Uint8 red, Uint8 green, Uint8 blue );

	//Gets image dimensions
	int getWidth();
	int getHeight();
private:
	//The actual hardware texture
	SDL_Texture* mTexture;

	//Image dimensions 
	int mWidth;
	int mHeight;
};

#endif

