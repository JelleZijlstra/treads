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
};

enum class BlockSpecial {
  None = 0,
  Points,
  ExtraLife,

  // these are converted to Flags
  Indestructible,
  Immovable,
  Bomb,
  Bouncy,
  CreatesMonsters,

  // these are also used in special_type_to_frames_remaining
  Invincibility,
  Speed,
  TimeStop,
  ThrowBombs,
  KillsMonsters,
};

BlockSpecial special_for_name(const char* name);
const char* name_for_special(BlockSpecial special);
const char* display_name_for_special(BlockSpecial special);

struct Monster {
  enum Flag {
    IsPlayer         = 0x0001, // used for checking flags when blocks run over
    CanPushBlocks    = 0x0002, // ignored for players (they can always push)
    CanDestroyBlocks = 0x0004, // ignored for players (they can always destroy)
    BlocksPlayers    = 0x0008, // collides with players
    BlocksMonsters   = 0x0010, // collides with monsters
    Squishable       = 0x0020, // dies when block hits it
    KillsPlayers     = 0x0040, // kills players on contact
    KillsMonsters    = 0x0080, // kills monsters on contact
    Invincible       = 0x0100, // cannot be killed for any reason
  };
  static const char* name_for_flag(int64_t f);

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

  Monster() = delete;
  Monster(int64_t x, int64_t y, int64_t flags);

  std::string str() const;

  bool has_special(BlockSpecial special) const;
  void add_special(BlockSpecial special, int64_t frames);
  void attenuate_and_delete_specials();

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

  float decay_rate; // [0, 1]
  float integrity; // [0, 1]; when it reaches 0 the block is deleted

  BlockSpecial special;
  int64_t flags;

  Block() = delete;
  Block(int64_t x, int64_t y, BlockSpecial special = BlockSpecial::None,
      int64_t flags = Flag::Pushable | Flag::Destructible | Flag::KillsPlayers | Flag::KillsMonsters);

  std::string str() const;

  bool has_flags(uint64_t flags) const;
  bool has_any_flags(uint64_t flags) const;
  void set_flags(uint64_t flags);
  void clear_flags(uint64_t flags);
};

class LevelState {
public:
  struct GenerationParameters {
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

    bool power_monsters_can_push;
    bool power_monsters_become_creators;

    int64_t player_move_speed;
    int64_t basic_monster_move_speed;
    int64_t power_monster_move_speed;
    int64_t push_speed;
    int64_t bounce_speed_absorption;
    float block_destroy_rate;
  };

  LevelState() = delete;
  LevelState(const GenerationParameters& params);

  // checks that the level will behave properly when exec_frame is called
  void validate() const;

  int64_t get_grid_pitch() const;
  int64_t get_w() const;
  int64_t get_h() const;

  const std::shared_ptr<Monster> get_player() const;
  const std::unordered_set<std::shared_ptr<Monster>>& get_monsters() const;
  const std::unordered_set<std::shared_ptr<Block>>& get_blocks() const;

  float get_updates_per_second() const;
  int64_t get_frames_executed() const;

  int64_t count_monsters_with_flags(uint64_t flags, uint64_t mask) const;

  // executes a single update to the level state
  struct FrameEvents {
    int64_t events_mask;
    struct ScoreInfo {
      int64_t score;
      int64_t lives;
      BlockSpecial bonus;
      std::shared_ptr<Monster> monster;
      std::shared_ptr<Monster> killed; // may be NULL (if score came from a bonus)

      ScoreInfo(std::shared_ptr<Monster> monster,
          std::shared_ptr<Monster> killed = NULL, int64_t score = 0,
          int64_t lives = 0, BlockSpecial bonus = BlockSpecial::None);
    };
    std::vector<ScoreInfo> scores;

    FrameEvents();
    FrameEvents& operator|=(const FrameEvents& other);
  };
  FrameEvents exec_frame(int64_t player_control_impulse);

private:
  int64_t grid_pitch;
  int64_t w;
  int64_t h;

  std::shared_ptr<Monster> player;
  std::unordered_set<std::shared_ptr<Monster>> monsters;
  std::unordered_set<std::shared_ptr<Block>> blocks;

  float updates_per_second;
  int64_t frames_executed;

  // checks if the given position is a multiple of the grid pitch
  bool is_aligned(int64_t pos) const;
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
