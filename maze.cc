#include "maze.hh"

#include <inttypes.h>
#include <stdint.h>

#include <algorithm>
#include <deque>
#include <random>
#include <vector>

#include "level.hh" // for Impulse

using namespace std;


struct Map2D {
  uint64_t w;
  uint64_t h;
  vector<bool> data;

  Map2D() = delete;
  Map2D(uint64_t w, uint64_t h, bool v) : w(w), h(h), data(w * h, v) { }

  bool get(int64_t x, int64_t y) const {
    return this->data[y * this->w + x];
  }

  void put(int64_t x, int64_t y, bool v) {
    this->data[y * this->w + x] = v;
  }

  void print(FILE* stream) const {
    fprintf(stream, "[map:%" PRIu64 "x%" PRIu64 "]\n", this->w, this->h);
    for (uint64_t y = 0; y < this->h; y++) {
      for (uint64_t x = 0; x < this->w; x++) {
        fputc(this->get(x, y) ? '#' : '-', stream);
      }
      fputc('\n', stream);
    }

    for (bool v : this->data) {
      fputc(v ? '#' : '-', stream);
    }
    fputc('\n', stream);
  }
};

struct DFSStep {
  int64_t x;
  int64_t y;
  uint8_t directions_remaining;

  DFSStep(int64_t x, int64_t y, uint8_t directions_remaining) : x(x), y(y),
      directions_remaining(directions_remaining) { }
};

vector<bool> generate_maze(uint64_t w, uint64_t h) {
  if (!(w & 1) || !(h & 1)) {
    throw invalid_argument("dimensions must be odd integers");
  }

  Map2D map(w, h, true);
  Map2D nodes_visited(w, h, false);

  // now choose a random node and start DFSing in a random direction from it
  deque<DFSStep> steps;
  {
    int64_t start_x = (rand() % ((w + 1) / 2)) * 2;
    int64_t start_y = (rand() % ((h + 1) / 2)) * 2;
    uint8_t start_directions =
        ((start_x == 0)     ? 0 : Impulse::Left) |
        ((start_x == w - 1) ? 0 : Impulse::Right) |
        ((start_y == 0)     ? 0 : Impulse::Up) |
        ((start_y == h - 1) ? 0 : Impulse::Down);
    steps.emplace_back(start_x, start_y, start_directions);
    nodes_visited.put(start_x, start_y, true);
    map.put(start_x, start_y, false);
  }

  // prepare for randomness
  random_device rd;
  mt19937 g(rd());

  // now do that DFS
  Impulse direction_order[] = {Impulse::Left, Impulse::Right, Impulse::Up,
      Impulse::Down};
  while (!steps.empty()) {
    auto& current_step = steps.back();

    // pick a direction to go that we haven't already gone from this step
    Impulse direction = Impulse::None;
    shuffle(direction_order, direction_order + 4, g);
    for (auto check_direction : direction_order) {
      if (current_step.directions_remaining & check_direction) {
        direction = check_direction;
        break;
      }
    }

    // if there aren't any directions to go, pop the step
    if (direction == Impulse::None) {
      steps.pop_back();
      continue;
    }

    // mark this direction as done
    current_step.directions_remaining &= ~direction;

    // compute the destination node's coordinates
    int64_t x_offset, y_offset;
    if (direction == Impulse::Left) {
      x_offset = -1;
      y_offset = 0;
    } else if (direction == Impulse::Right) {
      x_offset = 1;
      y_offset = 0;
    } else if (direction == Impulse::Up) {
      x_offset = 0;
      y_offset = -1;
    } else if (direction == Impulse::Down) {
      x_offset = 0;
      y_offset = 1;
    } else {
      throw logic_error("unknown direction");
    }

    // if the destination node has already been visited, do nothing
    int64_t dest_x = current_step.x + 2 * x_offset;
    int64_t dest_y = current_step.y + 2 * y_offset;
    if (nodes_visited.get(dest_x, dest_y)) {
      continue;
    }

    // mark the node as visited and make a path to it
    int64_t path_x = current_step.x + x_offset;
    int64_t path_y = current_step.y + y_offset;
    nodes_visited.put(dest_x, dest_y, true);
    map.put(path_x, path_y, false);
    map.put(dest_x, dest_y, false);

    // add the new node to the stack
    uint8_t available_directions =
        ((dest_x == 0)     ? 0 : Impulse::Left) |
        ((dest_x == w - 1) ? 0 : Impulse::Right) |
        ((dest_y == 0)     ? 0 : Impulse::Up) |
        ((dest_y == h - 1) ? 0 : Impulse::Down);
    steps.emplace_back(dest_x, dest_y, available_directions);
  }

  return map.data;
}
