#define _XOPEN_SOURCE_EXTENDED 1 // thanks gippity (fixed screen buffer flicker issue)
#include <ncursesw/ncurses.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <locale.h>

#define WIDTH 100
#define HEIGHT 50

int grid[HEIGHT][WIDTH];
int next[HEIGHT][WIDTH];

// 18 binary values representing 'rules'
int birth[9] = {0};
int survive[9] = {0};

// percentage of the map which is to be populated with ON cells at generation
int density[25] = {100, 99, 98, 95, 90, 85, 80, 75, 70, 65, 60, 55, 50, 45, 40, 35, 30, 25, 20, 15, 10, 5, 2, 1, 0};

int sleep_value = 131072; // usleep value, controls speed of the program

// initialize some random pair of rules
void random_rules() {
    int bits[18] = {0}; // to be populated into birth/survive
    int flipped = 0; // number of bits flipped
    /*
     * I think, theoretically, we only need to look at half of the search
     * space. there are 2^18 life-like rules, however there's some repetitive
     * work being done there. every life-like ruleset has a complementary
     * ruleset which is functionally identical, where the behavior of
     * the ON and OFF cells are swapped. you could try to  cut the search
     * space in half to 2^17 by starting from an all-zero ruleset, and
     * then allowing up to 9 bits to be flipped. if 10 or more bits are
     * flipped, then there exists a functionally identical equivalent of
     * it which can be done with <= 9 bits flipped.
     *
     * anyways, I have not implemented this and the full 2^18 ruleset is
     * possible :)
    */
    for (int i=0;i<9;i++) {
	birth[i] = rand() % 2;
	survive[i] = rand() % 2;
    }
    
    
    // uncomment this to see the original conways game of life ruleset :)
    /*
    for(int i=0;i<9;i++) {
    	birth[i]=0; survive[i]=0;
    }
    birth[3] = 1;
    survive[2] = 1;
    survive[3] = 1;
    */
}

// randomize position
void init_grid(int density_index) {
    int density_value = density[density_index];
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            grid[y][x] = rand() % 100 < density_value;	// modify density of graph here
	}
    }
}

int count_neighbors(int y, int x) {
    int count = 0;
    /* checking adjacent cells as follows:
     *
     * [ UP LEFT ] -> [  UP  ] -> [ UP RIGHT ]
     * [   LEFT  ] -> [CENTER] -> [   RIGHT  ]
     * [DOWN LEFT] -> [ DOWN ] -> [DOWN RIGHT]
     * 
     * return: number of adjacent cells that are ON
    */
    for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
            if (dy == 0 && dx == 0) continue;	// don't count current cell 


	    int ny = y+dy;
	    int nx = x+dx;
	    if (ny < 0)
                ny = HEIGHT - 1;
	    else if (ny >= HEIGHT)
		ny = 0;

	    if (nx < 0)
                nx = WIDTH - 1;
	    else if (nx >= WIDTH)
		nx = 0;


            // int ny = (y + dy + HEIGHT) % HEIGHT;	// wrap around screen vertically
            // int nx = (x + dx + WIDTH) % WIDTH;	// wrap around screen horizontally

            count += grid[ny][nx];
        }
    }
    return count;
}

void update() {
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            int neighbors = count_neighbors(y, x);
            if (grid[y][x])
                next[y][x] = survive[neighbors];  // checks against current survival rules
            else
                next[y][x] = birth[neighbors];    // checks against current birth rules
        }
    }

    for (int y = 0; y < HEIGHT; y++)
        for (int x = 0; x < WIDTH; x++)
	    grid[y][x] = next[y][x];
}

void draw() {
    // thanks gippity lol
    wchar_t line[WIDTH + 1];

    for (int y = 0; y < HEIGHT; y += 2) {
        for (int x = 0; x < WIDTH; x++) {
            int top = grid[y][x];
            int bottom = (y + 1 < HEIGHT) ? grid[y + 1][x] : 0;

            if (top && bottom) line[x] = L'█';
            else if (top) line[x] = L'▀';
            else if (bottom) line[x] = L'▄';
            else line[x] = L' ';
        }
        line[WIDTH] = L'\0';
        mvaddwstr(y / 2 + 2, 0, line);
    }
}

// rule toggles
void handle_rule_input(int ch) {
    // toggle birth 0-8 with keys '1'-'9'
    // toggle survive 0-8 with keys 'a'-'l' (a=0,s=1,...l=8)
    if (ch >= '1' && ch <= '9') {
        int n = ch - '1';
        birth[n] = !birth[n];
    } else {
        switch(ch) {
	case 'a':
	    survive[0] = !survive[0];
            break;
	case 's':
	    survive[1] = !survive[1];
            break;
	case 'd':
	    survive[2] = !survive[2];
            break;
	case 'f':
	    survive[3] = !survive[3];
            break;
	case 'g':
	    survive[4] = !survive[4];
            break;
	case 'h':
	    survive[5] = !survive[5];
            break;
	case 'j':
	    survive[6] = !survive[6];
            break;
	case 'k':
	    survive[7] = !survive[7];
            break;
	case 'l':
	    survive[8] = !survive[8];
            break;
            
	default:
	    break;
        }
    }
}

// stop the simulation in it's current state (implemented cleverly)
void pause_grid() {
    for (int i = 0; i < 9; i++) {
	birth[i] = 0;
	survive[i] = 1;
    }
    // no way to unpause lol
}

// display controls and current rules at the top
void draw_rule(int dense_level) {
    mvprintw(0, 0, "Controls: q=quit, r=randomize map, c=randomize rules, 0-8=toggle birth, a-l=toggle survive +=increase density -=decrease density ,=decrease speed .=increase speed");
    move(1,0);
    printw("Birth:   ");
    for(int i=0;i<9;i++) printw("%d", birth[i]);
    printw("  Survive: ");
    for(int i=0;i<9;i++) printw("%d", survive[i]);
    printw("  Current density: %d", density[dense_level]);
    printw("  Current speed: %d", sleep_value);
}

int main() {
    setlocale(LC_ALL, ""); 			// allow wide chars
    initscr();
    noecho();
    curs_set(FALSE);
    nodelay(stdscr, TRUE);
    keypad(stdscr, TRUE);
    srand(time(NULL));
    int dense_level = 20; 			// 50% percentage of map to be filled
    int dense_size = sizeof(density) / sizeof(density[0]); // get number of elements in density


    random_rules();				// initialize random rules
    init_grid(dense_level);				// initialize random starting position

    while (1) {
        int ch = getch();
        if (ch == 'q') break;				// exit condition
        if (ch == 'r') init_grid(dense_level);	// randomize map condition
	if (ch == 'c') random_rules();			// randomize rules condition
        if (ch == 'p') pause_grid();			// pause the simulation in its current state
	if (ch == '-') {
	    dense_level = (dense_level+1) % dense_size;	// cycle through array
	}
	if (ch == '+') {
	    dense_level = (dense_level-1);
	    if (dense_level < 0) {
		dense_level = dense_size - 1;
	    }
	}
	if (ch == ',') {
            sleep_value >>= 1;
	    if (sleep_value < 1) {
                sleep_value = 1;
	    }
	}
	if (ch == '.') {
            sleep_value <<= 1;
	    if (sleep_value > (1 << 18)) {
		sleep_value = (1 << 18);
	    }
	}
	handle_rule_input(ch);				// check for user bitflips

        erase();
        draw_rule(dense_level);
        draw();
        refresh();

	update();
	usleep(sleep_value);
	//usleep(100000);
    }

    endwin();
    return 0;
}
