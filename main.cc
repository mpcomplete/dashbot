#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <iostream>
#include <vector>
#include <string>
#include <set>

#include "readpng.h"
#include "xutil.h"

#define arraysize(x) (sizeof(x)/sizeof(x[0]))

// The size of the game board, in tiles.
const int kNumXTiles = 10;
const int kNumYTiles = 9;

// The size of the game board, in pixels.
const int kBoardWidth = 400;
const int kBoardHeight = 360;

// The size of a (square) tile, in pixels.
const int kTileSize = 40;

// Offsets of key pieces in the game window, in pixels from the top-left corner
// of the border:
// Location of the game board (the top-left tile).
const int kBorderOffsetBoard[] = {79, 131};
// Locaiton of the "landmark" (the game logo), which we use to find where the
// game board is.
const int kBorderOffsetLandmark[] = {583, 2};
// Location of the Play button.
const int kBorderOffsetPlay[] = {520, 450};

// A snapshot of the game board state. Each tile is a letter code, like 'p' for
// purple.
struct Board {
  char tiles[kNumYTiles][kNumXTiles];
  char& get(int x, int y) { return tiles[y][x]; }
};

// A point on the screen, either in pixels or tiles, because I'm a wildcard.
struct Point {
  int x, y;
  Point() : x(0), y(0) { }
  Point(int x, int y) : x(x), y(y) { }
};

// A png of a game tile and its corresponding letter code.
struct Tile {
  PngImage image;
  char code;
};
Tile g_tiles[6];  // blue, green, red, purple, yellow, diamond
PngImage g_landmark;  // game logo tells us where the game window is
Point g_borderpos;  // top-left corner of the game window
Point g_boardpos;  // top-left corner of the game board (the first tile)
Display* g_display;  // X display

inline int blend(png_byte a, png_byte b) {
  return a + b - 2 * std::min(a, b);
}

int diff_color(png_byte* a, png_byte* b)
{
  return (blend(a[0], b[0])
          + blend(a[1], b[1])
          + blend(a[2], b[2])) / 3;
}

int write_diff_color(png_byte* a, png_byte* b)
{
  int d = diff_color(a, b);
  a[0] = blend(a[0], b[0]);
  a[1] = blend(a[1], b[1]);
  a[2] = blend(a[2], b[2]);
  return d;
}

// Diffs screen at the given coords with image.
int diff_image(PngImage* image, PngImage* screen,
               int start_x, int start_y, int max_diff)
{
  int diff = 0;
  int h = std::min(image->height, screen->height - start_y);
  int w = std::min(image->width, screen->width - start_x);
  for (int y = 0; y < h; ++y) {
    for (int x = 0; x < w; ++x) {
      int d = diff_color(image->get(x, y),
                         screen->get(start_x + x, start_y + y));
      diff += d;

      if (diff > max_diff)
        return diff;
    }
  }
  return diff;
}

void create_diff_image(PngImage* image, PngImage* screen,
                       int start_x, int start_y)
{
  int diff = 0;
  int h = std::min(image->height, screen->height - start_y);
  int w = std::min(image->width, screen->width - start_x);
  for (int y = 0; y < h; ++y) {
    for (int x = 0; x < w; ++x) {
      write_diff_color(screen->get(start_x + x, start_y + y),
                       image->get(x, y));
    }
  }
}

// Take a screenshot and look for our "landmark" image to find where the
// game board is.
void init_boardpos()
{
  XImage* image;
  int width = DisplayWidth(g_display, DefaultScreen(g_display));
  int height = DisplayHeight(g_display, DefaultScreen(g_display));
  image = XGetImage(g_display, DefaultRootWindow(g_display),
                    0, 0, width, height,
                    AllPlanes, XYPixmap);
  PngImage screen;
  screen.from_ximage(image, width, height);
  XFree(image);

  Point start(0, 0);
//  mouse_getpos(&start.x, &start.y);

  for (int y = start.y; y < height - g_landmark.height + 1; ++y) {
    printf("Looking for game board at y pos %d...\n", y);
    for (int x = start.x; x < width - g_landmark.width + 1; ++x) {
      int diff = diff_image(&g_landmark, &screen, x, y, 10000);
      if (diff < 10000) {
        g_borderpos.x = x - kBorderOffsetLandmark[0];
        g_borderpos.y = y - kBorderOffsetLandmark[1];
        g_boardpos.x = g_borderpos.x + kBorderOffsetBoard[0];
        g_boardpos.y = g_borderpos.y + kBorderOffsetBoard[1];
        printf("Found the game board: %d,%d (diff=%d)\n",
               g_boardpos.x, g_boardpos.y, diff);
        return;
      }
    }
  }

  printf("Uh oh! Couldn't find the game board! Is the game on the screen?\n");
}

// Try to identify which tile is at the given tile position. If we're not sure,
// we leave it as unknown, because mistakes are penalized more than
// uncertainty.
void identify_tile(PngImage* screen, int x, int y, Board* board)
{
  board->get(x, y) = '?';

  int thresh = 40000;
  for (int i = 0; i < 6; ++i) {
    int d = diff_image(&g_tiles[i].image, screen,
                       x*kTileSize, y*kTileSize, thresh);
    if (d < thresh) {
      board->get(x, y) = g_tiles[i].code;
      thresh = d;
    }
  }
}

// Take a screenshot of the game window into the |screen| image, and process
// it to identify all the tiles.
void scan_board(Board* board, PngImage* screen)
{
  XImage* ximage;
  ximage = XGetImage(g_display, DefaultRootWindow(g_display),
                     g_boardpos.x, g_boardpos.y,
                     kBoardWidth, kBoardHeight,
                     AllPlanes, XYPixmap);
  screen->from_ximage(ximage, kBoardWidth, kBoardHeight);
  XFree(ximage);

  printf("Identifying tiles...\n");
  for (int y = 0; y < kNumYTiles; ++y) {
    for (int x = 0; x < kNumXTiles; ++x) {
      identify_tile(screen, x, y, board);
    }
  }

#if 1
  for (int y = 0; y < kNumYTiles; ++y) {
    for (int x = 0; x < kNumXTiles; ++x) {
      printf("%c", board->get(x, y));
    }
    printf("\n");
  }
#endif
}

int get_tile_from_code(char code)
{
  for (int i = 0; i < 6; ++i) {
    if (code == g_tiles[i].code)
      return i;
  }
  return -1;
}

// Draw a colored box at the given location in the image.
void draw_box(PngImage* image, png_byte* color, int sx, int sy, int w, int h) {
  int x, y;

  for (int x = 0; x < w; ++x) {
    image->set(sx + x, sy, color);
    image->set(sx + x, sy + h, color);
  }
  for (int y = 0; y < h; ++y) {
    image->set(sx,     sy + y, color);
    image->set(sx + w, sy + y, color);
  }
}

// Dumps a diff of the game board as it is from what we think it is. Used for
// debugging.
void dump_screen_diff(Board* board, PngImage* screen,
                      const std::vector<Point>& clusters)
{
  static int counter = 0;
  char buf[256];

  ++counter;

  // draw a red box around each cluster
  png_byte red[] = {255, 0, 0};
  for (size_t i = 0; i < clusters.size(); ++i) {
    draw_box(screen, red,
             clusters[i].x * kTileSize, clusters[i].y * kTileSize,
             kTileSize, kTileSize);
  }

  snprintf(buf, sizeof(buf), "screens/%04d-orig.png", counter);
  screen->write_file(buf);

  // diff the screen with what we think it is
  for (int y = 0; y < kNumYTiles; ++y) {
    for (int x = 0; x < kNumXTiles; ++x) {
      int tile = get_tile_from_code(board->get(x, y));
      if (tile != -1) {
        create_diff_image(&g_tiles[tile].image, screen,
                          x*kTileSize, y*kTileSize);
      }
    }
  }

  snprintf(buf, sizeof(buf), "screens/%04d-diff.png", counter);
  screen->write_file(buf);
}

// Returns the size of the tile cluster at the given position (ie, the number
// of identical tiles adjacent). Modifies the game board.
int count_cluster(Board* board, int x, int y, char t)
{
  if (x < 0 || y < 0 || x >= kNumXTiles || y >= kNumYTiles)
    return 0;

  int cluster = 1;
  if (board->get(x, y) != t)
    return 0;
  board->get(x, y) = 'v';  // mark as visited

  cluster += count_cluster(board, x+1, y, t);
  cluster += count_cluster(board, x, y+1, t);
  cluster += count_cluster(board, x-1, y, t);
  // no need to test up, since we scan downwards.

  return cluster;
}

// Fill the area around the cluster so that we don't click on exploding
// tiles in flamey-mode.
// .....      .VVV.
// .vvv. ===> VVVVV
// ...v.      .VVVV
// .....      ...V.
void mark_cluster_border(Board* board, int x, int y)
{
  if (x < 0 || y < 0 || x >= kNumXTiles || y >= kNumYTiles)
    return;

  char c = board->get(x, y);
  board->get(x, y) = 'V';
  if (c != 'v')
    return;

  mark_cluster_border(board, x+1, y);
  mark_cluster_border(board, x, y+1);
  mark_cluster_border(board, x-1, y);
  mark_cluster_border(board, x, y-1);
}

// Converts a cluster of visited tiles to a different code, so we can
// distinguish between a cluster and just visited tiles.
void mark_noncluster(Board* board, int x, int y)
{
  if (x < 0 || y < 0 || x >= kNumXTiles || y >= kNumYTiles)
    return;

  char c = board->get(x, y);
  if (c != 'v')
    return;
  board->get(x, y) = 'x';

  mark_noncluster(board, x+1, y);
  mark_noncluster(board, x, y+1);
  mark_noncluster(board, x-1, y);
  mark_noncluster(board, x, y-1);
}

// Finds all the tile clusters on the current board, and returns them as a list
// of tile positions.
void find_clusters(Board* board, std::vector<Point>* clusters)
{
  // We mark up copy so we don't revisit old tiles.
  // Key:
  // v = visited, temporary, maybe part of a cluster
  // V = part of, or bordering, a cluster of 3 or more
  // x = visited, not part of a cluster
  Board copy;
  memcpy(copy.tiles, board->tiles, sizeof(board->tiles));

  for (int y = 0; y < kNumYTiles; ++y) {
    for (int x = 0; x < kNumXTiles; ++x) {
      char t = copy.get(x, y);
      if (t == '?' || t == 'v' || t == 'V' || t == 'x')
        continue;
      int cluster = count_cluster(&copy, x, y, t);
      if (cluster >= 3 || t == 'd') {
        mark_cluster_border(&copy, x, y);
        clusters->push_back(Point(x, y));
      } else {
        mark_noncluster(&copy, x, y);
      }
    }
  }

#if 0
  for (int y = 0; y < kNumYTiles; ++y) {
    for (int x = 0; x < kNumXTiles; ++x) {
      printf("%c", copy.get(x, y));
    }
    printf("\n");
  }
#endif
}

// Simulate a mouse click at the given tile position.
void click_tile(const Point& tile)
{
  mouse_move(g_boardpos.x + tile.x*kTileSize + kTileSize/2,
             g_boardpos.y + tile.y*kTileSize + kTileSize/2);
  mouse_click(Button1);
  printf("Clicking tile at: %dx%d\n", tile.x, tile.y);
}

// Returns true if the Right Control key is being pressed.
bool is_ctrl_pressed()
{
  static int ctrl_keycode =
      XKeysymToKeycode(g_display, XStringToKeysym("Control_R"));

  char keys[32];
  XQueryKeymap(g_display, keys);

  // keycode: 6 bits: UUULLL
  // UUU = index into keys
  // LLL = index into bit of keys[UUU]
  return keys[ctrl_keycode >> 3] & (1<<(ctrl_keycode & 7));
}

// Loads a tile .png and assigns it a letter code.
void load_tile(int index, const std::string& filename, char code) {
  printf("Loading %s...\n", filename.c_str());
  g_tiles[index].image.read_file("tiles/" + filename);
  g_tiles[index].code = code;
}

int main(int argc, char *argv[])
{ 
  g_display = XOpenDisplay(NULL);
  if (g_display == NULL) {
    fprintf (stderr, "Can't open g_display!\n");
    return -1;
  }

  load_tile(0, "blue.png", 'b');
  load_tile(1, "green.png", 'g');
  load_tile(2, "purple.png", 'p');
  load_tile(3, "red.png", 'r');
  load_tile(4, "yellow.png", 'y');
  load_tile(5, "diamond.png", 'd');
  g_landmark.read_file("tiles/landmark.png");

  printf("\nReady!\n");
  printf("Go to the game's main menu.\n");
  printf("Hold the RIGHT CTRL key to enable the bot.\n");

  while (!is_ctrl_pressed())
    usleep(100*1000);

  init_boardpos();

  printf("Hold RIGHT CTRL and the bot will click Play.\n");

  do {
    usleep(100*1000);
  } while (!is_ctrl_pressed());
  mouse_move(g_borderpos.x + kBorderOffsetPlay[0],
             g_borderpos.y + kBorderOffsetPlay[1]);
  mouse_click(Button1);

  printf("Playing. Hold RIGHT CTRL and the bot will do its thing.\n");

  for (;;) {
    if (!is_ctrl_pressed()) {
      usleep(10*1000);
      continue;
    }

    Board board;
    PngImage screen;
    scan_board(&board, &screen);

    std::vector<Point> clusters;
    find_clusters(&board, &clusters);
    printf("Found %lu clusters.\n", clusters.size());

//    dump_screen_diff(&board, &screen, clusters);

    for (size_t i = 0; i < clusters.size(); ++i) {
      if (!is_ctrl_pressed())
        break;
      click_tile(clusters[i]);
//      usleep(150 * 1000);
    }

    fflush(stdout);
    usleep(330*1000);
  }

  XCloseDisplay(g_display);
  return 0;
}
