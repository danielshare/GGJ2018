#include <SFML/Graphics.hpp> //For SFML graphics
#include "map.cpp"
#include "Entity.cpp"


sf::Text txt_float;


const uint8_t TILE_SCALE = 32;
const uint8_t TILE_W = 64, TILE_H = 32;
const uint8_t SPRITE_W = 64, SPRITE_H = 64;
const uint16_t WINDOW_W = 1024, WINDOW_H = 512;

const uint16_t mm_size = WINDOW_W / 8; //Size of minimap on the screen
const uint16_t mm_diag_width = sqrt(pow(mm_size, 2) + pow(mm_size, 2)); //Width of minimap rotated 45deg
const uint32_t mm_len = MAP_A * 4;
const uint8_t mm_crosshair = 2; //Half width of crosshair, in tiles
sf::Uint8* mm = new sf::Uint8[mm_len]; //Pixel data of the minimap
sf::Texture mm_tex;
sf::RectangleShape minimap (sf::Vector2f(MAP_W, MAP_H));

float sky_darkness = 0;
const uint16_t daynight_cycle = 2000;
float daynight_colour (uint32_t game_time, uint16_t x, uint16_t y) {
    float lux = getLux(x, y);
    sky_darkness = (1 - (fabs(sin(float(game_time + daynight_cycle) / daynight_cycle)) * .8));
    lux = sky_darkness + (lux / 5);
    return (lux < 1 ? lux : 1);
}

void getBiomeTex (uint16_t x, uint16_t y, uint8_t &biome_code, uint16_t &tex_X, uint16_t &tex_Y)
{
    biome_code = getBiome(x, y);
    tex_X = biome_code * TILE_W;
    tex_Y = 0;
}
void getSpriteTex (uint8_t sprite_code, uint16_t x, uint16_t y, uint16_t &tex_X, uint16_t &tex_Y)
{
    tex_X = getFrame(x, y) * SPRITE_W;
    tex_Y = (sprite_code - 1) * SPRITE_H;
}

void getVillagerTex (Entity* e, uint16_t &tex_X, uint16_t &tex_Y)
{
    if (e->frame > 4) { e->frame = 1; }
    //Calculate the angle to show the sprite
    uint16_t a = normaliseAng(e->rot - 23);
    a /= 45; //Every 45 degrees, the next texture in the sheet is used (eg. 90 becomes 2)
    tex_X = ENTITY_W * e->frame;
    tex_Y = ENTITY_H * a;
}
void getZombieTex (Entity* e, uint16_t &tex_X, uint16_t &tex_Y)
{
    if (e->is_dead) {
        tex_X = ENTITY_W * e->frame;
        tex_Y = ENTITY_H * 8;
        return;
    }
    if (e->frame > 4) { e->frame = 1; }
    //Calculate the angle to show the sprite
    uint16_t a = normaliseAng(e->rot - 23);
    a /= 45; //Every 45 degrees, the next texture in the sheet is used (eg. 90 becomes 2)
    tex_X = ENTITY_W * e->frame;
    tex_Y = ENTITY_H * a;
}




void drawBiome (Entity* prot, uint32_t game_time, sf::RenderWindow &window, sf::Sprite &biomeTile, uint16_t x, uint16_t y, double draw_X, double draw_Y)
{
    uint32_t *mapPtr = &map[x][y];
  //Prepare biome for draw
    //Get biome texture
    uint8_t biome_code;
    uint16_t tex_X, tex_Y;
    getBiomeTex(x, y, biome_code, tex_X, tex_Y);
    //Set biome Position
    biomeTile.setTextureRect(sf::IntRect(tex_X, tex_Y, TILE_W, TILE_H));
    biomeTile.setPosition(sf::Vector2f(draw_X, draw_Y));
    //Modulate biome
    uint8_t r = 255, g = 255, b = 255;
    switch (biome_code) {
        case B_STONE: //Stone
        {
            uint8_t c = pi((x*y), 0, 20);
            r = 235 + c;
            g = 235 + c;
            b = 235 + c;
        }
            break;
        case B_GRASS: //Grass
            r = 210 + fabs( sin(x * y / 20) ) * 40;
            g = 225 + fabs( sin(x * y / 33) ) * 30;
            b = 255;
            break;
        case B_WATER: //Water
            r = 255;
            g = 255;
            b = 200 + fabs( sin(((x * y / 20) + ((float)game_time / 100))) ) * 55;
            break;
    }
    //Modulate for day/night
    r *= daynight_colour(game_time, x, y);
    g *= daynight_colour(game_time, x, y);
    b *= daynight_colour(game_time, x, y);
    biomeTile.setColor(sf::Color(r, g, b));
    //Modulate if protag pos
    if (x == (int16_t)prot->pos_X && y == (int16_t)prot->pos_Y) {
        biomeTile.setColor(sf::Color(255, 0, 255));
    }
    //Draw biome
    window.draw(biomeTile);
}

void drawSprite (Entity* prot, uint32_t game_time, sf::RenderWindow &window, sf::Sprite &spriteTile, uint16_t x, uint16_t y, double draw_X, double draw_Y)
{
    uint32_t *mapPtr = &map[x][y];
    uint8_t sprite_code = getSprite(x, y);
    if (sprite_code) {
      //Prepare sprite for draw
        uint16_t tex_X, tex_Y;
        getSpriteTex(sprite_code, x, y, tex_X, tex_Y);
        //Set sprite position
        spriteTile.setTextureRect(sf::IntRect(tex_X, tex_Y, SPRITE_W, SPRITE_H));
        spriteTile.setPosition(sf::Vector2f(draw_X, draw_Y - SPRITE_H/2));
        //Modulation
        uint8_t r = 255, g = 255, b = 255;
        //If foliage, modulate with a distinctive colour
        if (isFoliage(sprite_code)) {
            uint8_t l = pi(x * y + 3, 0, 50);
            r = pi(x * y    , 200, 255) - l;
            g = pi(x * y + 1, 200, 255) - l;
            b = pi(x * y + 2, 200, 255) - l;
        }
        //Modulate for day/night
        r *= daynight_colour(game_time, x, y);
        g *= daynight_colour(game_time, x, y);
        b *= daynight_colour(game_time, x, y);
        spriteTile.setColor(sf::Color(r, g, b));
        //Draw sprite
        window.draw(spriteTile);
    }
}

void doISOMETRIC (Entity* prot, uint32_t game_time, sf::RenderWindow &window, void (*drawer)(Entity* prot, uint32_t, sf::RenderWindow&, sf::Sprite&, uint16_t, uint16_t, double, double), sf::Sprite tile)
{
  //Calculate map crop (as map coords)
    int16_t tiles_X, tiles_Y, camera_X1, camera_Y1, camera_X2, camera_Y2, camera_W, camera_H;
    tiles_X = (WINDOW_W / TILE_SCALE);
    tiles_Y = (WINDOW_H / TILE_SCALE);
    camera_X1 = uint16_t(prot->pos_X) - tiles_X/2;
    camera_Y1 = uint16_t(prot->pos_Y) - tiles_Y ;
    camera_X2 = camera_X1 + tiles_X + 2;
    camera_Y2 = camera_Y1 + tiles_Y*2 + 2;
    camera_W = camera_X2 - camera_X1;
    camera_H = camera_Y2 - camera_Y1;
  //Prepare isometric loop
    float p_X_d = decimal(prot->pos_X);
    float p_Y_d = decimal(prot->pos_Y);
    float p_X_D = 1 - p_X_d;
    float p_Y_D = 1 - p_Y_d;
    float protag_offset_X = ( (p_Y_D * TILE_H) + (p_X_D * (TILE_W/2)) );     //
    float protag_offset_Y = ( (p_Y_D * (TILE_H/2)) + (p_X_d * (TILE_H/2)) ); // Calculate protag decimal offset
    double draw_X = protag_offset_X, draw_Y = -(TILE_H * (tiles_Y / 2)) + protag_offset_Y;
    draw_X += TILE_W * (tiles_X/4);
    draw_Y -= TILE_H*2;
    double start_draw_X = draw_X, start_draw_Y = draw_Y;
  //Start isometric loop
    for (int16_t y = camera_Y1; y < camera_Y2; ++y) {
      for (int16_t x = camera_X2; x >= camera_X1; --x) {
          if (draw_X > -TILE_W && draw_X < WINDOW_W + TILE_W && draw_Y > -TILE_H && draw_Y < WINDOW_H + TILE_H) {
            //Prepare and call upon the argument drawer function
              (*drawer)(prot, game_time, window, tile, x, y, draw_X, draw_Y);
          }
        //Move half left and half down
          draw_X -= TILE_W / 2;
          draw_Y += TILE_H / 2;
      }
    //Move half right and half down
      start_draw_X += (TILE_W / 2);
      start_draw_Y += (TILE_H / 2);
      draw_X = start_draw_X;
      draw_Y = start_draw_Y;
    }
}

void drawEntities (Entity* prot, uint32_t game_time, sf::RenderWindow &window, sf::Sprite villagerTile, sf::Sprite zombieTile)
{
    double tiles_X, tiles_Y, camera_X1, camera_Y1, camera_X2, camera_Y2, camera_W, camera_H;
    tiles_X = (WINDOW_W / TILE_SCALE);
    tiles_Y = (WINDOW_H / TILE_SCALE);
    camera_X1 = prot->pos_X - tiles_X/2;
    camera_Y1 = prot->pos_Y - tiles_Y ;
    camera_X2 = camera_X1 + tiles_X + 2;
    camera_Y2 = camera_Y1 + tiles_Y*2 + 2;
    camera_W = camera_X2 - camera_X1;
    camera_H = camera_Y2 - camera_Y1;

    for (uint16_t e = 0; e < entity.size(); ++e) {
        double x = entity[e]->pos_X, y = entity[e]->pos_Y;
        if (x > camera_X1 && y > camera_Y1 && x < camera_X2 && y < camera_Y2) {
            double draw_X, draw_Y;
            draw_X = (x - camera_X1); //Tiles across X
            draw_Y = (y - camera_Y1); //Tiles across Y
            //In order to position correctly:
            double e_offset_X = 0, e_offset_Y = 0;
            //First go right and down according to the Y coord
            e_offset_X += (draw_Y * (TILE_W/2));
            e_offset_Y += (draw_Y * (TILE_H/2));
            //Then go left and down according to the X coord
            e_offset_X -= ((16 - draw_X) * (TILE_W/2));
            e_offset_Y += ((16 - draw_X) * (TILE_H/2));
            draw_X = e_offset_X - ENTITY_W/2;
            draw_Y = e_offset_Y - ENTITY_H;
            uint16_t tex_X, tex_Y;
            //Modulation
            uint8_t r = 255, g = 255, b = 255;
            //Modulate for day/night
            r *= daynight_colour(game_time, x, y);
            g *= daynight_colour(game_time, x, y);
            b *= daynight_colour(game_time, x, y);
            sf::Color color (r, g, b);
            //Texture & Display
            switch (entity[e]->type) {
                case 0: //Villager
                    getVillagerTex(entity[e], tex_X, tex_Y);
                    villagerTile.setTextureRect(sf::IntRect(tex_X, tex_Y, ENTITY_W, ENTITY_H));
                    villagerTile.setPosition(sf::Vector2f(draw_X, draw_Y));
                    villagerTile.setColor(color);
                    //Draw sprite
                    window.draw(villagerTile);
                    break;
                case 1: //Zombie
                    getZombieTex(entity[e], tex_X, tex_Y);
                    zombieTile.setTextureRect(sf::IntRect(tex_X, tex_Y, ENTITY_W, ENTITY_H));
                    zombieTile.setPosition(sf::Vector2f(draw_X, draw_Y));
                    zombieTile.setColor(color);
                    //Draw sprite
                    window.draw(zombieTile);
                    break;
            }
          //Draw floating text
            if (!entity[e]->is_dead) {
                txt_float.setPosition(sf::Vector2f(draw_X, draw_Y));
                txt_float.setString(std::to_string(uint16_t(entity[e]->health_score / 255 * 100)));
                txt_float.setFillColor(color);
                window.draw(txt_float);
            }
        }
    }
}

void drawProjectiles (Entity* prot, uint32_t game_time, sf::RenderWindow &window, sf::CircleShape projectileTile)
{
    double tiles_X, tiles_Y, camera_X1, camera_Y1, camera_X2, camera_Y2, camera_W, camera_H;
    tiles_X = (WINDOW_W / TILE_SCALE);
    tiles_Y = (WINDOW_H / TILE_SCALE);
    camera_X1 = prot->pos_X - tiles_X/2;
    camera_Y1 = prot->pos_Y - tiles_Y ;
    camera_X2 = camera_X1 + tiles_X + 2;
    camera_Y2 = camera_Y1 + tiles_Y*2 + 2;
    camera_W = camera_X2 - camera_X1;
    camera_H = camera_Y2 - camera_Y1;

    for (uint16_t p = 0; p < projectile.size(); ++p) {
        double x = projectile[p]->pos_X, y = projectile[p]->pos_Y;
        if (x > camera_X1 && y > camera_Y1 && x < camera_X2 && y < camera_Y2) {
            double draw_X, draw_Y;
            draw_X = (x - camera_X1); //Tiles across X
            draw_Y = (y - camera_Y1); //Tiles across Y
            //In order to position correctly:
            double e_offset_X = 0, e_offset_Y = 0;
            //First go right and down according to the Y coord
            e_offset_X += (draw_Y * (TILE_W/2));
            e_offset_Y += (draw_Y * (TILE_H/2));
            //Then go left and down according to the X coord
            e_offset_X -= ((16 - draw_X) * (TILE_W/2));
            e_offset_Y += ((16 - draw_X) * (TILE_H/2));
            draw_X = e_offset_X - ENTITY_W/2;
            draw_Y = e_offset_Y - ENTITY_H/2;
            uint16_t tex_X, tex_Y;
            //Position & Display
            projectileTile.setPosition(sf::Vector2f(draw_X, draw_Y));
            if (projectile[p]->was_successful) {
                projectileTile.setFillColor(sf::Color(255, 0, 0, projectile[p]->opacity * 255));
            } else {
                projectileTile.setFillColor(sf::Color(0, 0, 0, projectile[p]->opacity * 255));
            }
            window.draw(projectileTile);
        }
    }
}

void doDISPLAY (Entity* prot, uint32_t game_time, sf::RenderWindow &window, sf::Sprite &biomeTile, sf::Sprite &spriteTile, sf::Sprite &villagerTile, sf::Sprite &zombieTile, sf::CircleShape projectileTile, sf::Text txt_float, sf::Text txt_HUD, bool is_lazy = false)
{
    window.clear(sf::Color(255, 255, 255));

    doISOMETRIC(prot, game_time, window, drawBiome, biomeTile);
    doISOMETRIC(prot, game_time, window, drawSprite, spriteTile);

    drawEntities(prot, game_time, window, villagerTile, zombieTile);
    drawProjectiles(prot, game_time, window, projectileTile);

    window.draw(txt_HUD);

  //Lazy actions
    if (is_lazy) {
        uint32_t p = 0; //Pixel pointer for minimap
        for (uint16_t y = 0; y < MAP_H; ++y) {
            for (uint16_t x = 0; x < MAP_W; ++x) {
                if (getAnimated(x, y)) {
                    uint8_t frame = getFrame(x, y);
                    setFrame(x, y, (frame < 6 ? frame + 1 : 0));
                }
              //Render minimap
                if (x >= prot->pos_X - mm_crosshair && x <= prot->pos_X + mm_crosshair || y >= prot->pos_Y - mm_crosshair && y <= prot->pos_Y + mm_crosshair) {
                    mm[p] = 255;
                    mm[p + 1] = mm[p + 2] = 0;
                    mm[p + 3] = 255;
                } else {
                    uint16_t tex_X, tex_Y;
                    uint8_t biome_code = getBiome(x, y);
                    uint8_t sprite_code = getSprite(x, y);
                    sf::Color c;
                    if (sprite_code) {
                        switch (sprite_code) {
                            case 1: c = sf::Color(128, 84, 0); break; //Crate
                            case 2: c = sf::Color(0, 0, 0); break; //Brick
                            case 3: c = sf::Color::Yellow; break; //Campfire
                            default:
                                if (isFoliage(sprite_code)) {
                                    c = sf::Color(0, 64, 0); //Foliage
                                }
                                break;
                        }
                    } else {
                        switch (biome_code) {
                            case B_WATER: c = sf::Color::Blue; break;
                            case B_STONE: c = sf::Color(100, 64, 0); break;
                            case B_GRASS: c = sf::Color(0, 128, 0); break;
                            case B_SAND: c = sf::Color::Yellow; break;
                        }
                    }
                    mm[p]     = c.r;
                    mm[p + 1] = c.g;
                    mm[p + 2] = c.b;
                    mm[p + 3] = 255;
                }
                p += 4;
            }
        }
    }
    //Draw minimap
    mm_tex.update(mm);
    minimap.setTexture(&mm_tex);
    window.draw(minimap);


    window.display();
}
