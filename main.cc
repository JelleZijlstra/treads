#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#ifdef MACOSX
#include <CoreFoundation/CoreFoundation.h>
#endif

#include <GLFW/glfw3.h>

#include <deque>
#include <phosg/Filesystem.hh>
#include <phosg/Hash.hh>
#include <phosg/Image.hh>
#include <phosg/JSON.hh>
#include <phosg/Time.hh>
#include <stdexcept>
#include <string>
#include <vector>

#include "audio.hh"
#include "gl_text.hh"
#include "level.hh"
#include "maze.hh"

using namespace std;



// TODO: this is super ugly; clean this up
unordered_map<BlockSpecial, Image> special_to_image;

enum Phase {
  Playing = 0,
  Paused,
};



// TODO use projection matrix to make this unnecessary
static float to_window(float x, float w) {
  return ((x / w) * 2) - 1;
}



static void render_stripe_animation(int window_w, int window_h, int stripe_width,
    float br, float bg, float bb, float ba, float sr, float sg, float sb,
    float sa) {
  glBegin(GL_QUADS);
  glColor4f(br, bg, bb, ba);
  glVertex3f(-1.0f, -1.0f, 1.0f);
  glVertex3f(1.0f, -1.0f, 1.0f);
  glVertex3f(1.0f, 1.0f, 1.0f);
  glVertex3f(-1.0f, 1.0f, 1.0f);

  glColor4f(sr, sg, sb, sa);
  int xpos;
  for (xpos = -2 * stripe_width +
        (float)(now() % 3000000) / 3000000 * 2 * stripe_width;
       xpos < window_w + window_h;
       xpos += (2 * stripe_width)) {
    glVertex2f(to_window(xpos, window_w), 1);
    glVertex2f(to_window(xpos + stripe_width, window_w), 1);
    glVertex2f(to_window(xpos - window_h + stripe_width, window_w), -1);
    glVertex2f(to_window(xpos - window_h, window_w), -1);
  }
  glEnd();
}



static void glGray2f(float x, float a) {
  glColor4f(x, x, x, a);
}

static void aligned_rect(float x1, float x2, float y1, float y2) {
  glVertex3f(x1, -y1, 1);
  glVertex3f(x2, -y1, 1);
  glVertex3f(x2, -y2, 1);
  glVertex3f(x1, -y2, 1);
}

static void render_block(shared_ptr<const LevelState> game,
    shared_ptr<const Block> block, int window_w, int window_h) {
  const auto* block_ptr = block.get();

  if (block->special == BlockSpecial::CreatesMonsters) {
    float non_red_channels = static_cast<float>(block->frames_until_action)
        / game->get_frames_between_monsters();
    glColor4f(1.0, non_red_channels, non_red_channels, block->integrity);
  } else {
    float brightness_modifier = fnv1a64(&block_ptr, sizeof(block_ptr)) & 0x0F;
    float block_brightness = 0.8 + 0.2 * (brightness_modifier / 15);
    glGray2f(block_brightness, block->integrity);
  }

  const auto& params = game->get_params();
  float x1 = to_window(block->x, params.w);
  float x2 = to_window(block->x + params.grid_pitch, params.w);
  float y1 = to_window(block->y, params.h);
  float y2 = to_window(block->y + params.grid_pitch, params.h);
  aligned_rect(x1, x2, y1, y2);

  if (block->special != BlockSpecial::None) {
    render_image(special_to_image.at(block->special), x1, x2, -y1, -y2,
        block->integrity, false);
  }
}

static void render_monster(shared_ptr<const LevelState> game,
    shared_ptr<const Monster> monster, int window_w, int window_h) {
  const auto& params = game->get_params();
  float x1 = to_window(monster->x, params.w);
  float x2 = to_window(monster->x + params.grid_pitch, params.w);
  float y1 = to_window(monster->y, params.h);
  float y2 = to_window(monster->y + params.grid_pitch, params.h);

  if (monster->facing_direction == Impulse::Right || monster->facing_direction == Impulse::Left) {
    int64_t tread_pitch = params.grid_pitch / 4;
    float tread_boundary_1 = to_window(((monster->x + tread_pitch) / tread_pitch) * tread_pitch, params.w);
    float tread_boundary_2 = to_window(((monster->x + 2 * tread_pitch) / tread_pitch) * tread_pitch, params.w);
    float tread_boundary_3 = to_window(((monster->x + 3 * tread_pitch) / tread_pitch) * tread_pitch, params.w);
    float tread_boundary_4 = to_window(((monster->x + 4 * tread_pitch) / tread_pitch) * tread_pitch, params.w);
    float tread_y2 = to_window(monster->y + tread_pitch, params.h);
    float tread_y3 = to_window(monster->y + params.grid_pitch - tread_pitch, params.h);

    // draw top treads
    bool first_light = ((monster->x / tread_pitch) & 1);
    glGray2f(0.6 + first_light * 0.2, 1);
    aligned_rect(x1,               tread_boundary_1, y1, tread_y2);
    glGray2f(0.6 + !first_light * 0.2, 1);
    aligned_rect(tread_boundary_1, tread_boundary_2, y1, tread_y2);
    glGray2f(0.6 + first_light * 0.2, 1);
    aligned_rect(tread_boundary_2, tread_boundary_3, y1, tread_y2);
    glGray2f(0.6 + !first_light * 0.2, 1);
    aligned_rect(tread_boundary_3, tread_boundary_4, y1, tread_y2);
    glGray2f(0.6 + first_light * 0.2, 1);
    aligned_rect(tread_boundary_4, x2,               y1, tread_y2);

    glGray2f(0.6 + first_light * 0.2, 1);
    aligned_rect(x1,               tread_boundary_1, tread_y3, y2);
    glGray2f(0.6 + !first_light * 0.2, 1);
    aligned_rect(tread_boundary_1, tread_boundary_2, tread_y3, y2);
    glGray2f(0.6 + first_light * 0.2, 1);
    aligned_rect(tread_boundary_2, tread_boundary_3, tread_y3, y2);
    glGray2f(0.6 + !first_light * 0.2, 1);
    aligned_rect(tread_boundary_3, tread_boundary_4, tread_y3, y2);
    glGray2f(0.6 + first_light * 0.2, 1);
    aligned_rect(tread_boundary_4, x2,               tread_y3, y2);

  } else {
    int64_t tread_pitch = params.grid_pitch / 4;
    float tread_boundary_1 = to_window(((monster->y + tread_pitch) / tread_pitch) * tread_pitch, params.h);
    float tread_boundary_2 = to_window(((monster->y + 2 * tread_pitch) / tread_pitch) * tread_pitch, params.h);
    float tread_boundary_3 = to_window(((monster->y + 3 * tread_pitch) / tread_pitch) * tread_pitch, params.h);
    float tread_boundary_4 = to_window(((monster->y + 4 * tread_pitch) / tread_pitch) * tread_pitch, params.h);
    float tread_x2 = to_window(monster->x + tread_pitch, params.w);
    float tread_x3 = to_window(monster->x + params.grid_pitch - tread_pitch, params.w);

    // draw top treads
    bool first_light = ((monster->y / tread_pitch) & 1);
    glGray2f(0.6 + first_light * 0.2, 1);
    aligned_rect(x1, tread_x2, y1,               tread_boundary_1);
    glGray2f(0.6 + !first_light * 0.2, 1);
    aligned_rect(x1, tread_x2, tread_boundary_1, tread_boundary_2);
    glGray2f(0.6 + first_light * 0.2, 1);
    aligned_rect(x1, tread_x2, tread_boundary_2, tread_boundary_3);
    glGray2f(0.6 + !first_light * 0.2, 1);
    aligned_rect(x1, tread_x2, tread_boundary_3, tread_boundary_4);
    glGray2f(0.6 + first_light * 0.2, 1);
    aligned_rect(x1, tread_x2, tread_boundary_4, y2);

    glGray2f(0.6 + first_light * 0.2, 1);
    aligned_rect(tread_x3, x2, y1,               tread_boundary_1);
    glGray2f(0.6 + !first_light * 0.2, 1);
    aligned_rect(tread_x3, x2, tread_boundary_1, tread_boundary_2);
    glGray2f(0.6 + first_light * 0.2, 1);
    aligned_rect(tread_x3, x2, tread_boundary_2, tread_boundary_3);
    glGray2f(0.6 + !first_light * 0.2, 1);
    aligned_rect(tread_x3, x2, tread_boundary_3, tread_boundary_4);
    glGray2f(0.6 + first_light * 0.2, 1);
    aligned_rect(tread_x3, x2, tread_boundary_4, y2);
  }

  // draw body
  if (monster->has_flags(Monster::Flag::IsPlayer)) {
    glColor4f(0.2, 0.9, 0.0, monster->integrity);
  } else if (monster->has_flags(Monster::Flag::IsPower)) {
    glColor4f(0.9, 0, 0.9, monster->integrity);
  } else {
    glColor4f(0.9, 0, 0, monster->integrity);
  }
  float body_x1 = to_window(monster->x + params.grid_pitch / 8, params.w);
  float body_x2 = to_window(monster->x + (params.grid_pitch * 7) / 8, params.w);
  float body_y1 = to_window(monster->y + params.grid_pitch / 8, params.h);
  float body_y2 = to_window(monster->y + (params.grid_pitch * 7) / 8, params.h);
  aligned_rect(body_x1, body_x2, body_y1, body_y2);

  // draw eyes
  glColor4f(0, 0, 0, monster->integrity);
  if (monster->facing_direction == Impulse::Left) {
    float x1 = to_window(monster->x + 2 * params.grid_pitch / 8, params.w);
    float x2 = to_window(monster->x + 3 * params.grid_pitch / 8, params.w);
    aligned_rect(x1, x2, to_window(monster->y + 2 * params.grid_pitch / 8, params.h),
        to_window(monster->y + 3 * params.grid_pitch / 8, params.h));
    aligned_rect(x1, x2, to_window(monster->y + 5 * params.grid_pitch / 8, params.h),
        to_window(monster->y + 6 * params.grid_pitch / 8, params.h));
  } else if (monster->facing_direction == Impulse::Right) {
    float x1 = to_window(monster->x + 5 * params.grid_pitch / 8, params.w);
    float x2 = to_window(monster->x + 6 * params.grid_pitch / 8, params.w);
    aligned_rect(x1, x2, to_window(monster->y + 2 * params.grid_pitch / 8, params.h),
        to_window(monster->y + 3 * params.grid_pitch / 8, params.h));
    aligned_rect(x1, x2, to_window(monster->y + 5 * params.grid_pitch / 8, params.h),
        to_window(monster->y + 6 * params.grid_pitch / 8, params.h));
  } else if (monster->facing_direction == Impulse::Up) {
    float y1 = to_window(monster->y + 2 * params.grid_pitch / 8, params.h);
    float y2 = to_window(monster->y + 3 * params.grid_pitch / 8, params.h);
    aligned_rect(to_window(monster->x + 2 * params.grid_pitch / 8, params.w),
        to_window(monster->x + 3 * params.grid_pitch / 8, params.w), y1, y2);
    aligned_rect(to_window(monster->x + 5 * params.grid_pitch / 8, params.w),
        to_window(monster->x + 6 * params.grid_pitch / 8, params.w), y1, y2);
  } else if (monster->facing_direction == Impulse::Down) {
    float y1 = to_window(monster->y + 5 * params.grid_pitch / 8, params.h);
    float y2 = to_window(monster->y + 6 * params.grid_pitch / 8, params.h);
    aligned_rect(to_window(monster->x + 2 * params.grid_pitch / 8, params.w),
        to_window(monster->x + 3 * params.grid_pitch / 8, params.w), y1, y2);
    aligned_rect(to_window(monster->x + 5 * params.grid_pitch / 8, params.w),
        to_window(monster->x + 6 * params.grid_pitch / 8, params.w), y1, y2);
  }

  // if the monster has powerups, draw the bars half a cell above the monster.
  // but if it's in the top row, draw below instead
  bool below = (monster->y < params.grid_pitch);
  float bar_y = (below ? (y2 + (y2 - y1) / 2) : (y1 - (y2 - y1) / 2)) -
      (monster->special_to_frames_remaining.size() * (y2 - y1) / 16);
  float bar_center = (x1 + x2) / 2;
  for (const auto& it : monster->special_to_frames_remaining) {
    float bottom_y = bar_y + (y2 - y1) / 8;
    float bar_halfwidth = (static_cast<float>(it.second) / 300) * (x2 - x1);
    switch (it.first) {
      case BlockSpecial::Invincibility:
        glColor4f(0, 1, 0, 1); // green
        break;
      case BlockSpecial::Speed:
        glColor4f(1, 1, 0, 1); // yellow
        break;
      case BlockSpecial::TimeStop:
        glColor4f(1, 0, 1, 1); // magenta
        break;
      case BlockSpecial::ThrowBombs:
        glColor4f(1, 0.5, 0, 1); // orange
        break;
      case BlockSpecial::KillsMonsters:
        glColor4f(1, 0, 0, 1); // red
        break;
      default:
        throw logic_error("invalid special type on monster");
    }
    aligned_rect(bar_center - bar_halfwidth, bar_center + bar_halfwidth,
        bar_y, bottom_y);
    bar_y += (y2 - y1) / 8;
  }
}

static void render_explosions(shared_ptr<const LevelState> game, int window_w,
    int window_h) {
  auto explosions = game->get_explosions();
  if (explosions.empty()) {
    return;
  }

  glBegin(GL_QUADS);
  const auto& params = game->get_params();
  for (const auto& explosion : explosions) {
    float x1 = to_window(explosion->x, params.w);
    float x2 = to_window(explosion->x + params.grid_pitch, params.w);
    float y1 = to_window(explosion->y, params.h);
    float y2 = to_window(explosion->y + params.grid_pitch, params.h);

    glColor4f(1.0, 0.5, 0.0,
        (explosion->integrity > 1.0) ? 1.0 : explosion->integrity);
    aligned_rect(x1, x2, y1, y2);
  }
  glEnd();
}



static void render_level_state(shared_ptr<const LevelState> game,
    int64_t level_index, int64_t player_lives, int64_t player_score,
    int64_t player_skip_levels, int window_w, int window_h) {
  // draw black background
  glClearColor(0, 0, 0, 0);

  // draw explosions
  render_explosions(game, window_w, window_h);

  glBegin(GL_QUADS);

  // draw blocks gray
  for (const auto& block : game->get_blocks()) {
    render_block(game, block, window_w, window_h);
  }

  // draw monsters red and the player yellow
  for (const auto& monster : game->get_monsters()) {
    if (monster->death_frame >= 0) {
      continue;
    }
    render_monster(game, monster, window_w, window_h);
  }

  glEnd();

  // draw the player's score and lives
  float aspect_ratio = (float)window_w / window_h;
  vector<string> lines;
  lines.emplace_back(string_printf("Score: %" PRId64, player_score));
  if (level_index != 0) {
    lines.emplace_back(string_printf("Lives: %" PRId64, player_lives));
  }
  if (player_skip_levels) {
    lines.emplace_back(string_printf("Next level: %" PRId64, level_index + 1 + player_skip_levels));
  }
  lines.emplace_back(string_printf("%.1f%%", game->current_score_proportion() * 100.0));

  glBlendFunc(GL_ONE_MINUS_DST_COLOR, GL_ZERO);
  float y = -0.9;
  for (const auto& s : lines) {
    draw_text(-0.99, y, 0.0, 0.8, 0.0, 1.0, aspect_ratio, 0.01, false, "%s", s.c_str());
    y += 0.1;
  }
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}



struct Annotation {
  float x;
  float y;
  float r;
  float g;
  float b;
  float a;
  float decay;
  float size;
  uint64_t creation_time;
  string text;

  Annotation(float x, float y, float r, float g, float b, float a, float decay,
      float size, string&& text) : x(x), y(y), r(r), g(g), b(b), a(a),
      decay(decay), size(size), creation_time(now()), text(move(text)) { }
  ~Annotation() = default;
};

static void render_and_delete_annotations(int window_w, int window_h,
    unordered_set<unique_ptr<Annotation>>& annotations) {
  for (auto annotation_it = annotations.begin(); annotation_it != annotations.end();) {
    const auto& annotation = *annotation_it;
    double usecs_passed = now() - annotation->creation_time;
    float effective_a = annotation->a - (usecs_passed / 1000000) * annotation->decay;
    if (effective_a <= 0) {
      annotation_it = annotations.erase(annotation_it);
    } else {
      if (effective_a > 1) {
        effective_a = 1;
      }
      draw_text(annotation->x, annotation->y, annotation->r, annotation->g,
          annotation->b, effective_a, (float)window_w / window_h,
          annotation->size, true, annotation->text.c_str());
      annotation_it++;
    }
  }
}



static void render_game_screen(
    const vector<LevelState::GenerationParameters>& generation_params,
    shared_ptr<const LevelState> game, int window_w, int window_h,
    unordered_set<unique_ptr<Annotation>>& annotations, int64_t player_lives,
    int64_t player_score, int64_t player_skip_levels, int64_t level_index,
    int64_t frames_until_next_level, Phase phase) {
  render_level_state(game, level_index, player_lives, player_score,
      player_skip_levels, window_w, window_h);
  render_and_delete_annotations(window_w, window_h, annotations);

  float aspect_ratio = (float)window_w / window_h;
  if (frames_until_next_level) {
    render_stripe_animation(window_w, window_h, 100, 0.0f, 0.0f, 0.0f, 0.5f,
        0.0f, 0.0f, 0.0f, 0.1f);
    if (phase == Phase::Playing) {
      int64_t next_level = level_index + 1 + player_skip_levels;
      if (next_level >= generation_params.size()) {
        level_index = 0; // TODO: this should probably be size/2 or something
      }
      draw_text(0, 0.7, 1, 1, 1, 1, aspect_ratio, 0.025, true,
          "LEVEL %" PRId64 " COMPLETE", level_index);
      draw_text(0, 0.4, 1, 1, 1, 1, aspect_ratio, 0.015, true,
          "LEVEL %" PRId64 " NEXT", next_level);
      draw_text(0, 0.25, 1, 1, 1, 1, aspect_ratio, 0.01, true, "%s",
          generation_params[next_level].name.c_str());
    }

    glBegin(GL_QUADS);
    glGray2f(1, 1);
    float progress = static_cast<float>(frames_until_next_level) / (3 * game->get_updates_per_second());
    aligned_rect(-progress, progress, -0.05, 0.05);
    glEnd();

  } else if (game->get_player()->death_frame >= 0) {
    if (player_lives == 0) {
      render_stripe_animation(window_w, window_h, 100, 0.1f, 0.0f, 0.0f, 0.8f,
          1.0f, 0.0f, 0.0f, 0.1f);
      draw_text(0, 0.7, 1, 0, 0, 1, aspect_ratio, 0.03, true,
          "GAME OVER");
      draw_text(0, 0.2, 1, 1, 1, 1, aspect_ratio, 0.015, true,
          "YOUR SCORE IS %" PRId64, player_score);
      draw_text(0, 0.0, 1, 1, 1, 1, aspect_ratio, 0.01, true,
          "Press Enter to start over...");
    } else {
      render_stripe_animation(window_w, window_h, 100, 0.1f, 0.0f, 0.0f, 0.5f,
          1.0f, 0.0f, 0.0f, 0.1f);
      if (level_index != 0) {
        draw_text(0, 0.6, 1, 0, 0, 1, aspect_ratio, 0.01, true,
            "You have %" PRId64 " %s remaining", player_lives,
            (player_lives == 1) ? "life" : "lives");
      } else {
        draw_text(0, 0.6, 1, 0, 0, 1, aspect_ratio, 0.01, true,
            "You have unlimited lives on level 0");
      }
      draw_text(0, 0.2, 1, 0, 0, 1, aspect_ratio, 0.01, true,
          "Press Enter to try again...");
    }
  }
}



static void render_paused_overlay(int window_w, int window_h, int level_index,
    const char* level_name, uint64_t frames_executed, bool should_play_sounds) {
  render_stripe_animation(window_w, window_h, 100, 0.0f, 0.0f, 0.0f, 0.5f, 0.0f,
      0.0f, 0.0f, 0.1f);

  float aspect_ratio = (float)window_w / window_h;
  draw_text(0, 0.7, 1, 1, 1, 1, aspect_ratio, 0.03, true,
      "TREADS");

  draw_text(0, 0.3, 1, 1, 1, 1, aspect_ratio, 0.015, true, "LEVEL %" PRId64,
      level_index);
  draw_text(0, 0.15, 1, 1, 1, 1, aspect_ratio, 0.01, true, "%s", level_name);

  draw_text(0, 0.0, 1, 1, 1, 1, aspect_ratio, 0.007, true, "PRESS ENTER");

  draw_text(0, -0.5, 1, 1, 1, 1, aspect_ratio, 0.01, true,
      "arrow keys: move");
  draw_text(0, -0.6, 1, 1, 1, 1, aspect_ratio, 0.01, true,
      "space: push / destroy");
  draw_text(0, -0.7, 1, 1, 1, 1, aspect_ratio, 0.01, true,
      "shift+s: %smute sound", should_play_sounds ? "" : "un");
  draw_text(0, -0.8, 1, 1, 1, 1, aspect_ratio, 0.01, true, "esc: exit");
}



vector<LevelState::GenerationParameters> generation_params;
shared_ptr<LevelState> game;
int64_t frames_until_next_level = 0;
int64_t player_lives = 3;
int64_t player_score = 0;
int64_t player_skip_levels = 0;
int64_t level_index = 0;
int64_t instructions_page = 1;
Phase phase = Phase::Paused;
bool should_play_sounds = true;
uint64_t current_impulse = None;



static pair<int64_t, int64_t> parse_high_low(
    const shared_ptr<JSONObject>& json) {
  if (json->is_list()) {
    return make_pair(json->at(0)->as_int(), json->at(1)->as_int());
  } else {
    int64_t count = json->as_int();
    return make_pair(count, count);
  }
}

static int64_t json_get_default_int(const shared_ptr<JSONObject>& json,
    const std::string& key, int64_t default_value) {
  try {
    return json->at(key)->as_int();
  } catch (const JSONObject::key_error& e) {
    return default_value;
  }
}

static pair<int64_t, int64_t> json_get_default_int_pair(
    const shared_ptr<JSONObject>& json, const std::string& key,
    const pair<int64_t, int64_t>& default_value) {
  try {
    return parse_high_low(json->at(key));
  } catch (const JSONObject::key_error& e) {
    return default_value;
  }
}

static int64_t json_get_default_bool(const shared_ptr<JSONObject>& json,
    const std::string& key, bool default_value) {
  try {
    return json->at(key)->as_bool();
  } catch (const JSONObject::key_error& e) {
    return default_value;
  }
}

static double json_get_default_float(const shared_ptr<JSONObject>& json,
    const std::string& key, double default_value) {
  try {
    return json->at(key)->as_float();
  } catch (const JSONObject::key_error& e) {
    return default_value;
  }
}

static Monster::MovementPolicy json_get_default_MovementPolicy(
    const shared_ptr<JSONObject>& json, const std::string& key,
    Monster::MovementPolicy default_value) {
  try {
    return Monster::movement_policy_for_name(json->at(key)->as_string().c_str());
  } catch (const JSONObject::key_error& e) {
    return default_value;
  }
}

static unordered_map<BlockSpecial, pair<int64_t, int64_t>> parse_special_counts_dict(
    const shared_ptr<JSONObject>& json) {
  unordered_map<BlockSpecial, pair<int64_t, int64_t>> result;
  for (const auto& it : json->as_dict()) {
    result.emplace(special_for_name(it.first.c_str()), parse_high_low(it.second));
  }
  return result;
}

static vector<LevelState::GenerationParameters> load_generation_params(
    const string& filename) {
  auto json = JSONObject::load(filename);

  LevelState::GenerationParameters defaults;
  {
    auto defaults_json = json->at("defaults");
    defaults.grid_pitch = defaults_json->at("grid_pitch")->as_int();
    defaults.w = defaults_json->at("width")->as_int();
    defaults.h = defaults_json->at("height")->as_int();
    defaults.player_x = defaults_json->at("player_x")->as_int();
    defaults.player_y = defaults_json->at("player_y")->as_int();
    defaults.player_squishable = defaults_json->at("player_squishable")->as_bool();
    defaults.basic_monster_count = parse_high_low(defaults_json->at("basic_monster_count"));
    defaults.power_monster_count = parse_high_low(defaults_json->at("power_monster_count"));
    defaults.basic_monster_score = defaults_json->at("basic_monster_score")->as_int();
    defaults.power_monster_score = defaults_json->at("power_monster_score")->as_int();
    defaults.basic_monster_movement_policy = Monster::movement_policy_for_name(
        defaults_json->at("basic_monster_movement")->as_string().c_str());
    defaults.power_monster_movement_policy = Monster::movement_policy_for_name(
        defaults_json->at("power_monster_movement")->as_string().c_str());
    defaults.power_monsters_can_push = defaults_json->at("power_monsters_can_push")->as_bool();
    defaults.power_monsters_become_creators = defaults_json->at("power_monsters_become_creators")->as_bool();
    defaults.player_move_speed = defaults_json->at("player_move_speed")->as_int();
    defaults.basic_monster_move_speed = defaults_json->at("basic_monster_move_speed")->as_int();
    defaults.power_monster_move_speed = defaults_json->at("power_monster_move_speed")->as_int();
    defaults.push_speed = defaults_json->at("push_speed")->as_int();
    defaults.bomb_speed = defaults_json->at("bomb_speed")->as_int();
    defaults.bounce_speed_absorption = defaults_json->at("bounce_speed_absorption")->as_int();
    defaults.block_destroy_rate = defaults_json->at("block_destroy_rate")->as_float();
    defaults.special_type_to_count = parse_special_counts_dict(
        defaults_json->at("special_counts"));
    defaults.fixed_block_map = false; // TODO: should block maps be defaultable?
  }

  vector<LevelState::GenerationParameters> all_params;
  for (const auto& level_json : json->at("levels")->as_list()) {
    all_params.emplace_back();
    auto& params = all_params.back();

    try {
      params.name = level_json->at("name")->as_string();
    } catch (const out_of_range&) { }
    params.grid_pitch = json_get_default_int(level_json, "grid_pitch", defaults.grid_pitch);
    params.w = json_get_default_int(level_json, "width", defaults.w);
    params.h = json_get_default_int(level_json, "height", defaults.h);
    params.player_x = json_get_default_int(level_json, "player_x", defaults.player_x);
    params.player_y = json_get_default_int(level_json, "player_y", defaults.player_y);
    params.basic_monster_count = json_get_default_int_pair(level_json, "basic_monster_count", defaults.basic_monster_count);
    params.power_monster_count = json_get_default_int_pair(level_json, "power_monster_count", defaults.power_monster_count);
    params.basic_monster_score = json_get_default_int(level_json, "basic_monster_score", defaults.basic_monster_score);
    params.power_monster_score = json_get_default_int(level_json, "power_monster_score", defaults.power_monster_score);
    params.basic_monster_movement_policy = json_get_default_MovementPolicy(level_json, "basic_monster_movement", defaults.basic_monster_movement_policy);
    params.power_monster_movement_policy = json_get_default_MovementPolicy(level_json, "power_monster_movement", defaults.power_monster_movement_policy);
    params.player_move_speed = json_get_default_int(level_json, "player_move_speed", defaults.player_move_speed);
    params.basic_monster_move_speed = json_get_default_int(level_json, "basic_monster_move_speed", defaults.basic_monster_move_speed);
    params.power_monster_move_speed = json_get_default_int(level_json, "power_monster_move_speed", defaults.power_monster_move_speed);
    params.push_speed = json_get_default_int(level_json, "push_speed", defaults.push_speed);
    params.bomb_speed = json_get_default_int(level_json, "bomb_speed", defaults.bomb_speed);
    params.bounce_speed_absorption = json_get_default_int(level_json, "bounce_speed_absorption", defaults.bounce_speed_absorption);
    params.player_squishable = json_get_default_bool(level_json, "player_squishable", defaults.player_squishable);
    params.power_monsters_can_push = json_get_default_bool(level_json, "power_monsters_can_push", defaults.player_squishable);
    params.power_monsters_become_creators = json_get_default_bool(level_json, "power_monsters_become_creators", defaults.player_squishable);
    params.block_destroy_rate = json_get_default_float(level_json, "block_destroy_rate", defaults.block_destroy_rate);

    // TODO: support block_map
    params.fixed_block_map = false;

    try {
      params.special_type_to_count = parse_special_counts_dict(
          level_json->at("special_counts"));
    } catch (const JSONObject::key_error& e) {
      params.special_type_to_count = defaults.special_type_to_count;
    }
  }

  // postprocessing: multiple all w, h, player_x, player_y by grid_pitch (in the
  // json file, they're specified in cells rather than map units)
  for (auto& params : all_params) {
    params.w *= params.grid_pitch;
    params.h *= params.grid_pitch;
    params.player_x *= params.grid_pitch;
    params.player_y *= params.grid_pitch;
  }

  return all_params;
}

static void generate_random_elements(LevelState::GenerationParameters& params) {
  if (!params.fixed_block_map) {
    params.block_map = generate_maze(params.w / params.grid_pitch,
        params.h / params.grid_pitch);
  }
}



static void glfw_key_cb(GLFWwindow* window, int key, int scancode,
    int action, int mods) {

  if (action == GLFW_PRESS) {
    if ((key == GLFW_KEY_S) && (mods & GLFW_MOD_SHIFT)) {
      should_play_sounds = !should_play_sounds;

    } else if (key == GLFW_KEY_ESCAPE) {
      if (phase == Phase::Paused) {
        glfwSetWindowShouldClose(window, 1);
      } else {
        phase = Phase::Paused;
      }

    } else if (key == GLFW_KEY_ENTER) {
      if (phase == Phase::Playing) {
        if ((game->get_player()->death_frame >= 0) && !frames_until_next_level) {
          if (level_index == 0) {
            // you have infinite lives on level 0 but can't keep your score
            player_lives = 3;
            player_score = 0;
          } else if (player_lives == 0) {
            level_index = 0;
            player_lives = 3;
            player_score = 0;
            frames_until_next_level = 0; // don't allow player to enter next level
          } else {
            player_lives--;
          }
          player_skip_levels = 0;
          generate_random_elements(generation_params[level_index]);
          game.reset(new LevelState(generation_params[level_index]));
        }
        phase = Phase::Paused;
      } else if (phase == Phase::Paused) {
        phase = Phase::Playing;
      }

    } else if (key == GLFW_KEY_LEFT) {
      current_impulse |= Impulse::Left;
      phase = Phase::Playing;
    } else if (key == GLFW_KEY_RIGHT) {
      current_impulse |= Impulse::Right;
      phase = Phase::Playing;
    } else if (key == GLFW_KEY_UP) {
      current_impulse |= Impulse::Up;
      phase = Phase::Playing;
    } else if (key == GLFW_KEY_DOWN) {
      current_impulse |= Impulse::Down;
      phase = Phase::Playing;
    } else if (key == GLFW_KEY_SPACE) {
      current_impulse |= Impulse::Push;
      phase = Phase::Playing;
    }
  }

  if (action == GLFW_RELEASE) {
    // note: we don't check for paused here to avoid bad state if the player
    // pauses while holding a direction key
    if ((key == GLFW_KEY_LEFT) && ((phase == Phase::Playing) || (phase == Phase::Paused))) {
      current_impulse &= ~Impulse::Left;
    } else if ((key == GLFW_KEY_RIGHT) && ((phase == Phase::Playing) || (phase == Phase::Paused))) {
      current_impulse &= ~Impulse::Right;
    } else if ((key == GLFW_KEY_UP) && ((phase == Phase::Playing) || (phase == Phase::Paused))) {
      current_impulse &= ~Impulse::Up;
    } else if ((key == GLFW_KEY_DOWN) && ((phase == Phase::Playing) || (phase == Phase::Paused))) {
      current_impulse &= ~Impulse::Down;
    } else if ((key == GLFW_KEY_SPACE) && ((phase == Phase::Playing) || (phase == Phase::Paused))) {
      current_impulse &= ~Impulse::Push;
    }
  }
}

static void glfw_focus_cb(GLFWwindow* window, int focused) {
  // auto-pause when we lose focus, except if the player is dead (in this case
  // they will have to press enter to start playing again anyway)
  if ((focused == GL_FALSE) && (phase == Phase::Playing) &&
      (game->get_player()->death_frame < 0)) {
    phase = Phase::Paused;

    // refresh the "get ready" time
    if (frames_until_next_level) {
      frames_until_next_level = 3 * game->get_updates_per_second();
    }
  }
}

static void glfw_resize_cb(GLFWwindow* window, int width, int height) {
  glViewport(0, 0, width, height);
}

static void glfw_error_cb(int error, const char* description) {
  fprintf(stderr, "[GLFW %d] %s\n", error, description);
}



static bool add_sound(
    unordered_map<Event, unique_ptr<SampledSound>>& event_to_sound, Event event,
    const string& filename) {
  try {
    event_to_sound.emplace(piecewise_construct, forward_as_tuple(event),
        forward_as_tuple(new SampledSound(filename.c_str())));
    return true;
  } catch (const cannot_open_file&) {
    return false;
  }
}

static void add_block_special_image(BlockSpecial special,
    const string& filename) {
  special_to_image.emplace(piecewise_construct, forward_as_tuple(special),
      forward_as_tuple(filename.c_str()));
}

int main(int argc, char* argv[]) {

  for (int x = 1; x < argc; x++) {
    if (!strncmp(argv[x], "--level-index=", 14)) {
      level_index = strtoull(&argv[x][14], NULL, 0);
    } else {
      throw invalid_argument("unknown command-line option");
    }
  }

  srand(time(NULL) ^ getpid());

  string media_directory;
#ifdef MACOSX
  CFURLRef app_url = CFBundleCopyBundleURL(CFBundleGetMainBundle());
  CFStringRef path = CFURLCopyFileSystemPath(app_url, kCFURLPOSIXPathStyle);
  const char *p = CFStringGetCStringPtr(path, CFStringGetSystemEncoding());

  // if we're in an app bundle, look in Resources/ for media; else look in the
  // the executable directory
  size_t p_len = strlen(p);
  if ((p_len >= 4) && !strcmp(p + p_len - 4, ".app")) {
    media_directory = string(p) + "/Contents/Resources";
  } else {
    media_directory = string(p) + "/media";
  }

  CFRelease(app_url);
  CFRelease(path);
#else
  // assume it's in the same working directory for now
  media_directory = "media";
#endif

  add_block_special_image(BlockSpecial::Timer, media_directory + "/special_timer.bmp");
  add_block_special_image(BlockSpecial::LineUp, media_directory + "/special_line_up.bmp");
  add_block_special_image(BlockSpecial::Points, media_directory + "/special_points.bmp");
  add_block_special_image(BlockSpecial::ExtraLife, media_directory + "/special_extra_life.bmp");
  add_block_special_image(BlockSpecial::SkipLevels, media_directory + "/special_skip_levels.bmp");
  add_block_special_image(BlockSpecial::Indestructible, media_directory + "/special_indestructible.bmp");
  add_block_special_image(BlockSpecial::IndestructibleAndImmovable, media_directory + "/special_indestructible_and_immovable.bmp");
  add_block_special_image(BlockSpecial::Immovable, media_directory + "/special_immovable.bmp");
  add_block_special_image(BlockSpecial::Brittle, media_directory + "/special_brittle.bmp");
  add_block_special_image(BlockSpecial::Bomb, media_directory + "/special_bomb.bmp");
  add_block_special_image(BlockSpecial::Bouncy, media_directory + "/special_bouncy.bmp");
  add_block_special_image(BlockSpecial::BouncyBomb, media_directory + "/special_bouncy_bomb.bmp");
  add_block_special_image(BlockSpecial::CreatesMonsters, media_directory + "/special_creates_monsters.bmp");
  add_block_special_image(BlockSpecial::Invincibility, media_directory + "/special_invincibility.bmp");
  add_block_special_image(BlockSpecial::Speed, media_directory + "/special_speed.bmp");
  add_block_special_image(BlockSpecial::TimeStop, media_directory + "/special_time_stop.bmp");
  add_block_special_image(BlockSpecial::ThrowBombs, media_directory + "/special_throw_bombs.bmp");
  add_block_special_image(BlockSpecial::KillsMonsters, media_directory + "/special_kills_monsters.bmp");
  add_block_special_image(BlockSpecial::Everything, media_directory + "/special_everything.bmp");

  init_al();
  unordered_map<Event, unique_ptr<SampledSound>> event_to_sound;
  add_sound(event_to_sound, Event::BlockPushed, media_directory + "/push.wav");
  add_sound(event_to_sound, Event::MonsterSquished, media_directory + "/squish_monster.wav");
  add_sound(event_to_sound, Event::MonsterKilled, media_directory + "/squish_monster.wav");
  add_sound(event_to_sound, Event::PlayerKilled, media_directory + "/squish_player.wav");
  add_sound(event_to_sound, Event::BonusCollected, media_directory + "/crush_bonus.wav");
  add_sound(event_to_sound, Event::BlockDestroyed, media_directory + "/crush_block.wav");
  add_sound(event_to_sound, Event::BlockBounced, media_directory + "/block_bounce.wav");
  add_sound(event_to_sound, Event::Explosion, media_directory + "/explode.wav");
  add_sound(event_to_sound, Event::BlockStopped, media_directory + "/block_stop.wav");
  add_sound(event_to_sound, Event::PlayerSquished, media_directory + "/squish_player.wav");
  add_sound(event_to_sound, Event::LifeCollected, media_directory + "/extra_life.wav");
  add_sound(event_to_sound, Event::MonsterCreated, media_directory + "/monster_create.wav");

  if (!glfwInit()) {
    fprintf(stderr, "failed to initialize GLFW\n");
    return 2;
  }
  glfwSetErrorCallback(glfw_error_cb);

  // generate the level
  generation_params = load_generation_params(media_directory + "/levels.json");
  generate_random_elements(generation_params[level_index]);
  game.reset(new LevelState(generation_params[level_index]));
  uint64_t w_cells = generation_params[level_index].w / generation_params[level_index].grid_pitch;
  uint64_t h_cells = generation_params[level_index].h / generation_params[level_index].grid_pitch;

  // auto-size the window based on the primary monitor size
  GLFWmonitor* monitor = glfwGetPrimaryMonitor();
  const GLFWvidmode* vidmode = glfwGetVideoMode(monitor);
  int cell_size_w = (vidmode->width - 100) / w_cells;
  int cell_size_h = (vidmode->height - 100) / h_cells;
  int cell_size = (cell_size_w < cell_size_h) ? cell_size_w : cell_size_h;

  //glfwOpenWindowHint(GLFW_WINDOW_NO_RESIZE, GL_TRUE);
  GLFWwindow* window = glfwCreateWindow(w_cells * cell_size,
      h_cells * cell_size, "Treads", NULL, NULL);
  if (!window) {
    glfwTerminate();
    fprintf(stderr, "failed to create window\n");
    return 2;
  }

  glfwSetFramebufferSizeCallback(window, glfw_resize_cb);
  glfwSetKeyCallback(window, glfw_key_cb);
  glfwSetWindowFocusCallback(window, glfw_focus_cb);

  glfwMakeContextCurrent(window);

  // 2D drawing mode
  glDisable(GL_LIGHTING);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();

  // raster operations config
  glDisable(GL_DEPTH_TEST);
  glEnable(GL_LINE_SMOOTH);
  glLineWidth(3);
  glPointSize(12);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  uint64_t last_update_time = now();

  unordered_set<unique_ptr<Annotation>> annotations;
  while (!glfwWindowShouldClose(window)) {

    int window_w, window_h;
    glfwGetFramebufferSize(window, &window_w, &window_h);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    string validation_failure;
    try {
      game->validate();
    } catch (const exception& e) {
      fprintf(stderr, "validation failure: %s\n", e.what());
      validation_failure = e.what();
    }

    if (!validation_failure.empty()) {
      render_stripe_animation(window_w, window_h, 100, 0.0f, 0.0f, 0.0f, 0.6f,
          1.0, 0.0, 0.0, 0.3);
      draw_text(0, 0.3, 1, 0, 0, 1, (float)window_w / window_h, 0.004, true,
          validation_failure.c_str());
      draw_text(0, 0.0, 1, 1, 1, 1, (float)window_w / window_h, 0.01, true,
          "esc: exit");

    } else {
      uint64_t usec_per_update = 1000000.0 / game->get_updates_per_second();
      uint64_t now_time = now();
      uint64_t update_diff = now_time - last_update_time;
      if (update_diff >= usec_per_update) {
        if (phase == Phase::Playing) {
          if (frames_until_next_level == 0) {
            auto events = game->exec_frame(current_impulse);
            if (should_play_sounds) {
              while (events.events_mask) {
                uint64_t remaining_events = events.events_mask & (events.events_mask - 1);
                Event this_event = static_cast<Event>(events.events_mask ^ remaining_events);
                try {
                  event_to_sound.at(this_event)->play();
                } catch (const out_of_range& e) { }
                events.events_mask = remaining_events;
              }
            }

            for (const auto& score : events.scores) {
              if (score.monster->has_flags(Monster::Flag::IsPlayer)) {
                player_score += score.score;
                player_lives += score.lives;
                player_skip_levels += score.skip_levels;
              }

              const auto& params = game->get_params();
              if (!score.killed.get()) {
                // this score came from a bonus block
                float annotation_x = to_window(score.block_x + params.grid_pitch / 2, params.w);
                float annotation_y = -to_window(score.block_y + params.grid_pitch / 2, params.h);

                if (score.bonus != BlockSpecial::None) {
                  annotations.emplace(new Annotation(annotation_x, annotation_y,
                      0, 1, 0, 2, 1, 0.007, display_name_for_special(score.bonus)));
                } else if (score.lives) {
                  annotations.emplace(new Annotation(annotation_x, annotation_y,
                      0, 1, 0, 2, 1, 0.007, string_printf("%dUP", score.lives)));
                } else if (score.score) {
                  annotations.emplace(new Annotation(annotation_x, annotation_y,
                      0, 1, 0, 2, 1, 0.007, string_printf("%d", score.score)));
                }
              } else if (score.killed.get() && (
                  (score.killed->has_flags(Monster::Flag::IsPlayer)) ||
                  (score.killed == score.monster))) {
                float annotation_x = to_window(score.killed->x + params.grid_pitch / 2, params.w);
                float annotation_y = -to_window(score.killed->y + params.grid_pitch / 2, params.h);
                annotations.emplace(new Annotation(annotation_x, annotation_y,
                    1, 0.5, 0, 2, 1, 0.007, "oh no!"));
              } else {
                shared_ptr<Monster> position_monster = score.killed.get() ?
                    score.killed : score.monster;
                float annotation_x = to_window(position_monster->x + params.grid_pitch / 2, params.w);
                float annotation_y = -to_window(position_monster->y + params.grid_pitch / 2, params.h);
                if (score.lives) {
                  annotations.emplace(new Annotation(annotation_x, annotation_y,
                      0, 1, 0, 2, 1, 0.007, string_printf("%dUP", score.lives)));
                } else if (score.score) {
                  annotations.emplace(new Annotation(annotation_x, annotation_y,
                      0, 1, 0, 2, 1, 0.007, string_printf("%d", score.score)));
                }
              }
            }

            // check if the player has completed the level
            if ((game->count_monsters_with_flags(0, Monster::Flag::IsPlayer) == 0) &&
                (game->count_blocks_with_special(BlockSpecial::CreatesMonsters) == 0)) {
              if ((game->get_player()->death_frame >= 0) && (player_lives >= 1)) {
                // player is dead, but has extra lives - they can go to the next
                // level and lose a life
                if (level_index != 0) {
                  player_lives--;
                }
                frames_until_next_level = 3 * game->get_updates_per_second();
              } else if (game->get_player()->death_frame < 0) {
                // player is alive
                frames_until_next_level = 3 * game->get_updates_per_second();
              }
            }

          } else if (frames_until_next_level == 1) {
            level_index += (1 + player_skip_levels);
            player_skip_levels = 0;
            if (level_index >= generation_params.size()) {
              level_index = 0; // TODO: this should probably be size/2 or something
            }
            generate_random_elements(generation_params[level_index]);
            game.reset(new LevelState(generation_params[level_index]));
            phase = Phase::Playing;
            frames_until_next_level = 0;
          } else if (frames_until_next_level > 0) {
            frames_until_next_level--;
          }
        }
        last_update_time = now_time;
      }

      render_game_screen(generation_params, game, window_w, window_h,
          annotations, player_lives, player_score, player_skip_levels,
          level_index, frames_until_next_level, phase);

      if (phase == Phase::Paused) {
        render_paused_overlay(window_w, window_h, level_index,
            generation_params[level_index].name.c_str(),
            game->get_frames_executed(), should_play_sounds);
      }
    }

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  glfwDestroyWindow(window);
  glfwTerminate();

  return 0;
}
