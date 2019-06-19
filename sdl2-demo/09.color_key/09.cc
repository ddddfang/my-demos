//SDL_image package: https://www.libsdl.org/projects/SDL_image/release/
//http://lazyfoo.net/tutorials/SDL/10_color_keying/index.php

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
shared_ptr<LTexture> gModulatedTexture;
shared_ptr<LTexture> gFadeModulatedTexture;
shared_ptr<LTexture> gFadeBackgroundTexture;
SDL_Rect gSpriteClips[ 4 ];
shared_ptr<LTexture> gSpriteSheetWalkTexture;
const int WALKING_ANIMATION_FRAMES = 4; 
SDL_Rect gSpriteWalkClips[ WALKING_ANIMATION_FRAMES ];

/************************** util func ****************************/
bool init()
{
	bool success = true; 

	//Initialize SDL 
	if( SDL_Init( SDL_INIT_VIDEO ) < 0 ) {
		printf( "SDL could not initialize! SDL_Error: %s\n", SDL_GetError() );
		success = false;
	} else {
		//Set texture filtering to linear
		if( !SDL_SetHint( SDL_HINT_RENDER_SCALE_QUALITY, "1" ) ) {
			printf( "Warning: Linear texture filtering not enabled!" );
		}
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
			//create renderer for window.VSync allows the rendering to update at the same time 
			//as when your monitor updates during vertical refresh. most monitors run at 60fps
			//gRenderer = SDL_CreateRenderer(gWindow, -1, SDL_RENDERER_ACCELERATED);
			gRenderer = SDL_CreateRenderer(gWindow, -1, SDL_RENDERER_ACCELERATED|SDL_RENDERER_PRESENTVSYNC);
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
		gModulatedTexture.reset(new LTexture());
		gFadeModulatedTexture.reset(new LTexture());
		gFadeBackgroundTexture.reset(new LTexture());
		gSpriteSheetWalkTexture.reset(new LTexture());
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
	//----------
	if( !gFooTexture->loadFromFile( gRenderer, "./pics/foo.png" ) ){
		printf( "Failed to load foo image!\n" );
		success = false;
	}
	if( !gBackgroundTexture->loadFromFile( gRenderer, "./pics/background.png" ) ){
		printf( "Failed to load background image!\n" );
		success = false;
	}
	//----------
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
	//----------
	if( !gModulatedTexture->loadFromFile( gRenderer, "./pics/colors.png" ) ){
		printf( "Failed to load colors image!\n" );
		success = false;
	}
	//----------
	//Load front alpha texture, fang: after fade out( with alpha), background will show ??
	if( !gFadeModulatedTexture->loadFromFile( gRenderer, "./pics/fadeout_front.png" ) )
	{
		printf( "Failed to load front texture!\n" );
		success = false;
	}
	else
	{
		//Set standard alpha blending
		gFadeModulatedTexture->setBlendMode( SDL_BLENDMODE_BLEND );
	}

	//Load background texture
	if( !gFadeBackgroundTexture->loadFromFile( gRenderer, "./pics/fadein_back.png" ) )
	{
		printf( "Failed to load background texture!\n" );
		success = false;
	}
	//----------
	//load walk texture
	if( !gSpriteSheetWalkTexture->loadFromFile( gRenderer, "./pics/walk.png" ) )
	{
		printf( "Failed to load background texture!\n" );
		success = false;
	} else {
		gSpriteWalkClips[0].x = 0;		//
		gSpriteWalkClips[0].y = 0;
		gSpriteWalkClips[0].w = 64;
		gSpriteWalkClips[0].h = 205;
		gSpriteWalkClips[1].x = 64;		//
		gSpriteWalkClips[1].y = 0;
		gSpriteWalkClips[1].w = 64;
		gSpriteWalkClips[1].h = 205;	//
		gSpriteWalkClips[2].x = 128;
		gSpriteWalkClips[2].y = 0;
		gSpriteWalkClips[2].w = 64;
		gSpriteWalkClips[2].h = 205;	//
		gSpriteWalkClips[3].x = 196;
		gSpriteWalkClips[3].y = 0;
		gSpriteWalkClips[3].w = 64;
		gSpriteWalkClips[3].h = 205;
	}
	return success;
}

void deinit() 
{
	//free LTexture.
	gSpriteSheetWalkTexture = nullptr;
	gFadeBackgroundTexture = nullptr;
	gFadeModulatedTexture = nullptr;
	gModulatedTexture = nullptr;
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
#if 0	//sprite_sheet DEMO
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
#endif
#if 0	//color modulation DEMO
		//Modulation components
		Uint8 r = 128;
		Uint8 g = 128;
		Uint8 b = 128;
		while(quit == false){
			while(SDL_PollEvent(&event) != 0){	//poll the event queue
				if(event.type == SDL_QUIT){
					printf( "Quit!\n" );
					quit = true;
				}
				if(event.type == SDL_KEYDOWN){
					switch(event.key.keysym.sym){
						//increase red.
						case SDLK_q:
							r += 32;
							break;
						case SDLK_a:
							r -= 32;
							break;
						//increase green
						case SDLK_w:
							g += 32;
							break;
						case SDLK_s:
							g -= 32;
							break;
						//increase blue
						case SDLK_e:
							b += 32;
							break;
						case SDLK_d:
							b -= 32;
							break;
						default :
							break;
					}
				}
			}

			//Clear screen
			SDL_SetRenderDrawColor( gRenderer, 0xFF, 0xFF, 0xFF, 0xFF );
			SDL_RenderClear( gRenderer );
			//gModulatedTexture->setColor(r,g,b);
			//gModulatedTexture->render( gRenderer,0,0);
			gBackgroundTexture->setColor(r,g,b);
			gBackgroundTexture->render( gRenderer,0,0);
			//Update screen
			SDL_RenderPresent( gRenderer );
		}
#endif
#if 0	//use alpha to fade DEMO
		Uint8 a = 255;
		while(quit == false){
			while(SDL_PollEvent(&event) != 0){	//poll the event queue
				if(event.type == SDL_QUIT){
					printf( "Quit!\n" );
					quit = true;
				}
				if(event.type == SDL_KEYDOWN){
					switch(event.key.keysym.sym){
						case SDLK_w:
							a = ((a+32)>255)?255:(a+32);
							break;
						case SDLK_s:
							a = ((a-32)<0)?0:(a-32);
							break;
						default :
							break;
					}
				}
			}

			//Clear screen
			SDL_SetRenderDrawColor( gRenderer, 0xFF, 0xFF, 0xFF, 0xFF );
			SDL_RenderClear( gRenderer );
			gFadeBackgroundTexture->render( gRenderer,0,0);
			//render front blended, so it will cover background, for now.
			gFadeModulatedTexture->setAlpha(a);
			gFadeModulatedTexture->render( gRenderer,0,0);
			//Update screen
			SDL_RenderPresent( gRenderer );
		}
#endif
		int frame = 0;
		int clipindex = 0;
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
				SDL_Rect* currentClip = &gSpriteWalkClips[ clipindex ];
				gSpriteSheetWalkTexture->render(	gRenderer, 
													( SCREEN_WIDTH - currentClip->w ) / 2,
													( SCREEN_HEIGHT - currentClip->h ) / 2,
													currentClip );
			//Update screen
			SDL_RenderPresent( gRenderer );
			frame++;
			clipindex = (frame/6)%WALKING_ANIMATION_FRAMES;//60fps/6 = 10fps
		}
	}

	//Free resources and close SDL 
	deinit();
	return 0;
}
