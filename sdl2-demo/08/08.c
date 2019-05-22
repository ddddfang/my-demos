//SDL_image package: https://www.libsdl.org/projects/SDL_image/release/
//http://lazyfoo.net/tutorials/SDL/08_geometry_rendering/index.php
//http://lazyfoo.net/tutorials/SDL/09_the_viewport/index.php

//use SDL texture and renderer to 
//	1.load images 
//	2.render rect/line/point
//	3.set viewport

//Using SDL and standard IO 
#include <SDL.h>
#include <SDL_image.h>
#include <stdio.h>

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
SDL_Texture *gKeyPressTextures[ KEY_PRESS_SURFACE_TOTAL ];
SDL_Texture *gTexture = NULL;

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

SDL_Texture* loadTexture(char *path)
{
	SDL_Texture *newTexture = NULL;
	SDL_Surface* loadedSurface = IMG_Load( path );	//use SDL_image API to load png picture
	if( loadedSurface == NULL ) {
		printf( "Unable to load image %s! SDL Error: %s\n", path, SDL_GetError() );
	} else {
		newTexture = SDL_CreateTextureFromSurface(gRenderer, loadedSurface);
		if(newTexture == NULL){
			printf( "Unable to create texture from %s! SDL Error: %s\n", path, SDL_GetError() );
		}
		//Get rid of old loaded surface
		SDL_FreeSurface( loadedSurface );
	}
	return newTexture;
}

int loadMedia()
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
}

void deinit() 
{
	//Deallocate surface
	int i=0;
	for(i=0;i<KEY_PRESS_SURFACE_TOTAL;i++){
		SDL_DestroyTexture( gKeyPressTextures[i] );
		gKeyPressTextures[i] = NULL;
	}

	//Destroy window
	SDL_DestroyWindow( gWindow );
	gWindow = NULL;

	//Quit SDL subsystems 
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

		gTexture = gKeyPressTextures[ KEY_PRESS_SURFACE_DEFAULT ];
		while(quit == 0){
			while(SDL_PollEvent(&event) != 0){	//poll the event queue
				if(event.type == SDL_QUIT){
					printf( "Quit!\n" );
					quit = 1;
				}
				if(event.type == SDL_KEYDOWN){
					switch(event.key.keysym.sym){
						case SDLK_UP:
							gTexture = gKeyPressTextures[ KEY_PRESS_SURFACE_UP ];
							break;
						case SDLK_DOWN:
							gTexture = gKeyPressTextures[ KEY_PRESS_SURFACE_DOWN ];
							break;
						case SDLK_LEFT:
							gTexture = gKeyPressTextures[ KEY_PRESS_SURFACE_LEFT ];
							break;
						case SDLK_RIGHT:
							gTexture = gKeyPressTextures[ KEY_PRESS_SURFACE_RIGHT ];
							break;
						default :
							gTexture = gKeyPressTextures[ KEY_PRESS_SURFACE_DEFAULT ];
							break;
					}
					
				}
			}
			//-----------Clear screen with white
			SDL_SetRenderDrawColor( gRenderer, 0xFF, 0xFF, 0xFF, 0xFF );
			SDL_RenderClear( gRenderer );

			//-----------Top left corner viewport
			SDL_Rect topLeftViewport;
			topLeftViewport.x = 0;
			topLeftViewport.y = 0;
			topLeftViewport.w = SCREEN_WIDTH / 2;
			topLeftViewport.h = SCREEN_HEIGHT / 2;
			SDL_RenderSetViewport( gRenderer, &topLeftViewport );

			SDL_RenderCopy(gRenderer, gTexture, NULL, NULL);	//renderer will only render the top-left corner part ,texture will copy to there, too(pic will be scaled)
			//SDL_RenderPresent( gRenderer );

			//-----------bottom right corner viewport
			SDL_Rect bottomRightViewport;
			bottomRightViewport.x = SCREEN_WIDTH / 2;
			bottomRightViewport.y = SCREEN_HEIGHT / 2;
			bottomRightViewport.w = SCREEN_WIDTH / 2;
			bottomRightViewport.h = SCREEN_HEIGHT / 2;
			SDL_RenderSetViewport( gRenderer, &bottomRightViewport );

			//Render red filled quad
			SDL_Rect fillRect = { bottomRightViewport.w / 4, bottomRightViewport.h / 4, bottomRightViewport.w / 2, bottomRightViewport.h / 2 };
			SDL_SetRenderDrawColor( gRenderer, 0xFF, 0x00, 0x00, 0xFF );
			SDL_RenderFillRect( gRenderer, &fillRect );

			//Render green outlined quad
			SDL_Rect outlineRect = { bottomRightViewport.w / 6, bottomRightViewport.h / 6, bottomRightViewport.w * 2 / 3, bottomRightViewport.h * 2 / 3 };
			SDL_SetRenderDrawColor( gRenderer, 0x00, 0xFF, 0x00, 0xFF );
			SDL_RenderDrawRect( gRenderer, &outlineRect );

			//Draw blue horizontal line
			SDL_SetRenderDrawColor( gRenderer, 0x00, 0x00, 0xFF, 0xFF ); 
			SDL_RenderDrawLine( gRenderer, 0, bottomRightViewport.h / 2, bottomRightViewport.w, bottomRightViewport.h / 2 );

			//Draw vertical line of yellow dots
			SDL_SetRenderDrawColor( gRenderer, 0xFF, 0xFF, 0x00, 0xFF );
			for( int i = 0; i < bottomRightViewport.h; i += 4 ){
				SDL_RenderDrawPoint( gRenderer, bottomRightViewport.w / 2, i );
			}
			//-----------Update screen
			SDL_RenderPresent( gRenderer );
		}
	}

	//Free resources and close SDL 
	deinit();
	return 0;
}
