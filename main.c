#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/XKBlib.h>
#include <X11/Xatom.h>

#include <time.h>
#include <stdio.h>
#include <stdlib.h>

//Window parameters
#define TITLE   "Skware"
#define POSY	500
#define POSX	500
#define WIDTH	1280
#define HEIGHT	720
#define BORDER	15

//Game Parameters
#define CYCLE_USEC	40000
#define CUBECOUNT_INIT 20
#define CUBECOUNT_INCREMENT 20
#define INCREMENT_CYCLES 500

//X11 variables
Display* dis;
int scr;
Window win;
GC gc;
XEvent ev;

//Game Variables
unsigned long black, white, red, green, blue;
typedef struct {int x, y, wait;} gameCube;
struct {double x;} gameCoord;
gameCube* gameCubes;
int gameCubeCount;
int gameCubeCountInterval;
int gameStage;
unsigned long floorCol;
XPoint* player;

//Game Functions
void window_start();
void window_close();
unsigned long rgb(int r, int g, int b);
void run();
int comp(gameCube* a, gameCube* b);


//entry function
int main() {
	srand(time(NULL)); //ensure psuedorandomness
	window_start(); //initialize window to be drawn on

	player = malloc(3*sizeof(XPoint));
	XPoint playerPoint;
	playerPoint.x=WIDTH/2;playerPoint.y=HEIGHT*0.9;
	player[0] = playerPoint;
	playerPoint.x=WIDTH/2-20;playerPoint.y=HEIGHT*0.9+20;
	player[1] = playerPoint;
	playerPoint.x=WIDTH/2+20;playerPoint.y=HEIGHT*0.9+20;
	player[2] = playerPoint;


	//start main game loop
	while (1) {

		//initialize or reinitialize game parameters
		gameStage = 1;
		gameCoord.x=WIDTH/2;
		gameCubeCount = CUBECOUNT_INIT;
		gameCubes = (gameCube*)malloc(gameCubeCount*sizeof(gameCube));
		gameCubeCountInterval = INCREMENT_CYCLES;
		floorCol = rgb(128, 128, 128);
		//add a variable amount of cubes to the cube array
		for (int i=0; i<gameCubeCount; i++) {
			gameCube cube;
			cube.x = gameCoord.x+(rand()%80)-40;
			cube.y = HEIGHT/8;
			cube.wait = rand()%(150-gameCubeCount);
			gameCubes[i] = cube;
		}
		
		//X11 file descriptor stuff that lets game run whislt still accepting inputs
		int x11_fd = ConnectionNumber(dis);
		fd_set in_fds;
		struct timeval interval;
		XFlush(dis);
		//start actual game
		while (gameStage == 1) {
			FD_ZERO(&in_fds);
			FD_SET(x11_fd, &in_fds);

			//define interval as 40000 microseconds
			interval.tv_usec = CYCLE_USEC;
			interval.tv_sec = 0;
			
			//runs an event check and game if there's an event and just the game if no event
			int num_ready_fds = select(x11_fd + 1, &in_fds, NULL, NULL, &interval);
			if (num_ready_fds > 0) {
				//define event actions for running game
				switch (ev.type) {
					case MotionNotify:
						player[0].x = WIDTH/2+(ev.xbutton.x-WIDTH/2)*0.1;
						break;
					case KeyPress:
						switch (XkbKeycodeToKeysym(dis, ev.xkey.keycode, 0, 0)) {
							case XK_q: //quit the program
								return 0;
						}
						break;
				}
				run();
				usleep(CYCLE_USEC);
			} else if (num_ready_fds == 0) {
				run();
			} 
			else {printf("An error occured!\n");}

			while(XPending(dis)) {
				XNextEvent(dis, &ev);
			}
		}
		//start game over loop
		while (gameStage == 2) {
			XNextEvent(dis, &ev);
			//define event actions for game over
			switch (ev.type) {
					case KeyPress:
						switch (XkbKeycodeToKeysym(dis, ev.xkey.keycode, 0, 0)) {
							case XK_q: //quit the program
								return 0;
							case XK_r: //restart the main game loop
								free(gameCubes);
								gameStage = 1;
								break;
						}
				}
		}
	}

	//clean up elemeents before quiting
	window_close();
    return (0);
}

//needed to sort and order the cube array
int comp(gameCube* a, gameCube* b) {
	return (a->y - b->y);
}
//one cycle run of the actual game
void run() {
	double move = (double)(ev.xbutton.x - WIDTH/2)/(WIDTH/4);
	if (move < -0.5) {move = -0.5;}
	if (move > 0.5) {move = 0.5;}
	gameCoord.x += move;
	
	//clear graphics
	XClearWindow(dis, win);
	//draw floor
	XSetForeground(dis, gc, floorCol);
	XFillRectangle(dis, win, gc, 0, HEIGHT/5, WIDTH, HEIGHT);
	//draw player
	XSetForeground(dis, gc, rgb(0, 0, 128));
	XFillPolygon(dis, win, gc, player, 3, Convex, CoordModeOrigin);

	
	//order the array of cubes so they draw on top of each other
	qsort(gameCubes, gameCubeCount, sizeof(gameCube), comp);
	//iterate through each cube in the field
	for (int i=0; i<gameCubeCount; i++) {
		//move cube and draw it depending on distance and relative location to player
		if (gameCubes[i].wait < 0) {
			gameCubes[i].y *= 1.05;
			double r = 2*(double)gameCubes[i].y/HEIGHT;
			if (r>1) {r=1;}
			XSetForeground(dis, gc, rgb((double)255*r, 128-(double)255*(r/2), 0));
			XFillRectangle(dis, win, gc, gameCubes[i].x+gameCubes[i].y*(gameCubes[i].x-gameCoord.x)*0.2 , gameCubes[i].y, gameCubes[i].y/2, gameCubes[i].y/2);
			//square is now close to player, check if lost or reset and randomize square
			if (gameCubes[i].y > HEIGHT*0.6) {
				if (fabs((double)gameCubes[i].x-gameCoord.x+1.2) < 1.4) {
					char msgFail[] = "GAME OVER";
					XDrawString(dis, win, gc, 10, 15, msgFail, strlen(msgFail));
					char msgQuit[] = "q - quit";
					XDrawString(dis, win, gc, 10, 25, msgQuit, strlen(msgQuit));
					char msgReplay[] = "r - replay";
					XDrawString(dis, win, gc, 10, 35, msgReplay, strlen(msgReplay));
					gameStage = 2;
				} else {
					gameCubes[i].x = gameCoord.x+(rand()%80)-40;
					gameCubes[i].y = HEIGHT/8;
					gameCubes[i].wait = rand()%(150-gameCubeCount);
				}
			}
		} else {
			//progress cube wait cycles
			gameCubes[i].wait--;
		}
	}

	//handles difficulty increase and stage advancement with a cycle countdown and increasing cube amount in field
	if (gameCubeCount < 100 && gameCubeCountInterval-- < 0) {
		floorCol = rgb(rand()%255, rand()%255, rand()%255);
		free(gameCubes);
		gameCubeCount += 20;
		gameCubes = (gameCube*)malloc(gameCubeCount*sizeof(gameCube));
		for (int i=0; i<gameCubeCount; i++) {
			gameCube cube;
			cube.x = gameCoord.x+(rand()%80)-40;
			cube.y = HEIGHT/8;
			cube.wait = rand()%(150-gameCubeCount);
			gameCubes[i] = cube;
		}
		gameCubeCountInterval = INCREMENT_CYCLES;
	}
}

//handles X11 window initiation
void window_start() {
	//open display
	dis = XOpenDisplay(NULL);
	if (dis == NULL) {
		printf(stderr, "Cannot open display\n");
		exit(1);
	}
	scr = DefaultScreen(dis);
	win = RootWindow(dis, scr);

	//shorthand for colors
	black = BlackPixel(dis, scr);
	white = WhitePixel(dis, scr);
	red = rgb(255, 0, 0);
	green = rgb(0, 255, 0);
	blue = rgb(0, 0, 255);

	//create window and event handlers
    win = XCreateSimpleWindow(dis, DefaultRootWindow(dis), POSX, POSY, WIDTH, HEIGHT, BORDER, white, black);
	XSetStandardProperties(dis, win, TITLE, TITLE, None, NULL, 0, NULL);
	XSelectInput(dis, win, KeyPressMask | PointerMotionMask);

	//create graphics context to draw on
	gc = XCreateGC(dis, win, 0, 0);
	XSetBackground(dis, gc, black);
	XSetForeground(dis, gc, white);

	//make windows floating on DWM
	XSizeHints xsh = {.min_width=WIDTH, .min_height=HEIGHT, .max_width=WIDTH, .max_height=HEIGHT};
	xsh.flags = PMinSize | PMaxSize;
	XSetSizeHints(dis, win, &xsh, XA_WM_NORMAL_HINTS);

	//map window to display
	XMapWindow(dis, win);
	XMapRaised(dis, win);
}
//handles closing the window and freeing memory
void window_close() {
	free(gameCubes);
	XFreeGC(dis, gc);
	XUnmapWindow(dis, win);
	XDestroyWindow(dis, win);
	XCloseDisplay(dis);
}

//spits out color that X11 needs
unsigned long rgb(int r, int g, int b) {
    return (b + (g<<8) + (r<<16));
}