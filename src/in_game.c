/**
 * Copyright (c) 2014, Miguel Angel Astor Romero. All rights reserved.
 * See the file LICENSE for more details.
 */

#include <stdlib.h>
#include <time.h>
#include <ncursesw/ncurses.h>
#include <fov.h>

#include "constants.h"
#include "in_game.h"
#include "map.h"

typedef struct PLAYER {
	unsigned short x;
	unsigned short y;
} player_t;

static bool **vis;
static bool **seen;
static bool ** wmap;
static bool w_mov = FALSE;
static bool uK, dK, lK, rK; 
static clock_t then;
static player_t player;
static map_cell_t ** map;
game_obj_t objs[MAX_OBJECTS];
static int mW, mH, nO;
fov_settings_type fov_settings;

void input();
gsname_t update();
void render(int, int);
void drawGui(int, int);
void setPlayerStart();
void initObjects();
void drawNeon(int, int, floor_t);
void apply(void *, int, int, int, int, void *);
bool opaque(void *, int, int);

void initInGameState( gs_t * gs) {
	int i, j;

	gs->name = IN_GAME;
	gs->input = &input;
	gs->update = &update;
	gs->render = &render;

    map = ( map_cell_t ** ) malloc ( sizeof ( map_cell_t * ) * MAX_MAP_SIZE);
	for ( i = 0; i < MAX_MAP_SIZE; ++i ) {
		map[ i ] = ( map_cell_t * ) calloc ( MAX_MAP_SIZE , sizeof ( map_cell_t ) );
	}

    wmap = ( bool ** ) malloc ( sizeof ( bool * ) * MAX_MAP_SIZE);
	for ( i = 0; i < MAX_MAP_SIZE; ++i ) {
		wmap[ i ] = ( bool * ) calloc ( MAX_MAP_SIZE, sizeof ( bool ) );
		for(j = 0; j < MAX_MAP_SIZE; ++j){
			wmap[i][j] = rand() % 2;
		}
	}

    vis = ( bool ** ) malloc ( sizeof ( bool * ) * MAX_MAP_SIZE);
    seen = ( bool ** ) malloc ( sizeof ( bool * ) * MAX_MAP_SIZE);
	for ( i = 0; i < MAX_MAP_SIZE; ++i ) {
		vis[ i ] = ( bool * ) calloc ( MAX_MAP_SIZE, sizeof ( bool ) );
        seen[ i ] = ( bool * ) calloc ( MAX_MAP_SIZE, sizeof ( bool ) );
		for(j = 0; j < MAX_MAP_SIZE; ++j){
			vis[i][j] = TRUE;
            seen[i][j] = FALSE;
		}
	}

    initObjects();

    errcode_t rc = readMapData("map_file.map", &map, &mW, &mH);
    if(rc != NO_ERROR){
        fprintf(stderr, "\t%s: readMapData() returned %d\n", __FILE__, rc);
        exit(rc);
    }

    game_obj_t * objsP = objs;
    rc = readMapObjects("map_file.map", &objsP, &nO);
    if(rc != NO_ERROR){
        fprintf(stderr, "\t%s: readMapObjects() returned %d\n", __FILE__, rc);
        exit(rc);
    }

    setPlayerStart();

    fov_settings_init(&fov_settings);
    fov_settings_set_opacity_test_function(&fov_settings, opaque);
    fov_settings_set_apply_lighting_function(&fov_settings, apply);
}

void input(){
	int key = 0;

	key = getch();

	if(key != ERR){
		fprintf(stderr, "\t%s: Caught keycode %d\n", __FILE__, key);
		if(key == KEY_UP) uK = TRUE;
		if(key == KEY_DOWN) dK = TRUE;
		if(key == KEY_LEFT) lK = TRUE;
		if(key == KEY_RIGHT) rK = TRUE;
	}
}

gsname_t update(){
    int iX, iY;

    iX = player.x;
    iY = player.y;

	if(uK){
        iY = iY - 1 < 0 ? mH - 1 : iY - 1;
		if((map[iY][iX].f > WINDOW_WALL && map[iY][iX].f <= WATER) || map[iY][iX].f == SECRET_WALL) player.y = iY;
		uK = FALSE;
	}

	if(dK){
        iY = (iY + 1) % mH;
		if((map[iY][iX].f > WINDOW_WALL && map[iY][iX].f <= WATER) || map[iY][iX].f == SECRET_WALL) player.y = iY;
		dK = FALSE;
	}

	if(lK){
        iX = iX - 1 < 0 ? mW - 1 : iX - 1;
		if((map[iY][iX].f > WINDOW_WALL && map[iY][iX].f <= WATER) || map[iY][iX].f == SECRET_WALL) player.x = iX;
		lK = FALSE;
	}

	if(rK){
        iX = (iX + 1) % mW;
		if((map[iY][iX].f > WINDOW_WALL && map[iY][iX].f <= WATER) || map[iY][iX].f == SECRET_WALL) player.x = iX;
		rK = FALSE;
	}

	return IN_GAME;
}

void render(int w, int h){
	clock_t now, delta;
	int i, j, pi, pj, ioff, joff, di, dj;

	now = clock();
	delta = now - then;
	if((float)delta / (float)CLOCKS_PER_SEC >= 0.25f){
		then = now;
		w_mov = TRUE;
	}

	pi = (((w - 1) - 27) / 2) + 27;
	pj = (h - 2) / 2 + 1;

	ioff = (w - 28) / 2;
	joff = (h - 2) / 2;

    fov_circle(&fov_settings, &map, NULL, player.x, player.y, (MAX_MAP_SIZE / 2) - 1);

	for(i = 27; i < w - 1; i++){
		for(j = 1; j < h - 1; j++){
			move(j, i);

			di = i - 27 + player.x - ioff;
			dj = j - 1 + player.y - joff;

			if( di < 0 || di >= mW || dj < 0 || dj >= mH ){
				printw(" ");
			}else{
                if(vis[dj][di] || seen[dj][di]){
                    switch(map[dj][di].f){
                        case WATER:
                            attron(COLOR_PAIR(DW_COLOR));
                            if(w_mov)
                                wmap[dj][di] = !wmap[dj][di];
                            if(wmap[dj][di])
                                printw("\u2248");
                            else
                                printw("~");
                            break;

                        case VOID:
                            if(!vis[dj][di]) attron(COLOR_PAIR(DW_COLOR));
                            else attron(COLOR_PAIR(MN_COLOR));
                            printw(" ");
                            break;

                        case EMPTY_FLOOR:
                            if(!vis[dj][di]) attron(COLOR_PAIR(DW_COLOR));
                            else attron(COLOR_PAIR(MN_COLOR));
                            printw(" ");
                            break;

                        case RUG:
                            if(!vis[dj][di]) attron(COLOR_PAIR(DW_COLOR));
                            else attron(COLOR_PAIR(SN_COLOR));
                            printw("\u2592");
                            break;

                        case WINDOW_WALL:
                            if(!vis[dj][di]) attron(COLOR_PAIR(DW_COLOR));
                            else attron(COLOR_PAIR(SW_COLOR));
                            printw("\u2591");
                            break;

                        case CLEAR_WALL:
                            if(!vis[dj][di]) attron(COLOR_PAIR(DW_COLOR));
                            else attron(COLOR_PAIR(SW_COLOR));
                            printw("\u2588");
                            break;

                        case SECRET_WALL:
                        case SOLID_WALL:
                            if(!vis[dj][di]) attron(COLOR_PAIR(DW_COLOR));
                            else attron(COLOR_PAIR(MN_COLOR));
                            printw("\u2588");
                            break;

                        case NEON_WALL:
                            drawNeon(dj, di, NEON_WALL);
                            break;

                        case BAR:
                            drawNeon(dj, di, BAR);
                            break;
                    }
                }else{
                    attron(COLOR_PAIR(MN_COLOR));
                    printw(" ");
                }
			}
		}
	}
	w_mov = FALSE;

	move(pj, pi);
	attron(COLOR_PAIR(BSC_COLOR));
	printw(/*"\u263A"*/ "@");

	drawGui(w, h);

    for ( i = 0; i < MAX_MAP_SIZE; ++i ) {
		for(j = 0; j < MAX_MAP_SIZE; ++j){
			vis[i][j] = FALSE;
		}
	}
}

void drawNeon(int i, int j, floor_t floor){
    int r;
    bool n, s, e, w;

    if(floor == NEON_WALL){
        if(!vis[i][j]) attron(COLOR_PAIR(DW_COLOR));
        else{
            r = rand() % 1000;
            if(r < 998)
                attron(COLOR_PAIR(FR_COLOR));
            else
                attron(COLOR_PAIR(MN_COLOR));
        }
    }else if(floor == BAR){
        if(!vis[i][j]) attron(COLOR_PAIR(DW_COLOR));
        else attron(COLOR_PAIR(SN_COLOR));
    }

    n = map[i                         ][j - 1 < 0 ? mH - 1 : j - 1].f == floor;
    s = map[i                         ][(j + 1) % mH              ].f == floor;
    e = map[i - 1 < 0 ? mW - 1 : i - 1][j                         ].f == floor;
    w = map[(i + 1) % mW              ][j                         ].f == floor;

    if((n && s && e) && (!w)){
        printw("\u2560");
        return;
    }

    if((n || s) && (!e && !w)){
        printw("\u2550");
        return;
    }

    if((e || w) && (!n && !s)){
        printw("\u2551");
        return;
    }

    if((e && n) && (!s && !w)){
        printw("\u255D");
        return;
    }

    if((w && n) && (!s && !e)){
        printw("\u2557");
        return;
    }

    if((e && s) && (!n && !w)){
        printw("\u255A");
        return;
    }

    if((w && s) && (!n && !e)){
        printw("\u2554");
        return;
    }

    if((s && e && w) && (!n)){
        printw("\u2560");
        return;
    }

    if((n && w && e) && (!s)){
        printw("\u2563");
        return;
    }

    if(n && s && e && w){
        printw("\u256C");
        return;
    }
}

void drawGui(int w, int h){
	int i, j;

	attron(COLOR_PAIR(BSC_COLOR));

	/* Clear the gui space. */
	for(i = 1; i < 26; i++){
		for(j = 1; j < h - 1; j++){
			move(j, i);
			printw(" "); 
		}
	}

	/* Upper horizontal bar. */
	move(0, 0);
	printw("\u2554");
	for(i = 0; i < w - 2; i++){
		if(i != 25){
			printw("\u2550");
		}else{
			printw("\u2566");
		}
	}
	printw("\u2557");

	/* Lower horizontal bar. */
	move(h - 1, 0);
	printw("\u255A");
	for(i = 0; i < w - 2; i++){
		if(i != 25){
			printw("\u2550");
		}else{
			printw("\u2569");
		}
	}
	printw("\u255D");

	/* Vertical bars. */
	for(i = 1; i < h - 1; i++){
		move(i, 0);
		printw("\u2551");
		move(i, 26);
		printw("\u2551");
		move(i, w-1);
		printw("\u2551");
	}
}

void setPlayerStart(){
	int i;

    for(i = 0; i < nO; i++){
        if(objs[i].type == PLAYER_START){
            player.y = objs[i].sX;
            player.x = objs[i].sY;
            break;
        }
    }
}

void initObjects(){
    int i;

    for(i = 0; i < MAX_OBJECTS; ++i){
        objs[i].type = NONE;
        objs[i].x = 0;
        objs[i].y = 0;
        objs[i].eX = 0;
        objs[i].eY = 0;
        objs[i].sX = 0;
        objs[i].sY = 0;
        objs[i].id = 0;
        objs[i].dId = 0;
        objs[i].name[0] = '\0';
        objs[i].target[0] = '\0';
        objs[i].dialog[0] = '\0';
        objs[i].unlocked = 0;
    }
}

void apply(void *map, int x, int y, int dx, int dy, void *src){
    if(x < 0 || x >= MAX_MAP_SIZE) return;
    if(y < 0 || y >= MAX_MAP_SIZE) return;

	vis[y][x] = TRUE;
    seen[y][x] = TRUE;
}

bool opaque(void *m, int x, int y){
    if(x < 0 || x >= MAX_MAP_SIZE) return FALSE;
    if(y < 0 || y >= MAX_MAP_SIZE) return FALSE;

	if(map[y][x].f == SOLID_WALL || map[y][x].f == SECRET_WALL){
        return TRUE;
    }else{
        return FALSE;
    }
}
