#include <stdlib.h>
#include <time.h>
#include "2048_engine.h"

/* Utilize block counter to improve some of the functions so they can run
 * quicker */

void gravitate(struct gamestate *g, direction d, void (*callback)(struct gamestate *g))
{

#define swap_if_space(xoff, yoff)\
    do {\
        if (g->grid[x][y] == 0 && g->grid[x+xoff][y+yoff] != 0) {\
            g->grid[x][y] = g->grid[x+xoff][y+yoff];\
            g->grid[x+xoff][y+yoff] = 0;\
            done = 0;\
            g->moved = 1;\
        }\
    } while (0)

    size_t x, y;
    int done = 0;

    if (d == dir_left) {
        while (!done) {
            done = 1;
            for (x = 0; x < g->opts->grid_width - 1; ++x) {
                for (y = 0; y < g->opts->grid_height; ++y) {
                    swap_if_space(1, 0);
                }
            }
            if (callback)
                callback(g);
        }
    }
    else if (d == dir_right) {
        while (!done) {
            done = 1;
            for (x = g->opts->grid_width - 1; x > 0; --x) {
                for (y = 0; y < g->opts->grid_height; ++y) {
                    swap_if_space(-1, 0);
                }
            }
            if (callback)
                callback(g);
        }
    }
    else if (d == dir_down) {
        while (!done) {
            done = 1;
            for (y = g->opts->grid_height - 1; y > 0; --y) {
                for (x = 0; x < g->opts->grid_width; ++x) {
                    swap_if_space(0, -1);
                }
            }
            if (callback)
                callback(g);
        }
    }
    else if (d == dir_up) {
        while (!done) {
            done = 1;
            for (y = 0; y < g->opts->grid_height - 1; ++y) {
                for (x = 0; x < g->opts->grid_width; ++x) {
                    swap_if_space(0, 1);
                }
            }
            if (callback)
                callback(g);
        }
    }
    else {
        fatal("Invalid direction passed to gravitate()");
        /* Not reached */
    }

#undef swap_if_space
}

void merge(struct gamestate *g, direction d, void (*callback)(struct gamestate *g))
{

#define merge_if_equal(xoff, yoff)\
    do {\
        if (g->grid[x][y] && (g->grid[x][y] == g->grid[x+xoff][y+yoff])) {\
            g->grid[x][y] += g->grid[x+xoff][y+yoff];\
            g->grid[x+xoff][y+yoff] = 0;\
            g->blocks_in_play -= 1;\
            g->score_last += g->grid[x][y];\
            g->score += g->grid[x][y];\
            g->moved = 1;\
        }\
    } while (0)

    size_t x, y;
    g->score_last = 0;
        
    if (d == dir_left) {
        for (x = 0; x < g->opts->grid_width - 1; ++x) {
            for (y = 0; y < g->opts->grid_height; ++y) {
                merge_if_equal(1, 0);
            }
        }
    }
    else if (d == dir_right) {
        for (x = g->opts->grid_width - 1; x > 0; --x) {
            for (y = 0; y < g->opts->grid_height; ++y) {
                merge_if_equal(-1, 0);
            }
        }
    }
    else if (d == dir_down) {
        for (y = g->opts->grid_height - 1; y > 0; --y) {
            for (x = 0; x < g->opts->grid_width; ++x) {
                merge_if_equal(0, -1);
            }
        }
    }
    else if (d == dir_up) {
        for (y = 0; y < g->opts->grid_height - 1; ++y) {
            for (x = 0; x < g->opts->grid_width; ++x) {
                merge_if_equal(0, 1);
            }
        }
    }
    else {
        fatal("Invalid direction passed to merge()");
        /* Not reached */
    }

    if (callback)
        callback(g);

#undef merge_if_equal
}

/* Return -1 on lose condition, 1 on win condition, 0 on
 * haven't ended */
int end_condition(struct gamestate *g)
{
    int ret = -1;
    size_t blocks_counted = 0;

    size_t x, y;
    for (x = 0; x < g->opts->grid_width; ++x) {
        for (y = 0; y < g->opts->grid_height; ++y) {
            if (g->grid[x][y]) {
                blocks_counted++;
                if (g->grid[x][y] >= g->opts->goal)
                    return 1;
                else if (blocks_counted >= g->blocks_in_play)
                    return ret;
            }
            if (g->grid[x][y] >= g->opts->goal)
                return 1; 
            if (!g->grid[x][y] || ((x + 1 < g->opts->grid_width) &&
                        (g->grid[x][y] == g->grid[x+1][y]))
                    || ((y + 1 < g->opts->grid_height) && 
                        (g->grid[x][y] == g->grid[x][y+1])))
                ret = 0;
        }
    }

    return ret;
}

/* Find a better method for getting a random square. It would be useful to keep
 * track of how many blocks are in play, which we can query here, end_condition
 * to improve them. */
void random_block(struct gamestate *g)
{
    /* pick random square, if it is full, then move forward until we find
     * an empty square. This is biased */

    static int seeded = 0;
    if (!seeded) {
        seeded = 1;
        srand(time(NULL));
    }

    /* Fix up this random number generator */
    /* Method: 
     *  -   Find a non-biased index between 0 and blocks_play, n
     *  -   Find the nth 0 element in the array
     *  -   insert a random value there
     */

    /* Error here */
#ifdef NULLO
    size_t block_position = (size_t)rand() % (
            g->opts->grid_width * g->opts->grid_height - g->blocks_in_play);

    size_t i, ps;
    for (i = 0, ps = 0; ps < block_position; ++i) {
        if (!g->grid[i / g->opts->grid_width][i % g->opts->grid_height]) ps++;
    }

    g->grid[i / g->opts->grid_width][i % g->opts->grid_height]
         = (rand() & 3) ? g->opts->spawn_value : g->opts->spawn_value * 2;
#endif

    /* Use rudimentary for now */
    int x, y;
    while (g->grid[x = rand() % g->opts->grid_width][y = rand() % g->opts->grid_height]);
    g->grid[x][y] = (rand() & 3) ? g->opts->spawn_value : g->opts->spawn_value * 2;
    g->blocks_in_play += 1;
}

/* This returns the number of digits in the base10 rep of n. The ceiling is
 * taken so this will be one greater than required */
static int clog10(unsigned int n)
{
    int l = 0;
    while (n) n /= 10, ++l;
    return l + 1;
}

struct gamestate* gamestate_init(struct gameoptions *opt)
{
    if (!opt) return NULL;

    struct gamestate *g = malloc(sizeof(struct gamestate));
    if (!g) goto gamestate_alloc_fail;
    g->gridsize = opt->grid_width * opt->grid_height;

    long *grid_back = calloc(g->gridsize, sizeof(long));
    if (!grid_back) goto grid_back_alloc_fail;
   
    g->grid = malloc(opt->grid_height * sizeof(long*));
    if (!g->grid) goto grid_alloc_fail;

    /* Switch to two allocation version */
    size_t i;
    long **iterator = g->grid;
    for (i = 0; i < g->gridsize; i += opt->grid_width)
        *iterator++ = &grid_back[i];

    g->moved = 0;
    g->score = 0;
    g->score_high = 0;
    g->score_last = 0;
    g->print_width = clog10(opt->goal);
    g->blocks_in_play = 0;
    g->opts = opt;

    /* Initial 3 random blocks */
    random_block(g);
    random_block(g);
    random_block(g);
    return g;

grid_alloc_fail:
    free(grid_back);
grid_back_alloc_fail:
    free(g);
gamestate_alloc_fail:
    return NULL;
}

struct gameoptions* gameoptions_default(void)
{
    struct gameoptions *opt = malloc(sizeof(struct gameoptions));
    if (!opt) return NULL;

    opt->grid_height = DEFAULT_GRID_HEIGHT;
    opt->grid_width = DEFAULT_GRID_WIDTH;
    opt->goal = DEFAULT_GOAL;
    opt->spawn_value = DEFAULT_SPAWN_VALUE;
    opt->spawn_rate = DEFAULT_SPAWN_RATE;
    opt->enable_color = DEFAULT_COLOR_TOGGLE;
    opt->animate = DEFAULT_ANIMATE_TOGGLE;

    return opt;
}

int gamestate_tick(struct gamestate *g, direction d, void (*callback)(struct gamestate*))
{
    /* Reset move. Altered by gravitate and merge if we do move */
    g->moved = 0;
    gravitate(g, d, callback);
    merge(g, d, callback);
    gravitate(g, d, callback);
    return g->moved;
}

void gamestate_clear(struct gamestate *g)
{
    free(g->opts);
    free(g->grid[0]);   /* Free grid data */
    free(g->grid);      /* Free pointers to data slots */
    free(g);
}

/* The following may be moved into own file */
void reset_highscore(void)
{
    printf("Are you sure you want to reset your highscores? (Y)es/(N)o: ");

    int response;
    if ((response = getchar()) == 'y' || response == 'Y') {
        printf("Resetting highscore...\n");
    }
}

void print_usage(void)
{
    printf(
    "usage: 2048 [-cCaArh] [-g <goal>] [-b <rate>] [-s <size>]\n"
    "\n"
    "controls\n"
    "   hjkl        movement keys\n"
    "   q           quit current game\n"
    "\n"
    "options\n"
    "   -s <size>   set the grid side lengths\n"
    "   -b <rate>   set the block spawn rate\n"
    "   -g <goal>   set a new goal (default 2048)\n"
    "   -a          enable animations (default)\n"
    "   -A          disable animations\n"
    "   -c          enable color support\n"
    "   -C          disable color support (default)\n"
    );
}

#include <getopt.h>

struct gameoptions* parse_options(struct gameoptions *opt, int argc, char **argv)
{
    int c;
    while ((c = getopt(argc, argv, "aArcChg:s:b:")) != -1) {
        switch (c) {
        case 'a':
            opt->animate = 1;
            break;
        case 'A':
            opt->animate = 0;
            break;
        case 'c':
            opt->enable_color = 1;
            break;
        case 'C':
            opt->enable_color = 0;
            break;
        case 'g':
            opt->goal = strtol(optarg, NULL, 10);
            break;
        case 's':;
            /* Stick with square for now */
            int optint = strtol(optarg, NULL, 10);
            if (optint < CONSTRAINT_GRID_MAX && optint > CONSTRAINT_GRID_MIN) {
                opt->grid_height = optint;
                opt->grid_width = optint;
            }
            break;
        case 'b':
            opt->spawn_rate = strtol(optarg, NULL, 10);
            break;
        case 'r':
            reset_highscore();
            exit(0);
        case 'h':
            print_usage();
            exit(0);
        }
    }

    return opt;
}
