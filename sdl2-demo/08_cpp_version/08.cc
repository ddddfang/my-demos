//SDL_image package: https://www.libsdl.org/projects/SDL_image/release/
//http://lazyfoo.net/tutorials/SDL/10_color_keying/index.php

//use SDL texture and renderer to 
//	1.load images 
//	2.render rect/line/point
//	3.set viewport

//Using SDL and standard IO 
#include <SDL.h>
#include <SDL_image.h>
#include <stdio.h>
#include <string>
using namespace std;

//Screen dimension constants 
const int SCREEN_WIDTH = 640; 
const int SCREEN_HEIGHT = 480;
//Key press surfaces constants 
enum KeyPressSurfaces {
	KEY_PRESS_SURFACE_DEFAULT,
	KEY_PRESS_SURFACE_UP,
	KEY_PRESS_SURFACE_DOWN,
	KEY_PRESS_SURFACE_LEFT,
	KEY_PRESS_SURFACE_RIGHT,
	KEY_PRESS_SURFACE_TOTAL
};
SDL_Window* gWindow = NULL;
SDL_Renderer *gRenderer = NULL;

//Texture wrapper class
class LTexture {
public:
	//Initializes variables 
	LTexture();
	//Deallocates memory
	~LTexture();
	//Loads image at specified path 
	bool loadFromFile( std::string path );
	//Deallocates texture 
	void free();
	//Renders texture at given point 
	void render( int x, int y, SDL_Rect* clip = NULL );
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

LTexture::LTexture()
{
	//Initialize
	mTexture = NULL;
	mWidth = 0;
	mHeight = 0;
}

LTexture::~LTexture()
{
	//Deallocate
	free();
}

bool LTexture::loadFromFile( std::string path )
{
	//Get rid of preexisting texture
	free();
	//The final texture
	SDL_Texture* newTexture = NULL;
	//Load image at specified path
	SDL_Surface* loadedSurface = IMG_Load( path.c_str() );
	if( loadedSurface == NULL )	{
		printf( "Unable to load image %s! SDL_image Error: %s\n", path.c_str(), IMG_GetError() );
	} else {
		//Color key image ,after set color key, the picture color where match this key will be filtered
		SDL_SetColorKey( loadedSurface, SDL_TRUE, SDL_MapRGB( loadedSurface->format, 0, 0xFF, 0xFF ) );
		//Create texture from surface pixels
		newTexture = SDL_CreateTextureFromSurface( gRenderer, loadedSurface );
		if( newTexture == NULL ){
			printf( "Unable to create texture from %s! SDL Error: %s\n", path.c_str(), SDL_GetError() );
		} else {
			//Get image dimensions
			mWidth = loadedSurface->w;
			mHeight = loadedSurface->h;
		}
		//Get rid of old loaded surface
		SDL_FreeSurface( loadedSurface );
	}
	//Return success
	mTexture = newTexture;
	return mTexture != NULL;
}

void LTexture::free()
{
	//Free texture if it exists
	if( mTexture != NULL ) {
		SDL_DestroyTexture( mTexture );
		mTexture = NULL;
		mWidth = 0;
		mHeight = 0;
	}
}

void LTexture::render( int x, int y, SDL_Rect* clip )
{
	//Set rendering space and render to screen 
	SDL_Rect renderQuad = { x, y, mWidth, mHeight };
	if(clip){
		renderQuad.w = clip->w;
		renderQuad.h = clip->h;
	}
	//render to screen, from clip of mTexture to renderQuad of screen
	SDL_RenderCopy( gRenderer, mTexture, clip, &renderQuad );
}

int LTexture::getWidth()
{
	return mWidth;
}

int LTexture::getHeight()
{
	return mHeight;
}


/************************** some globals ****************************/
//The window we'll be rendering to 
LTexture gFooTexture;
LTexture gBackgroundTexture;
SDL_Rect gSpriteClips[ 4 ];

/************************** util func ****************************/
int init()
{
	int success = 1; 

	//Initialize SDL 
	if( SDL_Init( SDL_INIT_VIDEO ) < 0 ) {
		printf( "SDL could not initialize! SDL_Error: %s\n", SDL_GetError() );
		success = 0;
	} else { 
		//Create window 
		gWindow = SDL_CreateWindow( "SDL Tutorial", 
						SDL_WINDOWPOS_UNDEFINED,
						SDL_WINDOWPOS_UNDEFINED,
						SCREEN_WIDTH, SCREEN_HEIGHT,
						SDL_WINDOW_SHOWN );
		if( gWindow == NULL ) {
			printf( "Window could not be created! SDL_Error: %s\n", SDL_GetError() );
			success = 0;
		} else {
			//create renderer for window
			gRenderer = SDL_CreateRenderer(gWindow, -1, SDL_RENDERER_ACCELERATED);
			if(gRenderer == NULL){
				printf( "Renderer could not be created! SDL Error: %s\n", SDL_GetError() );
				success = 0;
			} else{
				SDL_SetRenderDrawColor(gRenderer, 0xff, 0xff, 0xff, 0xff);
			}

			//Initialize PNG loading , will use SDL_image library
			int imgFlags = IMG_INIT_PNG;
			if( !( IMG_Init( imgFlags ) & imgFlags ) ) {
				printf( "SDL_image could not initialize! SDL_image Error: %s\n", IMG_GetError());
				success = 0;
			}
		}
	}
	return success;
}

SDL_Texture* loadTexture(std::string path)
{
	SDL_Texture *newTexture = NULL;
	SDL_Surface* loadedSurface = IMG_Load( path.c_str() );	//use SDL_image API to load png picture
	if( loadedSurface == NULL ) {
		printf( "Unable to load image %s! SDL Error: %s\n", path.c_str(), SDL_GetError() );
	} else {
		newTexture = SDL_CreateTextureFromSurface(gRenderer, loadedSurface);
		if(newTexture == NULL){
			printf( "Unable to create texture from %s! SDL Error: %s\n", path.c_str(), SDL_GetError() );
		}
		//Get rid of old loaded surface
		SDL_FreeSurface( loadedSurface );
	}
	return newTexture;
}

/*int loadMedia()
{
	int success = 1;
	gKeyPressTextures[ KEY_PRESS_SURFACE_DEFAULT ] = loadTexture( "./pics/press.bmp" );
	if(	gKeyPressTextures[ KEY_PRESS_SURFACE_DEFAULT ] == NULL ) {
		printf( "Failed to load default image!\n" );
		success = 0;
	}
	gKeyPressTextures[ KEY_PRESS_SURFACE_UP ] = loadTexture( "./pics/up.png" );
	if(	gKeyPressTextures[ KEY_PRESS_SURFACE_UP ] == NULL ) {
		printf( "Failed to load default image!\n" );
		success = 0;
	}
	gKeyPressTextures[ KEY_PRESS_SURFACE_DOWN ] = loadTexture( "./pics/down.png" );
	if(	gKeyPressTextures[ KEY_PRESS_SURFACE_DOWN ] == NULL ) {
		printf( "Failed to load default image!\n" );
		success = 0;
	}
	gKeyPressTextures[ KEY_PRESS_SURFACE_LEFT ] = loadTexture( "./pics/left.png" );
	if(	gKeyPressTextures[ KEY_PRESS_SURFACE_LEFT ] == NULL ) {
		printf( "Failed to load default image!\n" );
		success = 0;
	}
	gKeyPressTextures[ KEY_PRESS_SURFACE_RIGHT ] = loadTexture( "./pics/right.png" );
	if(	gKeyPressTextures[ KEY_PRESS_SURFACE_RIGHT ] == NULL ) {
		printf( "Failed to load default image!\n" );
		success = 0;
	}
	return success;
}*/
bool loadMedia()
{
	//Loading success flag 
	bool success = true;
	//Load Foo' texture
	if( !gFooTexture.loadFromFile( "./pics/dots.png" ) ){
		printf( "Failed to load Foo' texture image!\n" );
		success = false;
	} else{
		//Set top left sprite
		gSpriteClips[ 0 ].x = 0;
		gSpriteClips[ 0 ].y = 0;
		gSpriteClips[ 0 ].w = 100;
		gSpriteClips[ 0 ].h = 100;
		//Set top right sprite
		gSpriteClips[ 1 ].x = 100;
		gSpriteClips[ 1 ].y = 0;
		gSpriteClips[ 1 ].w = 100;
		gSpriteClips[ 1 ].h = 100;
		//Set bottom left sprite
		gSpriteClips[ 2 ].x = 0;
		gSpriteClips[ 2 ].y = 100;
		gSpriteClips[ 2 ].w = 100;
		gSpriteClips[ 2 ].h = 100;
		//Set bottom right sprite
		gSpriteClips[ 3 ].x = 100;
		gSpriteClips[ 3 ].y = 100;
		gSpriteClips[ 3 ].w = 100;
		gSpriteClips[ 3 ].h = 100;
	}
	return success;
}

void deinit() 
{
	gFooTexture.free();
	//gBackgroundTexture.free();

	//Destroy window
	SDL_DestroyWindow( gWindow );
	gWindow = NULL;

	//Quit SDL subsystems 
	IMG_Quit();
	SDL_Quit();
}

/************************** main func ****************************/
int main( int argc, char* args[] ) 
{
	int quit = 0;
	SDL_Event event;
	if(!init())
		printf( "Failed to initialize!\n" );
	else {
		if(!loadMedia())
			printf( "Failed to load media!\n" );

		while(quit == 0){
			while(SDL_PollEvent(&event) != 0){	//poll the event queue
				if(event.type == SDL_QUIT){
					printf( "Quit!\n" );
					quit = 1;
				}
				if(event.type == SDL_KEYDOWN){
					switch(event.key.keysym.sym){
						case SDLK_UP:
							break;
						case SDLK_DOWN:
							break;
						case SDLK_LEFT:
							break;
						case SDLK_RIGHT:
							break;
						default :
							break;
					}
					
				}
			}

			//Clear screen
			SDL_SetRenderDrawColor( gRenderer, 0xFF, 0xFF, 0xFF, 0xFF );
			SDL_RenderClear( gRenderer );
			//Render background texture to screen
			//gBackgroundTexture.render( 0, 0 );
			//Render Foo' to the screen
			gFooTexture.render( 0,								0,									&gSpriteClips[ 0 ]);
			gFooTexture.render( SCREEN_WIDTH-gSpriteClips[1].w,	0,									&gSpriteClips[ 1 ]);
			gFooTexture.render( 0,								SCREEN_HEIGHT-gSpriteClips[2].h,	&gSpriteClips[ 2 ]);
			gFooTexture.render( SCREEN_WIDTH-gSpriteClips[3].w, SCREEN_HEIGHT-gSpriteClips[3].h,	&gSpriteClips[ 3 ]);
			//Update screen
			SDL_RenderPresent( gRenderer );
		}
	}

	//Free resources and close SDL 
	deinit();
	return 0;
}
