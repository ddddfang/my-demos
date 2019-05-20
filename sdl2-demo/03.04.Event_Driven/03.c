//Hello SDL: Your First Graphics Window
//Using SDL and standard IO 
#include <SDL.h> 
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
		success = 1;
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
		}
	}
	return success;
}

SDL_Surface* loadSurface(char *path)
{
	//Load image at specified path 
	SDL_Surface* loadedSurface = SDL_LoadBMP( path );
	if( loadedSurface == NULL ) {
		printf( "Unable to load image %s! SDL Error: %s\n", path, SDL_GetError() );
	}
	return loadedSurface;
}

int loadMedia()
{
	int success = 1;
	gKeyPressSurfaces[ KEY_PRESS_SURFACE_DEFAULT ] = loadSurface( "./pics/press.bmp" );
	if(	gKeyPressSurfaces[ KEY_PRESS_SURFACE_DEFAULT ] == NULL ) {
		printf( "Failed to load default image!\n" );
		success = 0;
	}
	gKeyPressSurfaces[ KEY_PRESS_SURFACE_UP ] = loadSurface( "./pics/up.bmp" );
	if(	gKeyPressSurfaces[ KEY_PRESS_SURFACE_UP ] == NULL ) {
		printf( "Failed to load default image!\n" );
		success = 0;
	}
	gKeyPressSurfaces[ KEY_PRESS_SURFACE_DOWN ] = loadSurface( "./pics/down.bmp" );
	if(	gKeyPressSurfaces[ KEY_PRESS_SURFACE_DOWN ] == NULL ) {
		printf( "Failed to load default image!\n" );
		success = 0;
	}
	gKeyPressSurfaces[ KEY_PRESS_SURFACE_LEFT ] = loadSurface( "./pics/left.bmp" );
	if(	gKeyPressSurfaces[ KEY_PRESS_SURFACE_LEFT ] == NULL ) {
		printf( "Failed to load default image!\n" );
		success = 0;
	}
	gKeyPressSurfaces[ KEY_PRESS_SURFACE_RIGHT ] = loadSurface( "./pics/right.bmp" );
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
			SDL_BlitSurface( gCurrentSurface, NULL, gScreenSurface, NULL );
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
