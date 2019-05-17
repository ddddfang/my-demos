//Hello SDL: Your First Graphics Window
//Using SDL and standard IO 
#include <SDL.h> 
#include <stdio.h> 

//Screen dimension constants 
#define SCREEN_WIDTH 640 
#define SCREEN_HEIGHT 480

#define LOAD_BGRA    1
#define LOAD_RGB24   0
#define LOAD_BGR24   0
#define LOAD_YUV420P 0
/************************** some globals ****************************/
int thread_exit=0;
//The window we'll be rendering to 
SDL_Window* gWindow = NULL; 
//The surface contained by the window 
SDL_Surface* gScreenSurface = NULL; 
//The image we will load and show on the screen 
SDL_Surface* gHelloWorld = NULL;
//render for window
SDL_Renderer* gRender = NULL;
SDL_Texture* gTexture = NULL;

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
                                SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE );//SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE
        if( gWindow == NULL ) {
            printf( "Window could not be created! SDL_Error: %s\n", SDL_GetError() );
            success = 0;
        } else {
            SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1"); //better performance??
            //Get window surface
            gScreenSurface = SDL_GetWindowSurface( gWindow );
            gRender = SDL_CreateRenderer(gWindow, -1, 0);
            gTexture = SDL_CreateTexture(gRender, SDL_PIXELFORMAT_YV12, SDL_TEXTUREACCESS_STREAMING, SCREEN_WIDTH, SCREEN_HEIGHT);
        }
    }
    return success;
}

//send refresh event every 40ms
#define REFRESH_EVENT  (SDL_USEREVENT + 1)
int refresh_video_event_sender(void *opaque)
{
    while (thread_exit==0) {
        SDL_Event event;
        event.type = REFRESH_EVENT;
        SDL_PushEvent(&event);
        SDL_Delay(40);
    }
    return 0;
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

//create dummy YUV420P
static char YUV[SCREEN_WIDTH*SCREEN_HEIGHT * 2];
void FillYuvImage(char* pYuv, int nWidth, int nHeight, int nIndex)
{
	int x, y, i;
 
	i = nIndex;
 
	char* pY = pYuv;			//start = 0, 			len = nWidth * nHeight
	char* pU = pYuv + nWidth * nHeight;	//start = nWidth * nHeight, 	len = 0.5*nWidth * 0.5*nHeight
	char* pV = pYuv + nWidth * nHeight * 5 / 4;
 
	/* Y */
	for (y = 0; y < nHeight; y++)
	{
		for (x = 0; x < nWidth; x++)
		{
			pY[y * nWidth + x] = x + y + i*3;
		}
	}
 
	/* Cb and Cr */
	for (y = 0; y < nHeight / 2; y++)
	{
		for (x = 0; x < nWidth / 2; x++)
		{
			pU[y * (nWidth/2) + x] = 128 + y + i * 2;
			pV[y * (nWidth/2) + x] = 64 + x + i * 5;
		}
	}
}

void deinit() 
{
    SDL_DestroyTexture(gTexture); gTexture = NULL;
    SDL_DestroyRenderer(gRender); gRender = NULL;
    //Deallocate surface
    SDL_FreeSurface( gHelloWorld );gHelloWorld = NULL;
    //Destroy window
    SDL_DestroyWindow( gWindow );gWindow = NULL;
    
    //Quit SDL subsystems 
    SDL_Quit();
}

/************************** main func ****************************/
int main( int argc, char* args[] ) 
{
    if(!init())
        printf( "Failed to initialize!\n" );
    else {
        SDL_Thread *refresh_thread = SDL_CreateThread(refresh_video_event_sender,NULL,NULL);
        SDL_Event event;
        char* pY = YUV;
        char* pU = YUV + SCREEN_WIDTH*SCREEN_HEIGHT;
        char* pV = YUV + SCREEN_WIDTH*SCREEN_HEIGHT * 5 / 4;
        int index = 0;
        while(thread_exit == 0){
            SDL_WaitEvent(&event);
            if(event.type == REFRESH_EVENT){
                FillYuvImage(YUV, SCREEN_WIDTH, SCREEN_HEIGHT, index++);
                int e = SDL_UpdateYUVTexture(gTexture, NULL, pY, SCREEN_WIDTH, pU, SCREEN_WIDTH/2, pV, SCREEN_WIDTH/2);

                //SDL_RenderClear(gRender);
                SDL_RenderCopy(gRender, gTexture, NULL, NULL);
                SDL_RenderPresent(gRender);
            }
            if(event.type == SDL_WINDOWEVENT){	//resize
                int screen_w,screen_h;
                SDL_GetWindowSize(gWindow,&screen_w,&screen_h);
                printf( "window resized to %d*%d.\n",screen_w,screen_h);
            }
            if(event.type == SDL_QUIT){
                printf( "quit.\n");
                thread_exit = 1;
                break;
            }
        }
    }
    
    //Free resources and close SDL 
    deinit();
    return 0;
}
