#include <string.h> //For memset()

#include "math.cpp"


const uint16_t MAP_W = 1024, MAP_H = 1024;
const uint32_t MAP_A = MAP_W * MAP_H;
uint16_t map[MAP_W][MAP_H]; //00000000-00000000 - 0000 frame, 0000 sprite, 00 biome

//Consonants
const uint8_t GEN_ISLANDS = 16;
const uint16_t GEN_ISLAND_MIN_RAD = 64;
const uint16_t GEN_ISLAND_MAX_RAD = 128;
const uint8_t GEN_ISLAND_RES = 4; //'resolution' of an island - how many blobs make it up

void setBiome (uint16_t x, uint16_t y, uint8_t b)
{
    if (x > 0 && x < MAP_W && y > 0 && y < MAP_H) { map[x][y] = b; }
}


void genMap ()
{
  //Set map to water
    memset(map, 3, sizeof map);
  //Generate islands
  //Generated by randomly placing differently sized circles (blobs) in a group
  for (uint w = 0; w < GEN_ISLANDS; ++w) { //For each group of islands
    //Calc island size and pos
      const uint16_t island_radius = ri(GEN_ISLAND_MIN_RAD, GEN_ISLAND_MAX_RAD);
      uint16_t island_X, island_Y;
      random_coord(MAP_W, MAP_H, island_X, island_Y);
    //Calc blob size and pos
      uint16_t blob_X = island_X + ri(island_radius / 2, island_radius);
      uint16_t blob_Y = island_Y + ri(island_radius / 2, island_radius);
      uint islandRad = ri(GEN_ISLAND_MIN_RAD, GEN_ISLAND_MAX_RAD / 2);

      for (uint b = 0; b < GEN_ISLAND_RES; ++b) { //For each blob in the island
          uint blob_X = island_X + ri(0, island_radius * 2);
          uint blob_Y = island_Y + ri(0, island_radius * 2);
        //Go through all angles from 0 to 2 * PI radians, in an ever-smaller circle (size), to fill the body of water
          float size = 1.0;
          const float step = .005;
          while (size > step) {
              float r = size * island_radius;
              for (float ang = 0; ang < 6.28; ang += .01) {
                  uint x = blob_X + r * sinf(ang);
                  uint y = blob_Y + r * cosf(ang);
                //Place some land there
                  setBiome(x, y, 1);
              }
              size -= step;
          }
      }
  }
}
