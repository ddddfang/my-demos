//SDL_image package: https://www.libsdl.org/projects/SDL_image/release/

//05_Optimized Surface Loading and Soft Stretching
//06_extension_libraries_and_loading_other_image_formats
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
//The surface contained by the window 
SDL_Surface* gScreenSurface = NULL;

//The images that correspond to a keypress
SDL_Surface* gKeyPressSurfaces[ KEY_PRESS_SURFACE_TOTAL ];
//Current displayed image 
SDL_Surface* gCurrentSurface = NULL;


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
			//Get window surface
			gScreenSurface = SDL_GetWindowSurface( gWindow );

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

SDL_Surface* loadSurface(char *path)
{
	//The final optimized image
	SDL_Surface* optimizedSurface = NULL;

	//Load image at specified path ,when you load a bitmap, 
	//it's typically loaded in a 24bit format since most bitmaps are 24bit.
	//SDL_Surface* loadedSurface = SDL_LoadBMP( path );	//use sdl API to load bmp picture
	SDL_Surface* loadedSurface = IMG_Load( path );	//use SDL_image API to load png picture
	if( loadedSurface == NULL ) {
		printf( "Unable to load image %s! SDL Error: %s\n", path, SDL_GetError() );
	} else {
		//Convert surface to screen format ,modern displays are not 24bit by default. 
		//If we blit an image that's 24bit onto a 32bit image, SDL will convert it 
		//every single time the image is blitted. so we convert them here.
		optimizedSurface = SDL_ConvertSurface( loadedSurface, gScreenSurface->format, 0 );
		if( optimizedSurface == NULL )
		{
			printf( "Unable to optimize image %s! SDL Error: %s\n", path, SDL_GetError() );
		}
		//Get rid of old loaded surface
		SDL_FreeSurface( loadedSurface );
	}
	return optimizedSurface;
}

int loadMedia()
{
	int success = 1;
	gKeyPressSurfaces[ KEY_PRESS_SURFACE_DEFAULT ] = loadSurface( "./pics/press.bmp" );
	if(	gKeyPressSurfaces[ KEY_PRESS_SURFACE_DEFAULT ] == NULL ) {
		printf( "Failed to load default image!\n" );
		success = 0;
	}
	//gKeyPressSurfaces[ KEY_PRESS_SURFACE_UP ] = loadSurface( "./pics/up.bmp" );
	gKeyPressSurfaces[ KEY_PRESS_SURFACE_UP ] = loadSurface( "./pics/up.png" );
	if(	gKeyPressSurfaces[ KEY_PRESS_SURFACE_UP ] == NULL ) {
		printf( "Failed to load default image!\n" );
		success = 0;
	}
	//gKeyPressSurfaces[ KEY_PRESS_SURFACE_DOWN ] = loadSurface( "./pics/down.bmp" );
	gKeyPressSurfaces[ KEY_PRESS_SURFACE_DOWN ] = loadSurface( "./pics/down.png" );
	if(	gKeyPressSurfaces[ KEY_PRESS_SURFACE_DOWN ] == NULL ) {
		printf( "Failed to load default image!\n" );
		success = 0;
	}
	//gKeyPressSurfaces[ KEY_PRESS_SURFACE_LEFT ] = loadSurface( "./pics/left.bmp" );
	gKeyPressSurfaces[ KEY_PRESS_SURFACE_LEFT ] = loadSurface( "./pics/left.png" );
	if(	gKeyPressSurfaces[ KEY_PRESS_SURFACE_LEFT ] == NULL ) {
		printf( "Failed to load default image!\n" );
		success = 0;
	}
	//gKeyPressSurfaces[ KEY_PRESS_SURFACE_RIGHT ] = loadSurface( "./pics/right.bmp" );
	gKeyPressSurfaces[ KEY_PRESS_SURFACE_RIGHT ] = loadSurface( "./pics/right.png" );
	if(	gKeyPressSurfaces[ KEY_PRESS_SURFACE_RIGHT ] == NULL ) {
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
		SDL_FreeSurface( gKeyPressSurfaces[i] );
		gKeyPressSurfaces[i] = NULL;
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
		gCurrentSurface = gKeyPressSurfaces[ KEY_PRESS_SURFACE_DEFAULT ];
		while(quit == 0){
			while(SDL_PollEvent(&event) != 0){	//poll the event queue
				if(event.type == SDL_QUIT){
					printf( "Quit!\n" );
					quit = 1;
				}
				if(event.type == SDL_KEYDOWN){
					switch(event.key.keysym.sym){
						case SDLK_UP:
							gCurrentSurface = gKeyPressSurfaces[ KEY_PRESS_SURFACE_UP ];
							break;
						case SDLK_DOWN:
							gCurrentSurface = gKeyPressSurfaces[ KEY_PRESS_SURFACE_DOWN ];
							break;
						case SDLK_LEFT:
							gCurrentSurface = gKeyPressSurfaces[ KEY_PRESS_SURFACE_LEFT ];
							break;
						case SDLK_RIGHT:
							gCurrentSurface = gKeyPressSurfaces[ KEY_PRESS_SURFACE_RIGHT ];
							break;
						default :
							gCurrentSurface = gKeyPressSurfaces[ KEY_PRESS_SURFACE_DEFAULT ];
							break;
					}
				}
			}
			//Apply the image
			//SDL_BlitSurface( gCurrentSurface, NULL, gScreenSurface, NULL );
			SDL_Rect stretchRect;
			stretchRect.x = 0;
			stretchRect.y = 0;
			stretchRect.w = SCREEN_WIDTH;
			stretchRect.h = SCREEN_HEIGHT;
			SDL_BlitScaled( gCurrentSurface, NULL, gScreenSurface, &stretchRect );

			//Update the surface
			SDL_UpdateWindowSurface( gWindow );
			//Wait 50ms
			SDL_Delay( 50 );

			//SDL_FillRect( gScreenSurface, NULL, SDL_MapRGB( gScreenSurface->format, 0xFF, 0, 0xFF ) );
			//SDL_UpdateWindowSurface( gWindow );
			//SDL_Delay( 500 );
		}
	}

	//Free resources and close SDL 
	deinit();
	return 0;
}
