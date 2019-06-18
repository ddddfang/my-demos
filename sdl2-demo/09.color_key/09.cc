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
//#include <string>
#include <memory> //shared_ptr
#include "LTexture.hpp"

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


/************************** some globals ****************************/
//The window we'll be rendering to 
SDL_Window* gWindow = NULL;
SDL_Renderer *gRenderer = NULL;

//use shared_ptr to manage class. free resources in de-constructor !
shared_ptr<LTexture> gKeyPressTextures[ KEY_PRESS_SURFACE_TOTAL ];
shared_ptr<LTexture> gFooTexture;
shared_ptr<LTexture> gBackgroundTexture;
shared_ptr<LTexture> gDotsTexture;
SDL_Rect gSpriteClips[ 4 ];

/************************** util func ****************************/
bool init()
{
	bool success = true; 

	//Initialize SDL 
	if( SDL_Init( SDL_INIT_VIDEO ) < 0 ) {
		printf( "SDL could not initialize! SDL_Error: %s\n", SDL_GetError() );
		success = false;
	} else { 
		//Create window 
		gWindow = SDL_CreateWindow( "SDL Tutorial", 
						SDL_WINDOWPOS_UNDEFINED,
						SDL_WINDOWPOS_UNDEFINED,
						SCREEN_WIDTH, SCREEN_HEIGHT,
						SDL_WINDOW_SHOWN );
		if( gWindow == NULL ) {
			printf( "Window could not be created! SDL_Error: %s\n", SDL_GetError() );
			success = false;
		} else {
			//create renderer for window
			gRenderer = SDL_CreateRenderer(gWindow, -1, SDL_RENDERER_ACCELERATED);
			if(gRenderer == NULL){
				printf( "Renderer could not be created! SDL Error: %s\n", SDL_GetError() );
				success = false;
			} else{
				SDL_SetRenderDrawColor(gRenderer, 0xff, 0xff, 0xff, 0xff);
			}

			//Initialize PNG loading , will use SDL_image library
			int imgFlags = IMG_INIT_PNG;
			if( !( IMG_Init( imgFlags ) & imgFlags ) ) {
				printf( "SDL_image could not initialize! SDL_image Error: %s\n", IMG_GetError());
				success = false;
			}
		}

		//create LTexture.
		int i;
		for(i=0;i<KEY_PRESS_SURFACE_TOTAL;i++){
			gKeyPressTextures[i].reset(new LTexture());
		}
		gFooTexture.reset(new LTexture());
		gBackgroundTexture.reset(new LTexture());
		gDotsTexture.reset(new LTexture());
	}
	return success;
}

bool loadMedia()
{
	bool success = true;
	if( !gKeyPressTextures[KEY_PRESS_SURFACE_DEFAULT]->loadFromFile( gRenderer, "./pics/press.bmp" ) ){
		printf( "Failed to load default image!\n" );
		success = false;
	}
	if( !gKeyPressTextures[KEY_PRESS_SURFACE_UP]->loadFromFile( gRenderer, "./pics/up.png" ) ){
		printf( "Failed to load up image!\n" );
		success = false;
	}
	if( !gKeyPressTextures[KEY_PRESS_SURFACE_DOWN]->loadFromFile( gRenderer, "./pics/down.png" ) ){
		printf( "Failed to load down image!\n" );
		success = false;
	}
	if( !gKeyPressTextures[KEY_PRESS_SURFACE_LEFT]->loadFromFile( gRenderer, "./pics/left.png" ) ){
		printf( "Failed to load left image!\n" );
		success = false;
	}
	if( !gKeyPressTextures[KEY_PRESS_SURFACE_RIGHT]->loadFromFile( gRenderer, "./pics/right.png" ) ){
		printf( "Failed to load right image!\n" );
		success = false;
	}
	if( !gFooTexture->loadFromFile( gRenderer, "./pics/foo.png" ) ){
		printf( "Failed to load foo image!\n" );
		success = false;
	}
	if( !gBackgroundTexture->loadFromFile( gRenderer, "./pics/background.png" ) ){
		printf( "Failed to load background image!\n" );
		success = false;
	}
	if( !gDotsTexture->loadFromFile( gRenderer, "./pics/dots.png" ) ){
		printf( "Failed to load dots image!\n" );
		success = false;
	} else {
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
	//free LTexture.
	gDotsTexture = nullptr;
	gFooTexture = nullptr;
	gBackgroundTexture = nullptr;
	int i;
	for(i=0;i<KEY_PRESS_SURFACE_TOTAL;i++){
		gKeyPressTextures[i] = nullptr;
	}

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
	bool quit = false;
	SDL_Event event;
	shared_ptr<LTexture> targetTexture;
	if(!init())
		printf( "Failed to initialize!\n" );
	else {
		if(!loadMedia())
			printf( "Failed to load media!\n" );
#if 0	//up down left right DEMO
		targetTexture = gKeyPressTextures[KEY_PRESS_SURFACE_DEFAULT];
		while(quit == false){
			while(SDL_PollEvent(&event) != 0){	//poll the event queue
				if(event.type == SDL_QUIT){
					printf( "Quit!\n" );
					quit = true;
				}
				if(event.type == SDL_KEYDOWN){
					switch(event.key.keysym.sym){
						case SDLK_UP:
							targetTexture = gKeyPressTextures[KEY_PRESS_SURFACE_UP];
							break;
						case SDLK_DOWN:
							targetTexture = gKeyPressTextures[KEY_PRESS_SURFACE_DOWN];
							break;
						case SDLK_LEFT:
							targetTexture = gKeyPressTextures[KEY_PRESS_SURFACE_LEFT];
							break;
						case SDLK_RIGHT:
							targetTexture = gKeyPressTextures[KEY_PRESS_SURFACE_RIGHT];
							break;
						default :
							targetTexture = gKeyPressTextures[KEY_PRESS_SURFACE_DEFAULT];
							break;
					}
					
				}
			}

			//Clear screen
			SDL_SetRenderDrawColor( gRenderer, 0xFF, 0xFF, 0xFF, 0xFF );
			SDL_RenderClear( gRenderer );
			targetTexture->render( gRenderer,0,0,NULL);
			//Update screen
			SDL_RenderPresent( gRenderer );
		}
#endif
#if 0	//color key DEMO
		while(quit == false){
			while(SDL_PollEvent(&event) != 0){	//poll the event queue
				if(event.type == SDL_QUIT){
					printf( "Quit!\n" );
					quit = true;
				}
			}
			//Clear screen
			SDL_SetRenderDrawColor( gRenderer, 0xFF, 0xFF, 0xFF, 0xFF );
			SDL_RenderClear( gRenderer );
			gBackgroundTexture->render( gRenderer,0,0,NULL);
			gFooTexture->render( gRenderer,240,190,NULL);
			//Update screen
			SDL_RenderPresent( gRenderer );
		}
#endif
		while(quit == false){
			while(SDL_PollEvent(&event) != 0){	//poll the event queue
				if(event.type == SDL_QUIT){
					printf( "Quit!\n" );
					quit = true;
				}
			}
			//Clear screen
			SDL_SetRenderDrawColor( gRenderer, 0xFF, 0xFF, 0xFF, 0xFF );
			SDL_RenderClear( gRenderer ); 

			//Render top left sprite
			gDotsTexture->render( gRenderer,0,0,&gSpriteClips[0]);
			//Render top right sprite
			gDotsTexture->render( gRenderer,SCREEN_WIDTH-gSpriteClips[1].w, 0, &gSpriteClips[1]);
			//Render bottom left sprite
			gDotsTexture->render( gRenderer,0,SCREEN_HEIGHT-gSpriteClips[2].h, &gSpriteClips[2]);
			//Render bottom right sprite
			gDotsTexture->render( gRenderer,SCREEN_WIDTH-gSpriteClips[3].w, SCREEN_HEIGHT-gSpriteClips[3].h, &gSpriteClips[3]);
			//Update screen
			SDL_RenderPresent( gRenderer );
		}
	}

	//Free resources and close SDL 
	deinit();
	return 0;
}
