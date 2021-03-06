#include <stdint.h>

#include <memory>
#include <phosg/Strings.hh>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>



enum Impulse {
  None  = 0x00,
  Up    = 0x01,
  Down  = 0x02,
  Left  = 0x04,
  Right = 0x08,
  Push  = 0x10,
};

Impulse opposite_direction(Impulse i);
Impulse collapse_direction(int64_t impulses);
std::pair<int64_t, int64_t> offsets_for_direction(Impulse direction);

enum Event {
  NoEvents         = 0x0000,
  BlockPushed      = 0x0001,
  MonsterSquished  = 0x0002,
  MonsterKilled    = 0x0004,
  PlayerKilled     = 0x0008,
  BonusCollected   = 0x0010,
  BlockDestroyed   = 0x0020,
  BlockBounced     = 0x0040,
  Explosion        = 0x0080,
  BlockStopped     = 0x0100,
  PlayerSquished   = 0x0200,
  LifeCollected    = 0x0400,
  MonsterCreated   = 0x0800,
};

enum class BlockSpecial {
  // these don't do anything special when destroyed
  None = 0,
  Timer,
  LineUp,

  // these are converted to ScoreInfos when destroyed
  Points,
  ExtraLife,
  SkipLevels,

  // these are converted to Flags on creation
  Indestructible,
  IndestructibleAndImmovable,
  Immovable,
  Brittle, // breaks when moved
  Bomb,
  Bouncy,
  BouncyBomb,
  CreatesMonsters,

  // these are also used in special_type_to_frames_remaining
  Invincibility,
  Speed,
  TimeStop,
  ThrowBombs,
  KillsMonsters,
  Everything,
};

BlockSpecial special_for_name(const char* name);
const char* name_for_special(BlockSpecial special);
const char* display_name_for_special(BlockSpecial special);

struct Monster {
  enum Flag {
    IsPlayer         = 0x0001, // used for checking flags when blocks run over
    IsPower          = 0x0002, // used for determining score when killed
    CanPushBlocks    = 0x0004, // ignored for players (they can always push)
    CanDestroyBlocks = 0x0008, // ignored for players (they can always destroy)
    BlocksPlayers    = 0x0010, // collides with players
    BlocksMonsters   = 0x0020, // collides with monsters
    Squishable       = 0x0040, // dies when block hits it
    KillsPlayers     = 0x0080, // kills players on contact
    KillsMonsters    = 0x0100, // kills monsters on contact
    Invincible       = 0x0200, // cannot be killed for any reason
  };

  static const char* name_for_flag(int64_t f);

  enum class MovementPolicy {
    Player = 0,
    Straight, // keep going straight; turn when encounter an obstacle
    Random, // move randomly, but don't turn around unless necessary
    SeekPlayer, // use A* to find a path to the player
  };

  static MovementPolicy movement_policy_for_name(const char* name);
  static const char* name_for_movement_policy(MovementPolicy special);

  int64_t death_frame;

  int64_t x;
  int64_t y;
  int64_t x_speed;
  int64_t y_speed;

  // these speeds need to evenly divide the level's grid_pitch or else
  // movement and collisions won't work properly
  int64_t move_speed;
  int64_t push_speed;
  float block_destroy_rate;

  float integrity; // starts at 0, increases to 1, then monster can move

  std::unordered_map<BlockSpecial, int64_t> special_to_frames_remaining;

  Impulse facing_direction;

  // ths control impulse tells the Monster what to do next. not valid for the
  // player; the player's control impulse is passed into exec_frame instead.
  int64_t control_impulse;

  int64_t flags;
  MovementPolicy movement_policy;

  Monster() = delete;
  Monster(int64_t x, int64_t y, int64_t flags);

  std::string str() const;

  bool has_special(BlockSpecial special) const;
  void add_special(BlockSpecial special, int64_t frames);
  bool is_alive() const;
  void attenuate_and_delete_specials();
  void choose_random_direction(uint8_t available_directions);

  bool has_flags(uint64_t flags) const;
  bool has_any_flags(uint64_t flags) const;
  void set_flags(uint64_t flags);
  void clear_flags(uint64_t flags);
};

struct Block {
  enum Flag {
    Pushable      = 0x01, // can be moved
    Destructible  = 0x02, // can be crushed
    Bouncy        = 0x04, // other blocks don't slow down when running into it
    KillsPlayers  = 0x08, // kills players if it runs them over
    KillsMonsters = 0x10, // kills monsters if it runs them over
    IsBomb        = 0x20, // explodes instead of bouncing (unimplemented)
    Brittle       = 0x40, // automatically destroyed when pushed
    DelayedBomb   = 0x80, // only explodes when it stops moving
  };
  static const char* name_for_flag(int64_t f);

  int64_t x;
  int64_t y;
  int64_t x_speed;
  int64_t y_speed;

  // which monster pushed the block (and should get the points)
  std::shared_ptr<Monster> owner;
  int64_t monsters_killed_this_push;

  int64_t bounce_speed_absorption;
  int64_t bomb_speed;

  float decay_rate; // [0, 1]
  float integrity; // [0, 1]; when it reaches 0 the block is deleted

  BlockSpecial special;
  int64_t flags;
  int64_t frames_until_action;

  Block() = delete;
  Block(int64_t x, int64_t y, BlockSpecial special = BlockSpecial::None,
      int64_t flags = Flag::Pushable | Flag::Destructible | Flag::KillsPlayers | Flag::KillsMonsters);

  std::string str() const;

  void set_special(BlockSpecial special, int64_t timer_value);

  bool has_flags(uint64_t flags) const;
  bool has_any_flags(uint64_t flags) const;
  void set_flags(uint64_t flags);
  void clear_flags(uint64_t flags);
};

struct Explosion {
  int64_t x;
  int64_t y;

  float decay_rate; // [0, 1]
  float integrity; // [0, 1]; when it reaches 0 the explosion is deleted
  // note: integrity starts at 1.0, but drops to 0.5 after the first frame

  Explosion() = delete;
  Explosion(int64_t x, int64_t y, float decay_rate);

  std::string str() const;
};

class LevelState {
public:
  struct GenerationParameters {
    std::string name;

    int64_t grid_pitch;
    int64_t w; // must be a multiple if grid_pitch
    int64_t h; // must be a multiple if grid_pitch
    int64_t player_x; // must be a multiple if grid_pitch
    int64_t player_y; // must be a multiple if grid_pitch
    bool player_squishable;

    bool fixed_block_map;
    std::vector<bool> block_map;
    std::unordered_map<BlockSpecial, std::pair<int64_t, int64_t>> special_type_to_count;

    std::pair<int64_t, int64_t> basic_monster_count;
    std::pair<int64_t, int64_t> power_monster_count;

    int64_t basic_monster_score;
    int64_t power_monster_score;

    Monster::MovementPolicy basic_monster_movement_policy;
    Monster::MovementPolicy power_monster_movement_policy;

    bool power_monsters_can_push;
    bool power_monsters_become_creators;

    int64_t player_move_speed;
    int64_t basic_monster_move_speed;
    int64_t power_monster_move_speed;
    int64_t push_speed;
    int64_t bomb_speed;
    int64_t bounce_speed_absorption;
    float block_destroy_rate;
  };

  LevelState() = delete;
  LevelState(const GenerationParameters& params);

  // checks that the level will behave properly when exec_frame is called
  void validate() const;

  const std::shared_ptr<Monster> get_player() const;
  const std::unordered_set<std::shared_ptr<Monster>>& get_monsters() const;
  const std::unordered_set<std::shared_ptr<Block>>& get_blocks() const;
  const std::unordered_set<std::shared_ptr<struct Explosion>>& get_explosions() const;
  const GenerationParameters& get_params() const;

  float get_updates_per_second() const;
  int64_t get_frames_executed() const;
  int64_t get_frames_between_monsters() const;

  int64_t count_monsters_with_flags(uint64_t flags, uint64_t mask) const;
  int64_t count_blocks_with_special(BlockSpecial special) const;

  Impulse find_path(int64_t x, int64_t y, int64_t target_x, int64_t target_y) const;

  // executes a single update to the level state
  struct FrameEvents {
    int64_t events_mask;
    struct ScoreInfo {
      int64_t score;
      int64_t lives;
      int64_t skip_levels;
      BlockSpecial bonus;
      int64_t block_x;
      int64_t block_y;
      std::shared_ptr<Monster> monster;
      std::shared_ptr<Monster> killed; // may be NULL (if score came from a bonus)

      ScoreInfo(std::shared_ptr<Monster> monster,
          std::shared_ptr<Monster> killed = NULL, int64_t score = 0,
          int64_t lives = 0, int64_t skip_levels = 0,
          BlockSpecial bonus = BlockSpecial::None, int64_t block_x = 0,
          int64_t block_y = 0);

      std::string str() const;
    };
    std::vector<ScoreInfo> scores;

    FrameEvents();
    FrameEvents& operator|=(const FrameEvents& other);
  };
  FrameEvents exec_frame(int64_t player_control_impulse);

  double current_score_proportion() const;

private:
  GenerationParameters params;

  std::shared_ptr<Monster> player;
  std::unordered_set<std::shared_ptr<Monster>> monsters;
  std::unordered_set<std::shared_ptr<Block>> blocks;
  std::unordered_set<std::shared_ptr<struct Explosion>> explosions;

  float updates_per_second;
  int64_t frames_executed;

  int64_t frames_between_monsters;

  int64_t score_for_monster(bool is_power_monster, int64_t mult = 1) const;
  uint64_t flags_for_monster(bool is_power_monster) const;

  // checks if the given position is a multiple of the grid pitch
  bool is_aligned(int64_t z) const;
  // aligns a gicen position to the closest grid cell
  int64_t align(int64_t z) const;
  // checks if the given position is within the level
  bool is_within_bounds(int64_t x, int64_t y) const;

  // finds the block at the given exact position
  std::shared_ptr<Block> find_block(int64_t x, int64_t y);

  // checks if an entire block can fit at the given position without colliding
  // with another block. note that this does not check for monsters or players!
  bool space_is_empty(int64_t x, int64_t y) const;

  // checks if the given object collides with the other object
  bool check_stationary_collision(int64_t this_x, int64_t this_y,
      int64_t other_x, int64_t other_y) const;
  bool check_moving_collision(int64_t this_x, int64_t this_y,
      int64_t this_x_speed, int64_t this_y_speed, int64_t other_x,
      int64_t other_y) const;

  // pushes or destroys a block
  FrameEvents apply_push_impulse(std::shared_ptr<Block> block,
      std::shared_ptr<Monster> responsible_monster, Impulse direction,
      int64_t speed);
  // kaboom
  FrameEvents apply_explosion(std::shared_ptr<Block> block);
};
