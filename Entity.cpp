#include "sound.cpp"

#define E_VILLAGER 0
#define E_ZOMBIE   1

const uint8_t ENTITY_W = 32, ENTITY_H = 64;
const uint16_t GEN_VILLAGERS = 1024;
const uint16_t GEN_ZOMBIES = 256;
const float ANI_INTERVAL = 2;
const uint8_t SHOOT_DISTANCE = 16;
const uint8_t ATTACK_DISTANCE = 16;
const uint8_t MAX_HEALTH = 255;
const float NORMAL_SPEED = .02, ATTACK_SPEED = .1;
const uint8_t PROJECTILE_DAMAGE = 12;
const float PROJECTILE_SPEED = .25;
const float REWARD_HEALTH = 10;
const uint8_t SOUND_INTERVAL = 30;

class Entity;
std::vector<Entity*> entity = std::vector<Entity*>();
class Projectile;
std::vector<Projectile*> projectile = std::vector<Projectile*>();

class Projectile {
public:
  float vel_X, vel_Y;
  double pos_X, pos_Y;
  bool had_hit = false;
  bool was_successful = false;
  float opacity = 1;
  Entity* shooter;

  void move();
  Projectile(double, double, float, Entity*);
  ~Projectile();

};

class Entity {
  public:
    uint8_t type; // 0 Villager, 1 Zombie
    uint16_t index_in_array;
    bool is_dead = false;

    double pos_X = 0, pos_Y = 0; // Entity position
    uint16_t targ_X = 0, targ_Y = 0; //Target position
    float rot = 0; // As degrees
    uint8_t frame = 0;
    bool had_moved = true;

    uint8_t attack_timeout = 0;
    uint64_t targetted_at = 0; //Last time this Entity was targetted
    uint64_t prev_hurt = 0; //Last time this entity was hurt

    float health_score = 255, speed = NORMAL_SPEED, power_score = 1;

    Entity (uint8_t, double, double);
    Entity ();

    void think (bool);
    void moveTowards (uint16_t, uint16_t);
    bool tryDir (float, float);
    void move ();
    void harm (uint8_t);
    void animate ();
    void shoot(Entity*);
    void shootDir();
    void reward();

  private:
      void loiter ();
      void attack (Entity*);
      void lashOut ();
      Entity* target = NULL;
      float animate_clock = 0;
};

Entity::Entity (uint8_t type, double pos_X, double pos_Y)
{
    this->index_in_array = entity.size();
    this->type = type;
    this->pos_X = this->targ_X = pos_X;
    this->pos_Y = this->targ_X = pos_Y;
    this->animate_clock = ri(0, ANI_INTERVAL);
}

Entity::Entity () { }

void Entity::attack (Entity* who)
{
    target = who;
    target->targetted_at = game_time;
    speed = ATTACK_SPEED;
    attack_timeout = 4;
}

void Entity::reward()
{/*
  if(this->health_score + REWARD_HEALTH > MAX_HEALTH)
  {
    this->health_score = MAX_HEALTH;
  }
  else
  {
    this->health_score += REWARD_HEALTH;
}*/
}

void Entity::lashOut ()
{
    if (target->type != E_ZOMBIE) {
        target->harm(power_score);
        reward();
    } else {
        attack_timeout = 0;
    }
}

void Entity::harm (uint8_t damage)
{
    if (prev_hurt + SOUND_INTERVAL < game_time) {
        prev_hurt = game_time;
        if(type == E_VILLAGER)
        {
          playSound(1, rf(.75, 1.25), this->pos_X, this->pos_Y, entity[1]->pos_X, entity[1]->pos_Y);
        } else if (type == E_ZOMBIE) {
          playSound(2, rf(.75, 1.25), this->pos_X, this->pos_Y, entity[1]->pos_X, entity[1]->pos_Y);
        }
    }
    health_score -= damage;
    if (health_score < 0) {
        if (type == E_VILLAGER) {
            type = E_ZOMBIE;
            health_score = MAX_HEALTH;
        } else if (type == E_ZOMBIE) {
          playSound(3, rf(.75, 1.25), this->pos_X, this->pos_Y, entity[1]->pos_X, entity[1]->pos_Y);
            is_dead = true;
            health_score = 0;
            frame = 0;
            speed = NORMAL_SPEED*3; //For the animation
        }
    }
}

void Entity::loiter ()
{
    moveTowards(pos_X + ri(-3, 3), pos_Y + ri(-3, 3));
}

void Entity::think (bool is_nighttime)
{
    switch (type) {
        case 0: //Villager
          //Loiter
            if (rb(.4)) { loiter(); }
            // Find zombie to shoot at
              if(rb(.2)) {
                for (uint16_t e = 0; e < entity.size(); ++e)
                {
                  if (entity[e]->type != E_ZOMBIE || entity[e]->is_dead) { continue; }
                  if (eD_approx(pos_X, pos_Y, entity[e]->pos_X, entity[e]->pos_Y) < SHOOT_DISTANCE / (is_nighttime+1)) {
                    shoot(entity[e]);
                    break;
                  }
                }
              }
            break;
        case 1: //Zombie
            if (attack_timeout) {
                --attack_timeout;
                if (!attack_timeout) { //Stop attacking
                    target = NULL;
                    speed = NORMAL_SPEED;
                }
            } else {
              //Loiter
                loiter();
              //Find a Villager to attack
                for (uint16_t e = 0; e < entity.size(); ++e) {
                    if (entity[e]->type != E_VILLAGER || entity[e]->is_dead) { continue; }
                    if (eD_approx(pos_X, pos_Y, entity[e]->pos_X, entity[e]->pos_Y) < ATTACK_DISTANCE * (is_nighttime+1)) {
                        if (entity[e]->targetted_at + 50 > game_time) { continue; }
                        attack(entity[e]);
                        break;
                    }
                }
            }
            break;
    }
}

void Entity::moveTowards (uint16_t x, uint16_t y)
{
    targ_X = x;
    targ_Y = y;
}

bool Entity::tryDir (float dir_X, float dir_Y)
{
    float dist = eD_approx(pos_X, pos_Y, pos_X + dir_X, pos_Y + dir_Y);
    double d_X = dir_X * dist * speed;
    double d_Y = dir_Y * dist * speed;
    double new_X = pos_X + d_X, new_Y = pos_Y + d_Y;
    double check_X = new_X + dir_X, check_Y = new_Y + dir_Y;
    uint8_t check_sprite = getSprite(check_X, check_Y);
    if (!isSolid(check_sprite) && getBiome(check_X, check_Y) != B_WATER && !(type == E_VILLAGER && check_sprite == S_CAMPFIRE)) {
        pos_X = new_X;
        pos_Y = new_Y;
        if (check_sprite == S_CAMPFIRE) {
            harm(10);
        }
        return true;
    } else {
      //Try pushing outwards
        if (type == E_VILLAGER) { pushCrate(check_X, check_Y, d_X, d_Y); }
      //Trigger a loiter
        loiter();
        return false;
    }
}

void Entity::move ()
{
    if (attack_timeout) {
        targ_X = target->pos_X;
        targ_Y = target->pos_Y;
    }
    if (abs(uint16_t(pos_X + .5) - targ_X) || abs(uint16_t(pos_Y + .5) - targ_Y)) { //Need to move?
        rot = vecToAng(targ_X - pos_X, targ_Y - pos_Y);
        float dir_X, dir_Y;
        angToVec(rot, dir_X, dir_Y);
        if (tryDir(dir_X, dir_Y)) {
            had_moved = true;
        } else {
            frame = 0;
            targ_X = pos_X;
            targ_Y = pos_Y;
        }
    } else {
        if (attack_timeout) {
            lashOut();
        }
        frame = 0;
    }
    setMapEntity(uint16_t(pos_X), uint16_t(pos_Y), index_in_array);
}

void Entity::animate ()
{
    animate_clock += speed * 4;
    if (animate_clock > ANI_INTERVAL) {
        animate_clock = 0;
        if (is_dead) { if (frame < 4) { ++frame; } return; }
        if (had_moved) { ++frame; had_moved = false; }
    }
}

void Entity::shoot (Entity* victim)
{
  double dir_X, dir_Y;
  targToVec(this->pos_X, this->pos_Y, victim->pos_X, victim->pos_Y, dir_X, dir_Y);
  float dir_ang = vecToAng(dir_X, dir_Y);
  playSound(0, rf(.5, 1.5), this->pos_X, this->pos_Y, entity[1]->pos_X, entity[1]->pos_Y);
  this->rot = dir_ang;
  projectile.push_back(new Projectile(this->pos_X, this->pos_Y, dir_ang, this));
}

void Entity::shootDir ()
{
  playSound(0, rf(.5, 1.5), this->pos_X, this->pos_Y, entity[1]->pos_X, entity[1]->pos_Y);
  projectile.push_back(new Projectile(this->pos_X, this->pos_Y, this->rot, this));
}





Projectile::Projectile(double pos_X, double pos_Y, float rot, Entity* shooter) {
  this->pos_X = pos_X;
  this->pos_Y = pos_Y;
  this->shooter = shooter;
  angToVec(rot, vel_X, vel_Y);
  vel_X *= PROJECTILE_SPEED;
  vel_Y *= PROJECTILE_SPEED;
}

void Projectile::move() {
  pos_X += vel_X;
  pos_Y += vel_Y;
  if(isSolid(getSprite(pos_X, pos_Y))) {
    had_hit = true;
  } else {
    uint16_t e = getMapEntity(uint16_t(pos_X), uint16_t(pos_Y));
    Entity* here = entity[e];
    if (here->index_in_array && here->type == E_ZOMBIE && !here->is_dead && here->index_in_array != shooter->index_in_array) {
        if (uint16_t(here->pos_X) == uint16_t(pos_X) && uint16_t(here->pos_Y) == uint16_t(pos_Y)) {
          had_hit = was_successful = true;
          here->harm(PROJECTILE_DAMAGE);
          shooter->reward();
        }
    }
  }
}
