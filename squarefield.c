/*
Couldn't figure out how to get arrow key inputs to work without weird behavior
Until then, the game uses mouse position to move the blue triangle
*/

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/XKBlib.h>
#include <X11/Xatom.h>

#include <math.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>

//Window parameters
#define TITLE   "Squarefield"
#define POSY	500
#define POSX	500
#define WIDTH	1280
#define HEIGHT	720
#define BORDER	15

//Game Parameters
#define CYCLE_USEC			33333
#define CUBECOUNT_INIT 		20
#define CUBESPREAD			40
#define CUBECOUNT_INCREMENT	20
#define INCREMENT_CYCLES 	500
#define PI 3.142857

//X11 objects used by the program
Display* dis;
int scr;
Window win;
GC gc;
XEvent ev;
int width;
int height;

//Game paremeters initialized for the game program
unsigned long black, white, red, green, blue;
struct {double x, angle;} gameCoord; //game's absolute coordinates
typedef struct {double x, y; int wait;} gameCube; //individual cube
gameCube* gameCubes; //collection of individual cubes
int gameCubeCount;
int gameCubeCountInterval;
int gameStage;
int gameScore;
unsigned long floorColor;
XPoint polygonPoint; //used in order to initialize or modify the XPoint array polygons
XPoint* polygonPlayer;
XPoint* polygonFloor;
XPoint* polygonSquare;

//Functions initialized to be used by the program
void window_start();
void window_close();
unsigned long rgb(int r, int g, int b);
void run();
int comp(gameCube* a, gameCube* b);


//entry function
int main() {
	srand(time(NULL)); //ensure psuedorandomness
	window_start(); //initialize window to be drawn on

	//set polygonPlayer shape as triangle
	polygonPlayer = malloc(3*sizeof(XPoint));
	polygonPoint.x=width/2;polygonPoint.y=height*0.9;
	polygonPlayer[0] = polygonPoint;
	polygonPoint.x=width/2-20;polygonPoint.y=height*0.9+20;
	polygonPlayer[1] = polygonPoint;
	polygonPoint.x=width/2+20;polygonPoint.y=height*0.9+20;
	polygonPlayer[2] = polygonPoint;

	//initialize floor shape
	polygonFloor = malloc(4*sizeof(XPoint));
	polygonPoint.x=0; polygonPoint.y=height/5;
	polygonFloor[0] = polygonPoint;
	polygonPoint.x=width; polygonPoint.y=height/5;
	polygonFloor[1] = polygonPoint;
	polygonPoint.x=width; polygonPoint.y=height;
	polygonFloor[2] = polygonPoint;
	polygonPoint.x=0; polygonPoint.y=height;
	polygonFloor[3] = polygonPoint;

	//define square shape to be 4 XPoints
	polygonSquare = malloc(4*sizeof(XPoint));

	//start main game loop
	while (1) {

		//initialize or reinitialize game parameters
		gameStage = 1;
		gameScore = 0;
		gameCoord.x = 0;
		gameCubeCount = CUBECOUNT_INIT;
		gameCubes = (gameCube*)malloc(gameCubeCount*sizeof(gameCube));
		gameCubeCountInterval = INCREMENT_CYCLES;
		floorColor = rgb(128, 128, 128);
		//add a variable amount of cubes to the cube array
		for (int i=0; i<gameCubeCount; i++) {
			gameCube cube;
			cube.x = gameCoord.x+(rand()%(CUBESPREAD*2))-CUBESPREAD;
			cube.y = height/8;
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
						gameCoord.angle = (ev.xbutton.x-width/2)*(PI/(4*width)); //changes the view angle depending on mouse position
						break;
					case KeyPress:
						switch (XkbKeycodeToKeysym(dis, ev.xkey.keycode, 0, 0)) {
							case XK_q: //cleanup and quit the program
								window_close();
								return 0;
							case XK_Left:
								gameCoord.x -= width/4;
								break;
							case XK_Right:
								gameCoord.x += width/4;
								break;
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
					case MotionNotify:
						gameCoord.angle = (ev.xbutton.x-width/2)*(PI/(4*width)); //changes the view angle depending on mouse position
						break;
					case KeyPress:
						switch (XkbKeycodeToKeysym(dis, ev.xkey.keycode, 0, 0)) {
							case XK_q: //cleanup and quit the program
								window_close();
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
    return 0;
}

//needed to sort and order the cube array
int comp(gameCube* a, gameCube* b) {
	return (a->y - b->y);
}
//one cycle run of the actual game
void run() {
	//update the absolute coordinates based on cursor position
	double move = (double)(ev.xbutton.x - width/2)/(width/4);
	if (move < -0.5) {move = -0.5;}
	if (move > 0.5) {move = 0.5;}
	gameCoord.x += move;
	
	//clear graphics
	XClearWindow(dis, win);

	//draw floor depending on gameCoord angle attribute
	polygonPoint.x=0, polygonPoint.y=width/5+sin(gameCoord.angle)*(width/2);
	polygonFloor[0] = polygonPoint;
	polygonPoint.x=width, polygonPoint.y=width/5-sin(gameCoord.angle)*(width/2);
	polygonFloor[1] = polygonPoint;
	XSetForeground(dis, gc, floorColor);
	XFillPolygon(dis, win, gc, polygonFloor, 4, Convex, CoordModeOrigin);

	//draw player triangle
	XSetForeground(dis, gc, rgb(0, 0, 128));
	XFillPolygon(dis, win, gc, polygonPlayer, 3, Convex, CoordModeOrigin);

	//draw score on upper left corner
	XSetForeground(dis, gc, white);
	char msgScore[16];
	sprintf(msgScore, "Score: %d", gameScore++);
	XDrawString(dis, win, gc, 10, 20, msgScore, strlen(msgScore));
	
	//order the array of cubes so they draw on top of each other in order
	qsort(gameCubes, gameCubeCount, sizeof(gameCube), comp);
	//iterate through each cube in the field
	for (int i=0; i<gameCubeCount; i++) {
		//once wait cooldown is done
		if (gameCubes[i].wait < 0) {
			//move cube down towards player
			gameCubes[i].y *= 1.05;

			//set color of cube depending on distance from player
			double r = 2*gameCubes[i].y/height;
			if (r>1) {r=1;}
			XSetForeground(dis, gc, rgb(255*r, 128-255*(r/2), 0));

			//real coordinates and information of cube
			double tempSize = gameCubes[i].y/2;
			double tempRealX = width/2 + (gameCubes[i].x-gameCoord.x) + gameCubes[i].y*(gameCubes[i].x-gameCoord.x)*0.2;
			double tempRealY = gameCubes[i].y + height/4;

			//sets the cube's shape, location, and angle that will appear on the window
			polygonPoint.x=(tempRealX-tempSize/2)*cos(gameCoord.angle); polygonPoint.y=tempRealY+(tempSize/2)*sin(gameCoord.angle)-(tempRealX-width/2)*sin(gameCoord.angle);
			polygonSquare[0] = polygonPoint;
			polygonPoint.x=tempSize*cos(gameCoord.angle); polygonPoint.y=-tempSize*sin(gameCoord.angle);
			polygonSquare[1] = polygonPoint;
			polygonPoint.x=tempSize*cos(PI/2+gameCoord.angle); polygonPoint.y=-tempSize*sin(PI/2+gameCoord.angle);
			polygonSquare[2] = polygonPoint;
			polygonPoint.x=tempSize*cos(PI+gameCoord.angle); polygonPoint.y=-tempSize*sin(PI+gameCoord.angle);
			polygonSquare[3] = polygonPoint;

			//draws the fake square on the window
			XFillPolygon(dis, win, gc, polygonSquare, 4, Convex, CoordModePrevious);

			//square is now close to player, check if lost or reset and randomize square
			if (gameCubes[i].y > height*0.65) {
				if (fabs(gameCubes[i].x-gameCoord.x) < 1.4) {
					XSetForeground(dis, gc, white);
					char msgFail[] = "GAME OVER";
					XDrawString(dis, win, gc, 10, 40, msgFail, strlen(msgFail));
					char msgReplay[] = "press r - replay";
					XDrawString(dis, win, gc, 10, 50, msgReplay, strlen(msgReplay));
					char msgQuit[] = "press q - quit";
					XDrawString(dis, win, gc, 10, 60, msgQuit, strlen(msgQuit));
					gameStage = 2;
				} else {
					//randomize cube and reset for rerun
					gameCubes[i].x = gameCoord.x+(rand()%80)-40;
					gameCubes[i].y = height/8;
					gameCubes[i].wait = rand()%(150-gameCubeCount);
				}
			}
		} else {
			//progress the cube's wait cycle
			gameCubes[i].wait--;
		}
	}

	//handles difficulty increase and stage advancement with a cycle countdown and increasing cube amount in field
	if (gameCubeCount < 100 && gameCubeCountInterval-- < 0) {
		//the floor changes to random color
		floorColor = rgb(rand()%255, rand()%255, rand()%255);
		free(gameCubes);
		gameCubeCount += CUBECOUNT_INCREMENT;
		gameCubes = (gameCube*)malloc(gameCubeCount*sizeof(gameCube));
		for (int i=0; i<gameCubeCount; i++) {
			gameCube cube;
			cube.x = gameCoord.x+(rand()%(CUBESPREAD*2))-CUBESPREAD;
			cube.y = height/8;
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
	width = DisplayWidth(dis, scr);
	height = DisplayHeight(dis, scr);

	//shorthand for colors
	black = BlackPixel(dis, scr);
	white = WhitePixel(dis, scr);

	//create window and event handlers
    win = XCreateSimpleWindow(dis, DefaultRootWindow(dis), POSX, POSY, width, height, BORDER, white, black);
	XSetStandardProperties(dis, win, TITLE, TITLE, None, NULL, 0, NULL);
	XSelectInput(dis, win, KeyPressMask | PointerMotionMask);

	//create graphics context to draw on
	gc = XCreateGC(dis, win, 0, 0);
	XSetBackground(dis, gc, black);
	XSetForeground(dis, gc, white);

	//make windows floating on DWM
	XSizeHints xsh = {.min_width=width, .min_height=height, .max_width=width, .max_height=height};
	xsh.flags = PMinSize | PMaxSize;
	XSetSizeHints(dis, win, &xsh, XA_WM_NORMAL_HINTS);

	//map window to display
	XMapWindow(dis, win);
	XMapRaised(dis, win);
}
//handles closing the window and freeing memory
void window_close() {
	XFreeGC(dis, gc);
	XUnmapWindow(dis, win);
	XDestroyWindow(dis, win);
	XCloseDisplay(dis);
	free(gameCubes);
	free(polygonPlayer);
	free(polygonFloor);
	free(polygonSquare);
}

//spits out color that X11 needs
unsigned long rgb(int r, int g, int b) {
    return (b + (g<<8) + (r<<16));
}
