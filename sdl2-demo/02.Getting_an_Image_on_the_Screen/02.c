//Hello SDL: Your First Graphics Window
//Using SDL and standard IO 
#include <SDL.h> 
#include <stdio.h> 

//Screen dimension constants 
const int SCREEN_WIDTH = 640; 
const int SCREEN_HEIGHT = 480;


/************************** some globals ****************************/
//The window we'll be rendering to 
SDL_Window* gWindow = NULL; 
//The surface contained by the window 
SDL_Surface* gScreenSurface = NULL; 
//The image we will load and show on the screen 
SDL_Surface* gHelloWorld = NULL;



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


int loadMedia()
{
    int success = 1;
    //Load splash image
    gHelloWorld = SDL_LoadBMP( "./hello_world.bmp" );
    if( gHelloWorld == NULL ) {
        printf( "Unable to load image %s! SDL Error: %s\n", "./hello_world.bmp", SDL_GetError() );
        success = 0;
    }
    return success;
}

void deinit() 
{
    //Deallocate surface
    SDL_FreeSurface( gHelloWorld );
    gHelloWorld = NULL;
    
    //Destroy window
    SDL_DestroyWindow( gWindow );
    gWindow = NULL;
    
    //Quit SDL subsystems 
    SDL_Quit();
}

/************************** main func ****************************/
int main( int argc, char* args[] ) 
{
    if(!init())
        printf( "Failed to initialize!\n" );
    else {
        if(!loadMedia())
            printf( "Failed to load media!\n" );
        else{
            //Apply the image
            SDL_BlitSurface( gHelloWorld, NULL, gScreenSurface, NULL );
            SDL_FillRect( gScreenSurface, NULL, SDL_MapRGB( gScreenSurface->format, 0xFF, 0, 0xFF ) );
            
            //Update the surface
            SDL_UpdateWindowSurface( gWindow );
            
            //Wait two seconds
            SDL_Delay( 2000 );
        }
    }
    
    //Free resources and close SDL 
    deinit();
    return 0;
}
