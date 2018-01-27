#include <string.h> //For memset()

#include "math.cpp"


const uint16_t MAP_W = 512, MAP_H = 512;
const uint32_t MAP_A = MAP_W * MAP_H;
uint16_t map[MAP_W][MAP_H]; //00000000-00000000 - 00 luminosity, 0 animated, 000 frame, 0000 sprite, 00 biome

//Constants
const uint8_t GEN_ISLANDS = 8;
const uint16_t GEN_ISLAND_RAD_MIN = 64;
const uint16_t GEN_ISLAND_RAD_MAX = 128;
const uint8_t GEN_ISLAND_RES = 4; //'resolution' of an island - how many blobs make it up
const uint8_t GEN_VILLAGES = 32;
const uint16_t GEN_VILLAGE_RAD_MIN = 16;
const uint16_t GEN_VILLAGE_RAD_MAX = 32;
const uint16_t GEN_GROW_MAP = 4096;
const uint16_t MAP_GROW_SPEED = 128;
const uint16_t MAP_DEATH_SPEED = 32;

bool inBounds (uint16_t x, uint16_t y)
{
    return x > 0 && x < MAP_W && y > 0 && y < MAP_H;
}

uint8_t getBiome (uint16_t x, uint16_t y) { if (inBounds(x, y)) {
    return map[x][y] & 0x03;
} }
void setBiome (uint16_t x, uint16_t y, uint8_t b) { if (inBounds(x, y)) {
    map[x][y] = (map[x][y] & 0xFFFC) | b;
} }
uint8_t getSprite (uint16_t x, uint16_t y) { if (inBounds(x, y)) {
    return (map[x][y] & 0x3C) >> 2;
} }
void setSprite (uint16_t x, uint16_t y, uint8_t s) { if (inBounds(x, y)) {
    map[x][y] = (map[x][y] & 0xFFC3) | (s << 2);
} }
uint8_t getFrame (uint16_t x, uint16_t y) {  if (inBounds(x, y)) {
    return (map[x][y] & 0x1C0) >> 6;
} }
void setFrame (uint16_t x, uint16_t y, uint8_t f) { if (inBounds(x, y)) {
    map[x][y] = (map[x][y] & 0xFE3F) | (f << 6);
} }
bool getAnimated (uint16_t x, uint16_t y) {  if (inBounds(x, y)) {
    return (map[x][y] & 0x200) >> 9;
} }
void setAnimated (uint16_t x, uint16_t y, bool a) { if (inBounds(x, y)) {
    map[x][y] = (map[x][y] & 0xFDFF) | (a << 9);
} }
uint8_t getLux (uint16_t x, uint16_t y) {  if (inBounds(x, y)) {
    return (map[x][y] & 0xC00) >> 10;
} }
void setLux (uint16_t x, uint16_t y, uint8_t l, bool append = false) { if (inBounds(x, y)) {
    if (append) { if (getLux(x, y) > l) { return; } }
    map[x][y] = (map[x][y] & 0xF3FF) | (l << 10);
} }

bool isFoliage (uint8_t sprite_code)
{
    switch (sprite_code) {
        case 4: case 5: return true;
        default: return false;
    }
}

void growMap (uint16_t grow_speed = MAP_GROW_SPEED, uint16_t death_speed = MAP_DEATH_SPEED);

void genMap ()
{
  //Set map to water
    memset(map, 3, sizeof map);
  //Generate islands
  //Generated by randomly placing differently sized circles (blobs) in a group
    for (uint8_t i = 0; i < GEN_ISLANDS; ++i) { //For each island
      //Calc island size and pos
        const uint16_t island_radius = ri(GEN_ISLAND_RAD_MIN, GEN_ISLAND_RAD_MAX);
        uint16_t island_X, island_Y;
        random_coord(MAP_W, MAP_H, island_X, island_Y);
      //Calc blob size and pos
        uint16_t blob_X = island_X + ri(island_radius / 2, island_radius);
        uint16_t blob_Y = island_Y + ri(island_radius / 2, island_radius);

        for (uint8_t b = 0; b < GEN_ISLAND_RES; ++b) { //For each blob in the island
            uint16_t blob_X = island_X + ri(0, island_radius * 2);
            uint16_t blob_Y = island_Y + ri(0, island_radius * 2);
          //Go through all angles from 0 to 2 * PI radians, in an ever-smaller circle (size), to fill the blob
            float size = 1.0;
            const float step = .005;
            while (size > step) {
                float r = size * island_radius;
                for (float ang = 0; ang < 6.28; ang += .01) {
                    uint16_t x = blob_X + r * sinf(ang);
                    uint16_t y = blob_Y + r * cosf(ang);
                  //Place some land there
                    setBiome(x, y, 1);
                }
                size -= step;
            }
        }
    }
  //Generate villages
    for (uint8_t v = 0; v < GEN_VILLAGES; ++v) { //For each village
      //Calc island size and pos
        const uint16_t village_radius = ri(GEN_VILLAGE_RAD_MIN, GEN_VILLAGE_RAD_MAX);
        uint16_t village_X, village_Y;
        do {
            random_coord(MAP_W, MAP_H, village_X, village_Y);
        } while (getBiome(village_X, village_Y) == 3);
      //Go through all angles from 0 to 2 * PI radians, in an ever-smaller circle (size), to fill the village biome
        float size = 1.0;
        const float step = .005;
        while (size > step) {
            float r = size * village_radius;
            for (float ang = 0; ang < 6.28; ang += .01) {
                uint16_t x = village_X + r * sinf(ang);
                uint16_t y = village_Y + r * cosf(ang);
                setBiome(x, y, 0);
            }
            size -= step;
        }
      //Go through all angles and add wall of crates
        for (float ang = 0; ang < 6.28; ang += .01) {
            uint16_t x = village_X + village_radius * sinf(ang);
            uint16_t y = village_Y + village_radius * cosf(ang);
            setSprite(x, y, 2);
        }
        size -= step;
    }
  //En-masse
    for (uint16_t y = 1; y < MAP_H - 1; ++y) {
        for (uint16_t x = 1; x < MAP_W - 1; ++x) {
          //Set animation properties (idk why I even have to do this)
            setFrame(x, y, 0);
            setAnimated(x, y, false);
          //Remove all brick walls surrounded by stone biome (where village blobs have overlapped)
            if (getSprite(x, y) == 2 && !getBiome(x+1, y) && !getBiome(x-1, y) && !getBiome(x, y+1) && !getBiome(x, y-1)) {
                setSprite(x, y, 0);
            }
            uint8_t biome_code = getBiome(x, y);
          //Add random fireplace and luminosity on stone
            if (!biome_code && !getSprite(x, y) && rb(.01)) {
                setSprite(x, y, 3);
                setAnimated(x, y, true);
                setFrame(x, y, ri(0, 6));
                setLux(x+0,y+0,3, true);
                setLux(x+1,y+0,2, true);
                setLux(x+2,y+0,1, true);
                setLux(x-1,y+0,2, true);
                setLux(x-2,y+0,1, true);
                setLux(x+0,y+1,2, true);
                setLux(x+1,y+1,2, true);
                setLux(x+2,y+1,1, true);
                setLux(x-1,y+1,2, true);
                setLux(x-2,y+1,1, true);
                setLux(x+0,y+2,1, true);
                setLux(x+1,y+2,1, true);
                setLux(x-1,y+2,1, true);
                setLux(x+0,y-1,2, true);
                setLux(x+1,y-1,2, true);
                setLux(x+2,y-1,1, true);
                setLux(x-1,y-1,2, true);
                setLux(x-2,y-1,1, true);
                setLux(x+0,y-2,1, true);
                setLux(x+1,y-2,1, true);
                setLux(x-1,y-2,1, true);
          //Add random foliage
            } else if (biome_code == 1) {
                if (rb(0.025)) {
                    setSprite(x, y, 4);
                    //setAnimated(x, y, true);
                } else if (rb(0.05)) {
                    setSprite(x, y, 5);
                    //setAnimated(x, y, true);
                }
            }
        }
    }
  //Grow the map naturally for a bit
    for (uint16_t g = 0; g < GEN_GROW_MAP; ++g) {
        //growMap();
    }
}



void growMap (uint16_t grow_speed, uint16_t death_speed)
{
  //Giveuth
    for (uint16_t g = 0; g < grow_speed; ++g) {
      //Select a random position on the map
        uint16_t grow_X, grow_Y;
        random_coord(MAP_W, MAP_H, grow_X, grow_Y);
        uint8_t sprite_code = getSprite(grow_X, grow_Y);
      //If it's growable, grow it
        uint8_t spread = 0;
        switch (sprite_code) {
            case 4: spread = 1 * rb(.6); break;
            case 5: spread = 2 * rb(1); break;
        }
        if (spread && getBiome(grow_X, grow_Y) == 1) {
            grow_X += ri(-spread, spread);
            grow_Y += ri(-spread, spread);
            if (!getSprite(grow_X, grow_Y)) {
                setSprite(grow_X, grow_Y, sprite_code);
            }
        }
    }
  //Takeuth awae
    for (uint16_t d = 0; d < death_speed; ++d) {
      //Select a random position on the map
        uint16_t grow_X, grow_Y;
        random_coord(MAP_W, MAP_H, grow_X, grow_Y);
        uint8_t sprite_code = getSprite(grow_X, grow_Y);
      //If it's killable, kill it
        bool to_kill = 0;
        switch (sprite_code) {
            case 4: to_kill = rb(.3); break;
            case 5: to_kill = rb(1); break;
        }
        if (to_kill) {
            setSprite(grow_X, grow_Y, 0);
        }
    }
}
