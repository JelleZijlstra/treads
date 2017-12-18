#include "level.hh"

#include <inttypes.h>
#include <stdint.h>
#include <stdlib.h>

#include <phosg/Strings.hh>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

using namespace std;


static int64_t random_int(int64_t low, int64_t high) {
  return low + (rand() % (high + 1 - low));
}

static int64_t random_int(const pair<int64_t, int64_t>& bounds) {
  return random_int(bounds.first, bounds.second);
}


static int64_t sgn(int64_t x) {
    return (0 < x) - (x < 0);
}

static const vector<Impulse> all_directions({
  Impulse::Left,
  Impulse::Right,
  Impulse::Down,
  Impulse::Up,
});

Impulse opposite_direction(Impulse i) {
  if (i == Impulse::Left) {
    return Impulse::Right;
  }
  if (i == Impulse::Right) {
    return Impulse::Left;
  }
  if (i == Impulse::Up) {
    return Impulse::Down;
  }
  if (i == Impulse::Down) {
    return Impulse::Up;
  }
  return Impulse::None;
}

Impulse collapse_direction(int64_t impulses) {
  if (impulses & Impulse::Left) {
    return Impulse::Left;
  }
  if (impulses & Impulse::Right) {
    return Impulse::Right;
  }
  if (impulses & Impulse::Up) {
    return Impulse::Up;
  }
  if (impulses & Impulse::Down) {
    return Impulse::Down;
  }
  return Impulse::None;
}

pair<int64_t, int64_t> offsets_for_direction(Impulse direction) {
  if (direction == Impulse::Left) {
    return make_pair(-1, 0);
  } else if (direction == Impulse::Right) {
    return make_pair(1, 0);
  } else if (direction == Impulse::Up) {
    return make_pair(0, -1);
  } else if (direction == Impulse::Down) {
    return make_pair(0, 1);
  }
  throw invalid_argument("direction is not valid");
}

BlockSpecial special_for_name(const char* name) {
  if (!strcmp(name, "Points")) {
    return BlockSpecial::Points;
  } else if (!strcmp(name, "ExtraLife")) {
    return BlockSpecial::ExtraLife;
  } else if (!strcmp(name, "SkipLevels")) {
    return BlockSpecial::SkipLevels;
  } else if (!strcmp(name, "Indestructible")) {
    return BlockSpecial::Indestructible;
  } else if (!strcmp(name, "IndestructibleAndImmovable")) {
    return BlockSpecial::IndestructibleAndImmovable;
  } else if (!strcmp(name, "Immovable")) {
    return BlockSpecial::Immovable;
  } else if (!strcmp(name, "Brittle")) {
    return BlockSpecial::Brittle;
  } else if (!strcmp(name, "Bomb")) {
    return BlockSpecial::Bomb;
  } else if (!strcmp(name, "Bouncy")) {
    return BlockSpecial::Bouncy;
  } else if (!strcmp(name, "BouncyBomb")) {
    return BlockSpecial::BouncyBomb;
  } else if (!strcmp(name, "CreatesMonsters")) {
    return BlockSpecial::CreatesMonsters;
  } else if (!strcmp(name, "Invincibility")) {
    return BlockSpecial::Invincibility;
  } else if (!strcmp(name, "Speed")) {
    return BlockSpecial::Speed;
  } else if (!strcmp(name, "TimeStop")) {
    return BlockSpecial::TimeStop;
  } else if (!strcmp(name, "ThrowBombs")) {
    return BlockSpecial::ThrowBombs;
  } else if (!strcmp(name, "KillsMonsters")) {
    return BlockSpecial::KillsMonsters;
  } else {
    throw invalid_argument(string_printf("unknown block special name: %s", name));
  }
}

const char* name_for_special(BlockSpecial special) {
  switch (special) {
    case BlockSpecial::Points:
      return "Points";
    case BlockSpecial::ExtraLife:
      return "ExtraLife";
    case BlockSpecial::SkipLevels:
      return "SkipLevels";
    case BlockSpecial::Indestructible:
      return "Indestructible";
    case BlockSpecial::IndestructibleAndImmovable:
      return "IndestructibleAndImmovable";
    case BlockSpecial::Immovable:
      return "Immovable";
    case BlockSpecial::Brittle:
      return "Brittle";
    case BlockSpecial::Bomb:
      return "Bomb";
    case BlockSpecial::Bouncy:
      return "Bouncy";
    case BlockSpecial::BouncyBomb:
      return "BouncyBomb";
    case BlockSpecial::CreatesMonsters:
      return "CreatesMonsters";
    case BlockSpecial::Invincibility:
      return "Invincibility";
    case BlockSpecial::Speed:
      return "Speed";
    case BlockSpecial::TimeStop:
      return "TimeStop";
    case BlockSpecial::ThrowBombs:
      return "ThrowBombs";
    case BlockSpecial::KillsMonsters:
      return "KillsMonsters";
    default:
      throw invalid_argument("unknown block special type");
  }
}

const char* display_name_for_special(BlockSpecial special) {
  switch (special) {
    case BlockSpecial::Points:
      return "Points";
    case BlockSpecial::ExtraLife:
      return "Extra Life";
    case BlockSpecial::SkipLevels:
      return "Skip Levels";
    case BlockSpecial::Indestructible:
      return "Indestructible";
    case BlockSpecial::IndestructibleAndImmovable:
      return "Indestructible and Immovable";
    case BlockSpecial::Immovable:
      return "Immovable";
    case BlockSpecial::Brittle:
      return "Brittle";
    case BlockSpecial::Bomb:
      return "Bomb";
    case BlockSpecial::Bouncy:
      return "Bouncy";
    case BlockSpecial::BouncyBomb:
      return "BouncyBomb";
    case BlockSpecial::CreatesMonsters:
      return "Monster Generator";
    case BlockSpecial::Invincibility:
      return "Invincibility";
    case BlockSpecial::Speed:
      return "Speed";
    case BlockSpecial::TimeStop:
      return "Time Stop";
    case BlockSpecial::ThrowBombs:
      return "Bombs";
    case BlockSpecial::KillsMonsters:
      return "Rampage";
    default:
      throw invalid_argument("unknown block special type");
  }
}

static string name_for_flags(int64_t f, const char* (*name_for_flag)(int64_t)) {
  string ret;
  while (f) {
    int64_t next_flags = f & (f - 1);
    int64_t this_flag = f ^ next_flags;
    if (!ret.empty()) {
      ret += ',';
    }
    ret += name_for_flag(this_flag);
    f = next_flags;
  }
  if (ret.empty()) {
    ret = "None";
  }
  return ret;
}



const char* Monster::name_for_flag(int64_t f) {
  switch (f) {
    case 0:
      return "None";
    case Flag::IsPlayer:
      return "IsPlayer";
    case Flag::CanPushBlocks:
      return "CanPushBlocks";
    case Flag::CanDestroyBlocks:
      return "CanDestroyBlocks";
    case Flag::BlocksPlayers:
      return "BlocksPlayers";
    case Flag::BlocksMonsters:
      return "BlocksMonsters";
    case Flag::Squishable:
      return "Squishable";
    case Flag::KillsPlayers:
      return "KillsPlayers";
    case Flag::KillsMonsters:
      return "KillsMonsters";
    default:
      return "<InvalidFlag>";
  }
}

Monster::Monster(int64_t x, int64_t y, int64_t flags) :
    death_frame(-1), x(x), y(y), x_speed(0), y_speed(0), move_speed(4),
    push_speed(8), block_destroy_rate(0.02), integrity(0),
    facing_direction(Impulse::Up), control_impulse(0), flags(flags) {
  // players always have integroty = 1.0 so they can move at the level start
  if (this->has_flags(Flag::IsPlayer)) {
    this->integrity = 1.0;
  }
}

string Monster::str() const {
  string flags_str = name_for_flags(this->flags, this->name_for_flag);
  return string_printf("<Monster: x=%" PRId64 " y=%" PRId64 " x_speed=%" PRId64
      " y_speed=%" PRId64 " move_speed=%" PRId64 " push_speed=%" PRId64
      " facing_direction=%" PRIu64 " control_impulse=%" PRIu64 " flags=%s>",
      this->x, this->y, this->x_speed, this->y_speed, this->move_speed,
      this->push_speed, static_cast<int64_t>(this->facing_direction),
      this->control_impulse, flags_str.c_str());
}

bool Monster::has_special(BlockSpecial special) const {
  return this->special_to_frames_remaining.count(special);
}

void Monster::add_special(BlockSpecial special, int64_t frames) {
  switch (special) {
    case BlockSpecial::TimeStop:
    case BlockSpecial::ThrowBombs:
      // these don't affect the monster's flags/params at all
      break;
    case BlockSpecial::Invincibility:
      this->set_flags(Monster::Flag::Invincible);
      break;
    case BlockSpecial::Speed: {
      auto emplace_ret = this->special_to_frames_remaining.emplace(special, 300);
      if (emplace_ret.second) {
        // it didn't have the special already; increase its move speed
        this->move_speed *= 2;
        this->push_speed *= 2;
        this->block_destroy_rate *= 2;
      } else {
        // it did already have the special; just change the timeout
        emplace_ret.first->second = 300;
      }
      break;
    }
    case BlockSpecial::KillsMonsters:
      // if the monster already kills monsters, then this bonus does nothing
      if (!this->has_flags(Monster::Flag::KillsMonsters) ||
          this->has_special(BlockSpecial::KillsMonsters)) {
        this->set_flags(Monster::Flag::KillsMonsters);
        this->special_to_frames_remaining[special] = 300; // 10 seconds
      }
      break;
    default:
      throw logic_error("unimplemented special addition action");
  }

  this->special_to_frames_remaining[special] = frames;
}

void Monster::attenuate_and_delete_specials() {
  for (auto special_it = this->special_to_frames_remaining.begin();
       special_it != this->special_to_frames_remaining.end();) {
    special_it->second--;
    if (special_it->second == 0) {
      switch (special_it->first) {
        case BlockSpecial::TimeStop:
        case BlockSpecial::ThrowBombs:
          // these don't affect the monster's flags/params at all
          break;
        case BlockSpecial::KillsMonsters:
          this->clear_flags(Monster::Flag::KillsMonsters);
          break;
        case BlockSpecial::Invincibility:
          this->clear_flags(Monster::Flag::Invincible);
          break;
        case BlockSpecial::Speed:
          this->move_speed /= 2;
          this->push_speed /= 2;
          this->block_destroy_rate /= 2;
          break;
        default:
          throw logic_error("unimplemented special removal action");
      }
      special_it = this->special_to_frames_remaining.erase(special_it);
    } else {
      special_it++;
    }
  }
}

bool Monster::has_flags(uint64_t flags) const {
  return (this->flags & flags) == flags;
}

bool Monster::has_any_flags(uint64_t flags) const {
  return (this->flags & flags) != 0;
}

void Monster::set_flags(uint64_t flags) {
  this->flags |= flags;
}

void Monster::clear_flags(uint64_t flags) {
  this->flags &= ~flags;
}



const char* Block::name_for_flag(int64_t f) {
  switch (f) {
    case 0:
      return "None";
    case Flag::Pushable:
      return "Pushable";
    case Flag::Destructible:
      return "Destructible";
    case Flag::Bouncy:
      return "Bouncy";
    case Flag::KillsPlayers:
      return "KillsPlayers";
    case Flag::KillsMonsters:
      return "KillsMonsters";
    case Flag::IsBomb:
      return "IsBomb";
    case Flag::DelayedBomb:
      return "DelayedBomb";
    default:
      return "<InvalidFlag>";
  }
}

Block::Block(int64_t x, int64_t y, BlockSpecial special, int64_t flags) :
    x(x), y(y), x_speed(0), y_speed(0), owner(NULL),
    monsters_killed_this_push(0), bounce_speed_absorption(2), bomb_speed(16),
    decay_rate(0.0), integrity(1.0), special(special), flags(flags),
    frames_until_next_monster(0) { }

string Block::str() const {
  string flags_str = name_for_flags(this->flags, this->name_for_flag);
  return string_printf("<Block: x=%" PRId64 " y=%" PRId64 " x_speed=%" PRId64
      " y_speed=%" PRId64 " decay_rate=%g integrity=%g special=%" PRIu64
      " flags=%s>", this->x, this->y, this->x_speed, this->y_speed,
      this->decay_rate, this->integrity, static_cast<int64_t>(this->special),
      flags_str.c_str());
}

bool Block::has_flags(uint64_t flags) const {
  return (this->flags & flags) == flags;
}

bool Block::has_any_flags(uint64_t flags) const {
  return (this->flags & flags) != 0;
}

void Block::set_flags(uint64_t flags) {
  this->flags |= flags;
}

void Block::clear_flags(uint64_t flags) {
  this->flags &= ~flags;
}



Explosion::Explosion(int64_t x, int64_t y, float decay_rate) : x(x), y(y),
    decay_rate(decay_rate), integrity(1.5) { }

string Explosion::str() const {
  return string_printf("<Explosion: x=%" PRId64 " y=%" PRId64 ">", this->x,
      this->y);
}



LevelState::LevelState(const GenerationParameters& params) : params(params),
    updates_per_second(30.0f), frames_executed(0), frames_between_monsters(300) {

  // the player is a monster, technically
  uint64_t player_flags = Monster::Flag::IsPlayer | Monster::Flag::CanPushBlocks | Monster::Flag::CanDestroyBlocks | (params.player_squishable ? Monster::Flag::Squishable : 0);
  this->player.reset(new Monster(params.player_x, params.player_y, player_flags));
  this->monsters.emplace(this->player);

  // set player parameters
  this->player->block_destroy_rate = params.block_destroy_rate;
  this->player->move_speed = params.player_move_speed;
  this->player->push_speed = params.push_speed;

  // create blocks according to the block map
  int64_t w_cells = this->params.w / this->params.grid_pitch;
  int64_t h_cells = this->params.h / this->params.grid_pitch;
  if (params.block_map.size() != w_cells * h_cells) {
    throw invalid_argument("block map size doesn\'t match level dimensions");
  }
  for (int64_t y = 0; y < this->params.h / this->params.grid_pitch; y++) {
    for (int64_t x = 0; x < this->params.w / this->params.grid_pitch; x++) {
      int64_t z = y * (this->params.w / this->params.grid_pitch) + x;
      if (params.block_map[z]) {
        auto& block = *this->blocks.emplace(new Block(x * this->params.grid_pitch, y * this->params.grid_pitch)).first;
        block->bounce_speed_absorption = params.bounce_speed_absorption;
        block->bomb_speed = params.bomb_speed;
      }
    }
  }

  // replace some blocks with monsters until there are enough of them (the +1 is
  // necessary because the player is already in the monster set)
  int64_t basic_monster_count = random_int(params.basic_monster_count);
  int64_t power_monster_count = random_int(params.power_monster_count);
  while (this->monsters.size() < basic_monster_count + power_monster_count + 1) {
    auto block_it = this->blocks.begin();
    advance(block_it, rand() % this->blocks.size()); // O(n), sigh...

    bool is_power_monster = (this->monsters.size() >= basic_monster_count + 1);
    auto& monster = *this->monsters.emplace(new Monster((*block_it)->x,
        (*block_it)->y, this->flags_for_monster(is_power_monster))).first;
    monster->block_destroy_rate = params.block_destroy_rate;
    monster->move_speed = is_power_monster ? params.power_monster_move_speed :
        params.basic_monster_move_speed;
    monster->push_speed = params.push_speed;
    block_it = this->blocks.erase(block_it);
  }

  // now apply the block specials. again, taking advantage of unordered-ness
  auto remaining_blocks = this->blocks;
  for (const auto& special_it : params.special_type_to_count) {
    int64_t count = random_int(special_it.second);
    for (size_t x = 0; x < count; x++) {
      if (remaining_blocks.size() == 0) {
        return; // all blocks have specials? wow
      }
      auto block_it = remaining_blocks.begin();
      advance(block_it, rand() % remaining_blocks.size()); // O(n), sigh...

      auto& block = *block_it;
      block->special = special_it.first;

      switch (block->special) {
        case BlockSpecial::None:
          break; // just leave this out of the map, doofus

        case BlockSpecial::CreatesMonsters:
          block->frames_until_next_monster = this->frames_between_monsters;
          block->owner = this->player;
        case BlockSpecial::Points:
        case BlockSpecial::ExtraLife:
        case BlockSpecial::SkipLevels:
        case BlockSpecial::Invincibility:
        case BlockSpecial::Speed:
        case BlockSpecial::TimeStop:
        case BlockSpecial::ThrowBombs:
        case BlockSpecial::KillsMonsters:
          break;

        case BlockSpecial::Indestructible:
          block->clear_flags(Block::Flag::Destructible);
          break;

        case BlockSpecial::IndestructibleAndImmovable:
          block->clear_flags(Block::Flag::Pushable | Block::Flag::Destructible);
          break;

        case BlockSpecial::Immovable:
          block->clear_flags(Block::Flag::Pushable);
          break;

        case BlockSpecial::Brittle:
          block->set_flags(Block::Flag::Brittle);
          break;

        case BlockSpecial::Bouncy:
          block->set_flags(Block::Flag::Bouncy);
          break;

        case BlockSpecial::Bomb:
          block->set_flags(Block::Flag::IsBomb);
          break;

        case BlockSpecial::BouncyBomb:
          block->set_flags(Block::Flag::IsBomb | Block::Flag::DelayedBomb);
          break;
      }

      remaining_blocks.erase(block_it);
    }
  }

  // note: if you add more logic here, watch out for the return statement in the
  // above double loop
}

void LevelState::validate() const {
  // things to check:
  // 1. grid_pitch isn't zero, and evenly divides w and h
  // 2. no blocks overlap or are outside the level boundaries
  // 3. move_speed and push_speed for all monsters divides grid_pitch evenly,
  //    but push_speed can be 0 if the monster can't push

  // (1) w, h, grid_pitch
  if (this->params.grid_pitch == 0) {
    throw invalid_argument("grid pitch is zero");
  }
  if ((this->params.w == 0) || (this->params.h == 0)) {
    throw invalid_argument("one or both of the level dimensions is zero");
  }
  if ((this->params.w % this->params.grid_pitch) || (this->params.h % params.grid_pitch)) {
    throw invalid_argument(
        "level dimension is not a multiple of the grid pitch");
  }

  // (2) check that no blocks overlap or are outside the boundaries
  // O(n^2), sigh
  for (const auto& block : this->blocks) {
    if ((block->x < 0) || (block->x > this->params.w - this->params.grid_pitch) ||
        (block->y < 0) || (block->y > this->params.h - this->params.grid_pitch)) {
      string block_str = block->str();
      throw invalid_argument(string_printf("%s is outside of the boundary",
          block_str.c_str()));
    }
    for (const auto& other_block : this->blocks) {
      if (block == other_block) {
        continue;
      }
      if (this->check_stationary_collision(block->x, block->y, other_block->x,
          other_block->y)) {
        string block_str = block->str();
        string other_block_str = other_block->str();
        throw invalid_argument(string_printf("%s overlaps with %s",
            block_str.c_str(), other_block_str.c_str()));
      }
    }
  }

  // (3) check that no monsters are outside the boundaries (unlike blocks,
  // monsters may overlap)
  for (const auto& monster : this->monsters) {
    if ((monster->x < 0) || (monster->x > this->params.w - this->params.grid_pitch) ||
        (monster->y < 0) || (monster->y > this->params.h - this->params.grid_pitch)) {
      string monster_str = monster->str();
      throw invalid_argument(string_printf("%s is outside of the boundary",
          monster_str.c_str()));
    }
  }

  // (3) check move_speed and push_speed for all monsters
  for (const auto& monster : this->monsters) {
    if (this->params.grid_pitch % monster->move_speed) {
      auto monster_str = monster->str();
      throw invalid_argument(string_printf(
          "%s has invalid move speed (%" PRId64 " does not divide %" PRIu64")",
          monster_str.c_str(), monster->move_speed, this->params.grid_pitch));
    }
    if ((monster->push_speed == 0) && (monster->has_flags(Monster::Flag::CanPushBlocks))) {
      auto monster_str = monster->str();
      throw invalid_argument(string_printf("%s has no push speed but can push",
          monster_str.c_str()));
    }
    if (monster->push_speed && (this->params.grid_pitch % monster->push_speed)) {
      auto monster_str = monster->str();
      throw invalid_argument(string_printf(
          "%s has invalid push speed (%" PRId64 " does not divide %" PRIu64")",
          monster_str.c_str(), monster->move_speed, this->params.grid_pitch));
    }
  }
}

const LevelState::GenerationParameters& LevelState::get_params() const {
  return this->params;
}

const std::shared_ptr<Monster> LevelState::get_player() const {
  return this->player;
}
const std::unordered_set<std::shared_ptr<Monster>>& LevelState::get_monsters() const {
  return this->monsters;
}
const std::unordered_set<std::shared_ptr<Block>>& LevelState::get_blocks() const {
  return this->blocks;
}

const std::unordered_set<std::shared_ptr<struct Explosion>>& LevelState::get_explosions() const {
  return this->explosions;
}

float LevelState::get_updates_per_second() const {
  return this->updates_per_second;
}
int64_t LevelState::get_frames_executed() const {
  return this->frames_executed;
}
int64_t LevelState::get_frames_between_monsters() const {
  return this->frames_between_monsters;
}

int64_t LevelState::count_monsters_with_flags(uint64_t flags, uint64_t mask) const {
  int64_t count = 0;
  for (const auto& monster : this->monsters) {
    if ((monster->death_frame >= 0) || ((monster->flags & mask) != flags)) {
      continue;
    }
    count++;
  }
  return count;
}

int64_t LevelState::count_blocks_with_special(BlockSpecial special) const {
  int64_t count = 0;
  for (const auto& block : this->blocks) {
    if (block->special != special) {
      continue;
    }
    count++;
  }
  return count;
}

double LevelState::current_score_proportion() const {
  int64_t score_end_frame = this->updates_per_second * 120;
  if (this->frames_executed >= score_end_frame) {
    return 0.0;
  }
  return 1.0 - (static_cast<double>(this->frames_executed) / static_cast<double>(score_end_frame));
}

int64_t LevelState::score_for_monster(bool is_power_monster) const {
  int64_t base = is_power_monster ? this->params.power_monster_score : this->params.basic_monster_score;
  return this->current_score_proportion() * base;
}

uint64_t LevelState::flags_for_monster(bool is_power_monster) const {
  return Monster::Flag::Squishable | Monster::Flag::KillsPlayers |
      (is_power_monster ? Monster::Flag::IsPower : 0) |
      ((is_power_monster && this->params.power_monsters_can_push) ? Monster::Flag::CanPushBlocks : 0);
}

bool LevelState::is_aligned(int64_t pos) const {
  return (pos % this->params.grid_pitch) == 0;
}

bool LevelState::is_within_bounds(int64_t x, int64_t y) const {
  return (x >= 0) && (x <= this->params.w - this->params.grid_pitch) &&
         (y >= 0) && (y <= this->params.h - this->params.grid_pitch);
}

shared_ptr<Block> LevelState::find_block(int64_t x, int64_t y) {
  for (auto& block : this->blocks) {
    if ((block->x == x) && (block->y == y)) {
      return block;
    }
  }
  return NULL;
}

bool LevelState::space_is_empty(int64_t x, int64_t y) const {
  // if any part of the space is out of bounds, it's not empty
  if ((x < 0) || (y < 0) || (x >= this->params.w) || (y >= this->params.h)) {
    return false;
  }

  int64_t x_min = x - this->params.grid_pitch;
  int64_t y_min = y - this->params.grid_pitch;
  int64_t x_max = x + this->params.grid_pitch;
  int64_t y_max = y + this->params.grid_pitch;
  for (auto& block : this->blocks) {
    if ((block->x > x_min) && (block->x < x_max) &&
        (block->y > y_min) && (block->y < y_max)) {
      return false;
    }
  }
  return true;
}

bool LevelState::check_stationary_collision(int64_t this_x, int64_t this_y,
    int64_t other_x, int64_t other_y) const {
  return ((llabs(this_x - other_x) < this->params.grid_pitch) &&
          (llabs(this_y - other_y) < this->params.grid_pitch));
}

bool LevelState::check_moving_collision(int64_t this_x, int64_t this_y,
    int64_t this_x_speed, int64_t this_y_speed, int64_t other_x,
    int64_t other_y) const {
  if (this_x_speed) {
    int64_t new_x = this_x + this_x_speed;

    // if the monster is too far up or down, there's no collision
    if ((other_y >= this_y + this->params.grid_pitch) ||
        (other_y + this->params.grid_pitch <= this_y)) {
      return false;
    }

    // if the object is moving left, check its left edge against the other
    // object's entirety. otherwise, check its right edge
    if (this_x_speed < 0) {
      return (new_x < other_x + this->params.grid_pitch) && (new_x > other_x);
    } else {
      return (new_x + this->params.grid_pitch < other_x + this->params.grid_pitch) &&
             (new_x + this->params.grid_pitch > other_x);
    }

  } else if (this_y_speed) {
    // same as above, but in the other dimension
    int64_t new_y = this_y + this_y_speed;

    // if the monster is too far up or down, there's no collision
    if ((other_x >= this_x + this->params.grid_pitch) ||
        (other_x + this->params.grid_pitch <= this_x)) {
      return false;
    }

    if (this_y_speed < 0) {
      return (new_y < other_y + this->params.grid_pitch) && (new_y > other_y);
    } else {
      return (new_y + this->params.grid_pitch < other_y + this->params.grid_pitch) &&
             (new_y + this->params.grid_pitch > other_y);
    }

  }

  return false;
}

LevelState::FrameEvents::ScoreInfo::ScoreInfo(shared_ptr<Monster> monster,
    shared_ptr<Monster> killed, int64_t score, int64_t lives, int64_t skip_levels,
    BlockSpecial bonus, int64_t block_x, int64_t block_y) : score(score),
    lives(lives), skip_levels(skip_levels), bonus(bonus), block_x(block_x),
    block_y(block_y), monster(monster), killed(killed) { }

string LevelState::FrameEvents::ScoreInfo::str() const {
  return string_printf("ScoreInfo(score=%" PRId64 ", lives=%" PRId64
      ", skip_levels=%" PRId64 ", bonus=%" PRId64 ", block_x=%" PRId64
      ", block_y=%" PRId64 ", monster=%p, killed=%p)",
      this->score, this->lives, this->skip_levels, this->bonus, this->block_x,
      this->block_y, this->monster.get(), this->killed.get());
}

LevelState::FrameEvents::FrameEvents() : events_mask(0) { }

LevelState::FrameEvents& LevelState::FrameEvents::operator|=(
    const FrameEvents& other) {
  this->events_mask |= other.events_mask;
  this->scores.insert(this->scores.end(), other.scores.begin(),
      other.scores.end());
  return *this;
}

LevelState::FrameEvents LevelState::exec_frame(int64_t impulses) {
  // executes a single frame. the order of actions is as follows:
  // 1. set player and monster control impulses (essentially, everyone decides
  //    what they want to do on this frame). also change the directions that
  //    players and monsters are facing if they're aligned
  // 2. any push/destroy impulses are applied to blocks
  // 3. blocks decay according to their decay rates
  // 4. monster specials attenuate and eventually disappear
  // 5. blocks are moved
  // 6. players are moved
  // 7. moster generators attenuate, and generate monsters if necessary
  // 8. explosions attenuate
  // this order means that we never have to e.g. find out where a block/monster
  // used to be before executing this frame, since everything that depends on
  // alignment (especially step 2) happens before any motion is applied.

  // prepare for randomness
  static random_device rd;
  static mt19937 g(rd());

  // collect events that occurred during this frame (this is used for playing
  // sounds)
  FrameEvents ret;
  ret.events_mask = Event::NoEvents;

  // figure out which monsters are allowed to move
  unordered_set<shared_ptr<Monster>> time_stop_holders;
  for (const auto& monster : this->monsters) {
    if (monster->has_special(BlockSpecial::TimeStop)) {
      time_stop_holders.emplace(monster);
    }
  }

  // (step 1) the player's control impulse is given by the caller
  if (this->player) {
    this->player->control_impulse = impulses;
  }

  // (step 1) monsters update their impulses if their integrity is 1.0. if it's
  // not 1.0, their integrity increases a little
  for (auto& monster : this->monsters) {
    if (monster->death_frame >= 0) {
      continue; // dead monsters tell no tales
    }
    if (monster->integrity < 1) {
      monster->integrity += 0.01;
      continue; // can't move
    }
    if (!time_stop_holders.empty() && !time_stop_holders.count(monster)) {
      continue; // this monster is held by a time stop
    }

    bool is_player = monster->has_flags(Monster::Flag::IsPlayer);
    if (is_player) {
      monster->control_impulse = impulses;
    } else {
      // if the monster isn't aligned, don't bother - it can't move anyway
      if (!this->is_aligned(monster->x) || !this->is_aligned(monster->y)) {
        continue;
      }

      // figure out in which directions the monster can move
      uint8_t available_directions = Impulse::None;
      if (this->space_is_empty(monster->x - this->params.grid_pitch, monster->y)) {
        available_directions |= Impulse::Left;
      }
      if (this->space_is_empty(monster->x + this->params.grid_pitch, monster->y)) {
        available_directions |= Impulse::Right;
      }
      if (this->space_is_empty(monster->x, monster->y - this->params.grid_pitch)) {
        available_directions |= Impulse::Up;
      }
      if (this->space_is_empty(monster->x, monster->y + this->params.grid_pitch)) {
        available_directions |= Impulse::Down;
      }

      // if there are no directions available, do nothing
      if (available_directions == Impulse::None) {
        continue;
      }

      // if there's only one direction available, go that way
      if ((available_directions & (available_directions - 1)) == 0) {
        monster->control_impulse = available_directions;

      } else {
        // if there are multiple directions available, forbid the direction that
        // the monster just came from, then choose a direction at random
        available_directions &= ~(opposite_direction(monster->facing_direction));
        Impulse direction_order[] = {Impulse::Left, Impulse::Right, Impulse::Up,
            Impulse::Down};
        shuffle(direction_order, direction_order + 4, g);
        for (auto check_direction : direction_order) {
          if (available_directions & check_direction) {
            monster->control_impulse = check_direction;
            break;
          }
        }
      }
    }

    // make the monster face in the impulse direction and update its speed if
    // it's aligned
    bool apply_impulse = false;
    if (is_player) {
      // unlike monsters, players can turn around mid-cell
      Impulse new_direction = collapse_direction(monster->control_impulse);
      if (((new_direction == Impulse::Left) || (new_direction == Impulse::Right)) &&
          this->is_aligned(monster->y)) {
        apply_impulse = true;
      }
      if (((new_direction == Impulse::Up) || (new_direction == Impulse::Down)) &&
          this->is_aligned(monster->x)) {
        apply_impulse = true;
      }
      if ((new_direction == Impulse::None) && this->is_aligned(monster->x) && this->is_aligned(monster->y)) {
        apply_impulse = true;
      }
    } else {
      apply_impulse = this->is_aligned(monster->x) && this->is_aligned(monster->y);
    }
    if (apply_impulse) {
      Impulse new_direction = collapse_direction(monster->control_impulse);
      if (new_direction == Impulse::None) {
        monster->x_speed = 0;
        monster->y_speed = 0;
      } else {
        monster->facing_direction = new_direction;
        if (monster->facing_direction == Impulse::Left) {
          monster->x_speed = -monster->move_speed;
          monster->y_speed = 0;
        } else if (monster->facing_direction == Impulse::Right) {
          monster->x_speed = monster->move_speed;
          monster->y_speed = 0;
        } else if (monster->facing_direction == Impulse::Up) {
          monster->x_speed = 0;
          monster->y_speed = -monster->move_speed;
        } else if (monster->facing_direction == Impulse::Down) {
          monster->x_speed = 0;
          monster->y_speed = monster->move_speed;
        }
      }
    }
  }

  // (step 2) apply push impulses appropriately
  for (auto& monster : this->monsters) {
    if (monster->death_frame >= 0) {
      continue; // dead monsters tell no tales
    }
    if (!time_stop_holders.empty() && !time_stop_holders.count(monster)) {
      continue; // this monster is held by a time stop
    }

    // the following conditions must be satisfied for a monster to push a block:
    // 1. the monster has a Push control impulse
    // 2. there is a block in the next aligned position in the direction the
    //    monster is facing
    // 3. the monster's position is currently aligned (this happens after the
    //    previous check since monsters may throw bombs when not aligned)
    // 4. this block is not already moving
    // if all of the above are satisfied, the block is pushed only if the entire
    // space beyond it is empty; otherwise, the block is destroyed.

    // (2.1) check if the monster has a Push impulse
    if (!(monster->control_impulse & Impulse::Push)) {
      continue;
    }

    // (2.2) check if there's a block in the appropriate spot
    auto offsets = offsets_for_direction(monster->facing_direction);
    auto block = this->find_block(monster->x + offsets.first * this->params.grid_pitch,
                                  monster->y + offsets.second * this->params.grid_pitch);
    if (!block.get()) {
      // (2.2.1) no block; the monster throws a bomb if it has ThrowBombs and
      // there are two empty cells in front of it
      // TODO: do this more efficiently by not calling space_is_empty (and
      // iterating all blocks) twice
      if (monster->has_special(BlockSpecial::ThrowBombs) &&
          this->space_is_empty(monster->x + offsets.first * this->params.grid_pitch,
                               monster->y + offsets.second * this->params.grid_pitch) &&
          this->space_is_empty(monster->x + offsets.first * 2 * this->params.grid_pitch,
                               monster->y + offsets.second * 2 * this->params.grid_pitch)) {
        int64_t bomb_x = monster->x + offsets.first * (this->params.grid_pitch + monster->push_speed);
        int64_t bomb_y = monster->y + offsets.second * (this->params.grid_pitch + monster->push_speed);
        auto block = *this->blocks.emplace(new Block(bomb_x, bomb_y,
            BlockSpecial::Bomb)).first;
        block->set_flags(Block::Flag::IsBomb);
        block->x_speed = offsets.first * monster->push_speed;
        block->y_speed = offsets.second * monster->push_speed;
        block->owner = monster;
        block->bomb_speed = monster->push_speed;
      }
      continue; // there's no block to push
    }

    // (2.3) check if the monster's position is aligned
    if (!this->is_aligned(monster->x) || !this->is_aligned(monster->y)) {
      continue;
    }

    // (2.4) check if the block is already moving
    if (block->x_speed || block->y_speed) {
      continue;
    }

    // (2.5) check if there's space behind the block; push it if so
    ret |= this->apply_push_impulse(block, monster, monster->facing_direction,
        monster->push_speed);
  }

  // (step 3) update decaying blocks
  for (auto block_it = this->blocks.begin(); block_it != this->blocks.end();) {
    (*block_it)->integrity -= (*block_it)->decay_rate;

    // if the block has no integrity left, delete it
    if ((*block_it)->integrity <= 0.0) {
      block_it = this->blocks.erase(block_it);
    } else {
      block_it++;
    }
  }

  // (step 4) update monster specials
  for (auto& monster : this->monsters) {
    monster->attenuate_and_delete_specials();
  }

  // (step 5) moving blocks slide until they hit something that blocks them,
  // squishing things that get in their way and are squishable
  for (auto& block : this->blocks) {
    // if it's not moving, it won't hit anything
    if ((block->x_speed == 0) && (block->y_speed == 0)) {
      continue;
    }

    bool collision = false;

    // (5.1) check for collisions with the level edges (this will cause it to
    // stop or bounce)
    // (5.1.1) left edge
    if (this->check_moving_collision(block->x, block->y, block->x_speed,
        block->y_speed, -this->params.grid_pitch, block->y)) {
      block->x = 0;
      block->x_speed = -block->x_speed + block->bounce_speed_absorption * sgn(block->x_speed);
      collision = true;
      ret.events_mask |= block->x_speed ? Event::BlockBounced : Event::BlockStopped;
    }
    // (5.1.2) right edge
    if (this->check_moving_collision(block->x, block->y, block->x_speed,
        block->y_speed, this->params.w, block->y)) {
      block->x = this->params.w - this->params.grid_pitch;
      block->x_speed = -block->x_speed + block->bounce_speed_absorption * sgn(block->x_speed);
      collision = true;
      ret.events_mask |= block->x_speed ? Event::BlockBounced : Event::BlockStopped;
    }
    // (5.1.3) top edge
    if (this->check_moving_collision(block->x, block->y, block->x_speed,
        block->y_speed, block->x, -this->params.grid_pitch)) {
      block->y = 0;
      block->y_speed = -block->y_speed + block->bounce_speed_absorption * sgn(block->y_speed);
      collision = true;
      ret.events_mask |= block->y_speed ? Event::BlockBounced : Event::BlockStopped;
    }
    // (5.1.4) bottom edge
    if (this->check_moving_collision(block->x, block->y, block->x_speed,
        block->y_speed, block->x, this->params.h)) {
      block->y = this->params.h - this->params.grid_pitch;
      block->y_speed = -block->y_speed + block->bounce_speed_absorption * sgn(block->y_speed);
      collision = true;
      ret.events_mask |= block->y_speed ? Event::BlockBounced : Event::BlockStopped;
    }

    // (5.2) check for collisions with other blocks (this will cause it to stop
    // or bounce)
    for (auto& other_block : this->blocks) {
      if (block == other_block) {
        continue; // can't collide with itself, lolz
      }

      bool other_block_collision = this->check_moving_collision(block->x,
          block->y, block->x_speed, block->y_speed, other_block->x,
          other_block->y);
      if (other_block_collision) {
        // this block hit the other block; put this block right next to the
        // other block, and make it bounce away and maybe slow down. but
        // blocks can't stop on misaligned positions, so if the position is
        // misaligned, it always bounces elastically.
        if (block->x_speed) {
          bool elastic_bounce = (other_block->has_flags(Block::Flag::Bouncy)) ||
              !this->is_aligned(other_block->x);
          block->x = other_block->x - sgn(block->x_speed) * this->params.grid_pitch;
          block->x_speed = -block->x_speed + (!elastic_bounce) * block->bounce_speed_absorption * sgn(block->x_speed);
          ret.events_mask |= block->x_speed ? Event::BlockBounced : Event::BlockStopped;
        } else {
          bool elastic_bounce = (other_block->has_flags(Block::Flag::Bouncy)) ||
              !this->is_aligned(other_block->y);
          block->y = other_block->y - sgn(block->y_speed) * this->params.grid_pitch;
          block->y_speed = -block->y_speed + (!elastic_bounce) * block->bounce_speed_absorption * sgn(block->y_speed);
          ret.events_mask |= block->y_speed ? Event::BlockBounced : Event::BlockStopped;
        }
        collision = true;
      }
    }

    // (5.3) check for collisions with monsters (this will cause the block to
    // stop, bounce, or kill)
    for (auto& other_monster : this->monsters) {
      if (other_monster->death_frame >= 0) {
        continue; // dead monsters tell no tales
      }

      if (!this->check_moving_collision(block->x, block->y, block->x_speed,
          block->y_speed, other_monster->x, other_monster->y)) {
        continue;
      }

      // logic here is similar to the above loop
      if (other_monster->has_flags(Monster::Flag::Squishable) &&
          !other_monster->has_flags(Monster::Flag::Invincible)) {
        bool is_player = other_monster->has_flags(Monster::Flag::IsPlayer);
        bool is_power = other_monster->has_flags(Monster::Flag::IsPower);
        block->monsters_killed_this_push++;
        other_monster->death_frame = this->frames_executed;
        ret.events_mask |= is_player ? Event::PlayerSquished : Event::MonsterSquished;
        ret.scores.emplace_back(block->owner, other_monster,
            block->monsters_killed_this_push * this->score_for_monster(is_power));

      } else {
        if (block->x_speed) {
          bool elastic_bounce = !this->is_aligned(other_monster->x);
          block->x = other_monster->x - sgn(block->x_speed) * this->params.grid_pitch;
          block->x_speed = -block->x_speed + (!elastic_bounce) * block->bounce_speed_absorption * sgn(block->x_speed);
          ret.events_mask |= block->x_speed ? Event::BlockBounced : Event::BlockStopped;
        } else {
          bool elastic_bounce = !this->is_aligned(other_monster->y);
          block->y = other_monster->y - sgn(block->y_speed) * this->params.grid_pitch;
          block->y_speed = -block->y_speed + (!elastic_bounce) * block->bounce_speed_absorption * sgn(block->y_speed);
          ret.events_mask |= block->y_speed ? Event::BlockBounced : Event::BlockStopped;
        }
        collision = true;
      }
    }

    // (5.4) if collision is true, then we've already updated the block's
    // location, so we shouldn't move it incrementally
    if (!collision) {
      block->x += block->x_speed;
      block->y += block->y_speed;

    // (5.4.1) if the block collided and is a bomb and is aligned, it explodes.
    // if it's a bouncy bomb, it only explodes if it's stopped.
    } else if (block->has_flags(Block::Flag::IsBomb) &&
        this->is_aligned(block->x) && this->is_aligned(block->y) &&
        (!block->has_flags(Block::Flag::DelayedBomb) || ((block->x_speed == 0) && (block->y_speed == 0)))) {
      ret |= this->apply_explosion(block);
    }
  }

  // (step 6) monsters and players move according to their speeds; if they
  // collide with blocks they stop, if they collide with other monsters they may
  // stop, die, kill, or continue depending on their flags.
  for (auto& monster : this->monsters) {
    if (monster->death_frame >= 0) {
      continue; // dead monsters tell no tales
    }

    // TODO: this logic is very similar to the block logic; can we factor it out
    // somehow?

    // we can't ignore the case where the monster isn't moving like we can for
    // blocks. this is because a monster can be run over by a monster that kills
    // it, but only the current monster can be deleted during this loop, so we
    // have to check for collisions for the current monster anyway

    bool collision = false;

    // (6.1) check for collisions with the level edges (this will cause it to
    // stop)
    // (6.1.1) left edge
    if (this->check_moving_collision(monster->x, monster->y, monster->x_speed,
        monster->y_speed, -this->params.grid_pitch, monster->y)) {
      monster->x = 0;
      monster->x_speed = 0;
      collision = true;
    }
    // (6.1.2) right edge
    if (this->check_moving_collision(monster->x, monster->y, monster->x_speed,
        monster->y_speed, this->params.w, monster->y)) {
      monster->x = this->params.w - this->params.grid_pitch;
      monster->x_speed = 0;
      collision = true;
    }
    // (6.1.3) top edge
    if (this->check_moving_collision(monster->x, monster->y, monster->x_speed,
        monster->y_speed, monster->x, -this->params.grid_pitch)) {
      monster->y = 0;
      monster->y_speed = 0;
      collision = true;
    }
    // (6.1.4) bottom edge
    if (this->check_moving_collision(monster->x, monster->y, monster->x_speed,
        monster->y_speed, monster->x, this->params.h)) {
      monster->y = this->params.h - this->params.grid_pitch;
      monster->y_speed = 0;
      collision = true;
    }

    // (6.2) check for collisions with blocks (this will cause it to stop; we've
    // already checked for blocks running monsters over in step 4.3)
    for (auto& other_block : this->blocks) {
      bool other_block_collision = this->check_moving_collision(monster->x,
          monster->y, monster->x_speed, monster->y_speed, other_block->x,
          other_block->y);
      if (other_block_collision) {
        // this monster hit the block; put this monster right next to the block,
        // and make it slow down to the speed of the block. (we can't just make
        // it stop because monsters can't stop on misaligned positions.) but
        // don't let its speed increase or change signs.
        // TODO: can this be collapsed into something simpler?
        if (monster->x_speed) {
          monster->x = other_block->x - sgn(monster->x_speed) * this->params.grid_pitch;
          if (monster->x_speed < 0) { // monster moving left
            if (other_block->x_speed < 0) { // block moving left
              if (-other_block->x_speed < -monster->x_speed) { // block is slower
                monster->x_speed = other_block->x_speed;
              }
            } else { // block moving right
              if (other_block->x_speed < -monster->x_speed) { // block is slower
                monster->x_speed = -other_block->x_speed;
              }
            }
          } else { // monster moving right
            if (other_block->x_speed < 0) { // block moving left
              if (-other_block->x_speed < monster->x_speed) { // block is slower
                monster->x_speed = -other_block->x_speed;
              }
            } else { // block moving right
              if (other_block->x_speed < -monster->x_speed) { // block is slower
                monster->x_speed = other_block->x_speed;
              }
            }
          }

        } else {
          monster->y = other_block->y - sgn(monster->y_speed) * this->params.grid_pitch;
          if (monster->y_speed < 0) { // monster moving left
            if (other_block->y_speed < 0) { // block moving left
              if (-other_block->y_speed < -monster->y_speed) { // block is slower
                monster->y_speed = other_block->y_speed;
              }
            } else { // block moving right
              if (other_block->y_speed < -monster->y_speed) { // block is slower
                monster->y_speed = -other_block->y_speed;
              }
            }
          } else { // monster moving right
            if (other_block->y_speed < 0) { // block moving left
              if (-other_block->y_speed < monster->y_speed) { // block is slower
                monster->y_speed = -other_block->y_speed;
              }
            } else { // block moving right
              if (other_block->y_speed < -monster->y_speed) { // block is slower
                monster->y_speed = other_block->y_speed;
              }
            }
          }
        }
        collision = true;
      }
    }

    // (6.3) check for collisions with other monsters (this may cause the
    // monster to stop or die)
    bool is_player = monster->has_flags(Monster::Flag::IsPlayer);
    shared_ptr<Monster> killer;
    for (auto& other_monster : this->monsters) {
      if (other_monster->death_frame >= 0) {
        continue; // dead monsters tell no tales
      }

      // ignore self
      if (other_monster == monster) {
        continue;
      }

      // if the other monster kills us, then delete this monster. but if the
      // current monster is the player and has KillsMonsters or Invincible, then
      // it can't be killed even if the other monster has KillsPlayers
      bool other_monster_kills_monster = other_monster->has_flags(is_player ?
          Monster::Flag::KillsPlayers : Monster::Flag::KillsMonsters);
      bool monster_is_immune = is_player && monster->has_any_flags(
          Monster::Flag::KillsMonsters | Monster::Flag::Invincible);
      if (other_monster_kills_monster && !monster_is_immune) {
        if (this->check_stationary_collision(monster->x, monster->y,
            other_monster->x, other_monster->y)) {
          killer = other_monster;
          break;
        }

      // the other monster doesn't kill us; if it blocks us, then check for
      // collisions
      } else if (other_monster->has_flags(is_player ?
          Monster::Flag::BlocksPlayers : Monster::Flag::BlocksMonsters)) {
        if (this->check_moving_collision(monster->x, monster->y,
            monster->x_speed, monster->y_speed, other_monster->x,
            other_monster->y)) {
          if (monster->x_speed) {
            monster->x = other_monster->x - sgn(monster->x_speed) * this->params.grid_pitch;
            monster->x_speed = other_monster->x_speed;
          } else {
            monster->y = other_monster->y - sgn(monster->y_speed) * this->params.grid_pitch;
            monster->y_speed = other_monster->y_speed;
          }
        }
        collision = true;
      }
    }

    if (killer.get()) {
      ret.events_mask |= (monster->has_flags(Monster::Flag::IsPlayer)) ?
          Event::PlayerKilled : Event::MonsterKilled;
      monster->death_frame = this->frames_executed;
      bool is_power = monster->has_flags(Monster::Flag::IsPower);
      ret.scores.emplace_back(killer, monster, this->score_for_monster(is_power));
      continue;
    }

    // (6.4) if collision is true, then we've already updated the monster's
    // location, so we shouldn't move it incrementally. but don't move it if
    // it's caught in a time stop.
    if (!collision && (time_stop_holders.empty() || time_stop_holders.count(monster))) {
      monster->x += monster->x_speed;
      monster->y += monster->y_speed;
    }
  }

  // (7) attenuate monster generators
  for (auto block : this->blocks) {
    if ((block->special != BlockSpecial::CreatesMonsters) ||
        (block->integrity != 1.0)) {
      continue;
    }
    if (block->frames_until_next_monster == 0) {
      // figure out where the monster can go
      // TODO: don't iterate over all blocks 4 times, sigh
      vector<Impulse> candidate_directions;
      for (auto direction : all_directions) {
        auto offsets = offsets_for_direction(direction);
        // don't create a monster in the direction the block is moving (it would
        // just get smashed immediately)
        if (((offsets.first * block->x_speed) > 0) ||
            ((offsets.second * block->y_speed) > 0)) {
          continue;
        }
        int64_t target_x = block->x + offsets.first * this->params.grid_pitch;
        int64_t target_y = block->y + offsets.second * this->params.grid_pitch;
        if (!this->is_within_bounds(target_x, target_y)) {
          continue;
        }
        if (!this->space_is_empty(target_x, target_y)) {
          continue;
        }
        candidate_directions.emplace_back(direction);
      }

      if (candidate_directions.empty()) {
        // kaboom
        ret |= this->apply_explosion(block);
      } else {
        // create a monster
        int64_t which = random_int(0, candidate_directions.size() - 1);

        auto offsets = offsets_for_direction(candidate_directions[which]);
        int64_t target_x = block->x + offsets.first * this->params.grid_pitch;
        int64_t target_y = block->y + offsets.second * this->params.grid_pitch;

        bool is_power_monster = false; // TODO: should randomly choose
        auto& monster = *this->monsters.emplace(new Monster(
            target_x, target_y, this->flags_for_monster(is_power_monster))).first;
        monster->facing_direction = candidate_directions[which];
        monster->block_destroy_rate = this->params.block_destroy_rate;
        monster->move_speed = is_power_monster ? this->params.power_monster_move_speed : this->params.basic_monster_move_speed;
        monster->push_speed = this->params.push_speed;
        monster->x_speed = offsets.first * monster->move_speed;
        monster->y_speed = offsets.second * monster->move_speed;
        monster->integrity = 1.0;

        ret.events_mask |= Event::MonsterCreated;

        block->frames_until_next_monster = this->frames_between_monsters;
      }
    } else {
      block->frames_until_next_monster--;
    }
  }

  // (8) attenuate and delete explosions
  for (auto explosion_it = this->explosions.begin(); explosion_it != this->explosions.end();) {
    auto& explosion = *explosion_it;
    if (explosion->integrity >= 1.0) {
      explosion->integrity -= 0.5;
    } else {
      explosion->integrity -= explosion->decay_rate;
    }

    if (explosion->integrity <= 0.0) {
      explosion_it = this->explosions.erase(explosion_it);
    } else {
      explosion_it++;
    }
  }

  // increment frame counter and return the event mask
  this->frames_executed++;
  return ret;
}

LevelState::FrameEvents LevelState::apply_push_impulse(shared_ptr<Block> block,
    shared_ptr<Monster> responsible_monster, Impulse direction, int64_t speed) {
  LevelState::FrameEvents ret;

  block->owner = responsible_monster;

  auto offsets = offsets_for_direction(direction);
  if ((block->has_flags(Block::Flag::Pushable)) &&
      this->space_is_empty(block->x + offsets.first * this->params.grid_pitch,
                           block->y + offsets.second * this->params.grid_pitch)) {
    block->x_speed = offsets.first * speed;
    block->y_speed = offsets.second * speed;
    block->monsters_killed_this_push = 0;
    ret.events_mask |= Event::BlockPushed;

    if ((block->has_flags(Block::Flag::Brittle)) && (block->decay_rate == 0.0)) {
      if (responsible_monster.get()) {
        block->decay_rate = responsible_monster->block_destroy_rate;
      } else {
        block->decay_rate = 0.02;
      }
      ret.events_mask |= Event::BlockDestroyed;
    }

  } else if ((block->has_flags(Block::Flag::Destructible)) &&
             (block->decay_rate == 0.0)) {
    if (responsible_monster.get()) {
      block->decay_rate = responsible_monster->block_destroy_rate;
    } else {
      block->decay_rate = 0.02;
    }
    switch (block->special) {
      case BlockSpecial::Indestructible:
      case BlockSpecial::IndestructibleAndImmovable:
        throw logic_error("indestructible block was destroyed");

      case BlockSpecial::Bomb:
      case BlockSpecial::BouncyBomb:
        ret |= this->apply_explosion(block);
        break;

      case BlockSpecial::Points:
        if (responsible_monster.get()) {
          ret.scores.emplace_back(responsible_monster, nullptr,
              this->score_for_monster(false), 0, 0, BlockSpecial::None, block->x, block->y);
        }
      case BlockSpecial::None:
      case BlockSpecial::Immovable:
      case BlockSpecial::Brittle:
      case BlockSpecial::Bouncy:
        ret.events_mask |= Event::BlockDestroyed;
        break;

      case BlockSpecial::ExtraLife:
        if (responsible_monster.get()) {
          ret.scores.emplace_back(responsible_monster, nullptr, 0, 1, 0, BlockSpecial::None, block->x, block->y);
        }
        ret.events_mask |= Event::LifeCollected;
        break;

      case BlockSpecial::SkipLevels:
        if (responsible_monster.get()) {
          ret.scores.emplace_back(responsible_monster, nullptr, 0, 0, 4, BlockSpecial::None, block->x, block->y);
        }
        ret.events_mask |= Event::BonusCollected;
        break;

      case BlockSpecial::Invincibility:
      case BlockSpecial::Speed:
      case BlockSpecial::TimeStop:
      case BlockSpecial::ThrowBombs:
      case BlockSpecial::KillsMonsters:
        if (responsible_monster.get()) {
          responsible_monster->add_special(block->special, 300);
          ret.scores.emplace_back(responsible_monster, nullptr, 0, 0, 0, block->special, block->x, block->y);
        }
      case BlockSpecial::CreatesMonsters:
        ret.events_mask |= Event::BonusCollected;
    }
  }

  return ret;
}

LevelState::FrameEvents LevelState::apply_explosion(shared_ptr<Block> block) {
  FrameEvents ret;

  if (block->integrity <= 0.0) {
    return ret;
  }

  // hack: set the bomb block's integrity to zero so it gets deleted on the
  // next frame
  block->integrity = 0.0;
  ret.events_mask |= Event::Explosion;

  // make an explosion in place of the destroyed block
  this->explosions.emplace(new struct Explosion(block->x, block->y, 0.04));

  for (auto direction : all_directions) {
    auto offsets = offsets_for_direction(direction);
    int64_t target_x = block->x + offsets.first * this->params.grid_pitch;
    int64_t target_y = block->y + offsets.second * this->params.grid_pitch;
    if (!this->is_within_bounds(target_x, target_y)) {
      continue;
    }

    // make an explosion for the kaboom effect
    this->explosions.emplace(new struct Explosion(target_x, target_y, 0.05));

    auto target_block = this->find_block(target_x, target_y);
    if (target_block.get()) {
      if (!target_block->x_speed || !target_block->y_speed) {
        ret |= this->apply_push_impulse(target_block, block->owner, direction, block->bomb_speed);
      }
    } else {
      // note that we don't check for monsters if there was a block, since
      // monsters and blocks can't occupy the same space
      for (auto& monster : this->monsters) {
        if (monster->death_frame >= 0) {
          continue;
        }
        if (!this->check_stationary_collision(target_x, target_y, monster->x,
            monster->y)) {
          continue;
        }

        if (!monster->has_flags(Monster::Flag::Invincible)) {
          monster->death_frame = this->frames_executed;
          ret.events_mask |= monster->has_flags(Monster::Flag::IsPlayer)
              ? Event::PlayerSquished : Event::MonsterSquished;
          // TODO: we probably should have some kind of multiplier for killing
          // lots of monsters with one bomb push
          ret.scores.emplace_back(block->owner, monster, this->score_for_monster(false));
        }
      }
    }
  }

  return ret;
}
