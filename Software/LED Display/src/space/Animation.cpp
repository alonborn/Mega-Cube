#include "Animation.h"

#include "AnimationPlaylist.h"

#include "Accelerometer.h"
#include "Arrows.h"
#include "Atoms.h"
#include "Aurora.h"
#include "BlackHole.h"
#include "ChannelColorTest.h"
#include "ChannelTest.h"
#include "DNATunnel.h"
#include "ElectricStorm.h"
#include "EyeOfSauron.h"
#include "Fireworks.h"
#include "Helix.h"
#include "LedTest.h"
#include "Life.h"
#include "Mario.h"
#include "Metaballs.h"
#include "Plasma.h"
#include "Pong.h"
#include "RedTest.h"
#include "Scroller.h"
#include "Sinus.h"
#include "Spectrum.h"
#include "SpottedSphere.h"
#include "Starfield.h"
#include "Supernova.h"
#include "TheMatrix.h"
#include "Twinkels.h"
#include "WhiteTest.h"
#include "Cube.h"
/*------------------------------------------------------------------------------
 * ANIMATION STATIC DEFINITIONS
 *----------------------------------------------------------------------------*/
Noise Animation::noise = Noise();
Timer Animation::animation_timer = Timer();
uint16_t Animation::animation_sequence = 0;

static uint8_t playlist_index = 0;
static float playlist_elapsed = 0.0f;
static bool playlist_started = false;
static bool playlist_was_enabled = false;
/*------------------------------------------------------------------------------
 * ANIMATION GLOBAL DEFINITIONS
 *----------------------------------------------------------------------------*/
Accelerometer accelerometer;
Arrows arrows;
Atoms atoms;
Aurora aurora;
BlackHole black_hole;
ChannelColorTest channelcolortest;
ChannelTest channeltest;
DNATunnel dna_tunnel;
ElectricStorm electric_storm;
EyeOfSauron eye_of_sauron;
Fireworks fireworks1;
Fireworks fireworks2;
Helix helix;
LedTest ledtest;
Life life;
Mario mario;
Metaballs metaballs;
Plasma plasma;
Pong pong;
Scroller scroller;
Sinus sinus;
Spectrum spectrum;
SpottedSphere spotted_sphere;
Starfield starfield;
Supernova supernova;
TheMatrix the_matrix;
Twinkels twinkels;
WhiteTest white_test;
RedTest red_test;
Cube cube;

Animation *Animations[] = {&ledtest,    &atoms,    &sinus,        &starfield,
                           &fireworks1,
                           &fireworks2, &twinkels, &helix,        &arrows,
                           &plasma,     &mario,    &life,         &pong,
                           &spectrum,   &scroller, &accelerometer, &cube,
                           &channeltest, &channelcolortest, &the_matrix,
                           &spotted_sphere, &supernova, &aurora, &black_hole,
                           &metaballs, &electric_storm, &dna_tunnel,
                           &eye_of_sauron, &white_test, &red_test};

const uint8_t ANIMATIONS = sizeof(Animations) / sizeof(Animation *);
/*----------------------------------------------------------------------------*/
// Start display asap to minimize PL9823 blue startup
void Animation::begin() {
  playlist_index = 0;
  playlist_elapsed = 0.0f;
  playlist_started = false;
  playlist_was_enabled = false;
  Display::begin();
}

static void startPlaylistItem() {
  if (ANIMATION_PLAYLIST_SIZE == 0) return;

  const AnimationPlaylistEntry &entry = ANIMATION_PLAYLIST[playlist_index];
  jump_item_t jump = Animation::get_item(entry.animation_id);
  if (jump.object) {
    jump.object->init();
    jump.object->time_reduction = false;
    jump.object->timer_running = 0;
    if (jump.custom_init) jump.custom_init();
    playlist_started = true;
  }
}

// Render an animation frame, but only if the display allows it (non blocking)
void Animation::loop() {
  // Only draw to the display if it's available
  if (Display::available()) {
    // Update the animation timer to determine frame deltatime
    animation_timer.update();
    const float dt = animation_timer.dt();

    if (settings.playlist && !playlist_was_enabled) {
      playlist_index = 0;
      playlist_elapsed = 0.0f;
      playlist_started = false;
    }
    playlist_was_enabled = settings.playlist;

    if (settings.playlist) {
      if (!playlist_started) {
        startPlaylistItem();
      } else {
        playlist_elapsed += dt;
        const float duration =
            ANIMATION_PLAYLIST[playlist_index].duration_seconds;
        if (duration <= 0.0f || playlist_elapsed >= duration) {
          for (uint8_t i = 0; i < ANIMATIONS; ++i)
            Animations[i]->state = state_t::INACTIVE;
          playlist_index = (playlist_index + 1) % ANIMATION_PLAYLIST_SIZE;
          playlist_elapsed = 0.0f;
          startPlaylistItem();
        }
      }
    }
    // Clear the display before drawing any animations
    Display::clear();
    // Draw all active animations from the animation pool
    uint8_t active_animation_count = 0;
    for (uint8_t i = 0; i < ANIMATIONS; i++) {
      Animation &animation = *Animations[i];
      if (animation.state != state_t::INACTIVE) {
        animation.draw(dt);
      }
      // Animation can become inactive after drawing so check again
      if (animation.state != state_t::INACTIVE) {
        active_animation_count++;
        if (settings.changed) animation.end();
      }
    }
    // Clear the settings changed flag. Animations are ended allready
    settings.changed = false;
    // Select the next or specific animation from the sequence list
    if (active_animation_count == 0) {
      if (settings.playlist) {
        // Keep the current entry running for its full configured duration.
        startPlaylistItem();
      } else {
        Animation::next(settings.play_one, settings.animation);
      }
    }
    // Commit current animation frame to the display
    Display::update();
  }
}

// Override end method if more is needed than changing state and ending timer.
void Animation::end() {
  if (state == state_t::STARTING) {
    timer_ending = Timer(2.0f, timer_starting.ratio(), true);
  }
  else if (state == state_t::RUNNING) {
    timer_ending = 2.0f;
  }
  else if (state == state_t::ENDING && !time_reduction) {
    timer_ending = Timer(2.0f, timer_ending.ratio(), false);
  }
  state = state_t::ENDING;
  time_reduction = true;
}

// Get fps, if animate has been called than t > 0
float Animation::fps() {
  if (animation_timer.dt() > 0) {
    return 1 / animation_timer.dt();
  }
  return 0;
}

void FIREWORKS() {
  fireworks2.init();
  fireworks2.timer_running = fireworks1.timer_running;
}
void SCROLLER() { scroller.set_text("#MALT WHISKEY"); }
void TWINKELS1() {
  twinkels.set_mode(true, false);
  twinkels.set_color(Color(255, 150, 30));
  twinkels.set_clear();
}
void TWINKELS2() { twinkels.set_mode(false, true); }

jump_item_t Animation::get_item(uint16_t index) {
  const jump_item_t jump_table[] = {
      {"LED Test", "All LEDs color and sweep test", 0, &ledtest},
      {"Accelerometer", "Test accelerometer", 0, &accelerometer},
      {"Arrows", "Moving arrows", 0, &arrows},
      {"Atoms", "Electons arround nucleas", 0, &atoms},
      {"Cube", "Cube in a cube", 0, &cube},
      {"Fireworks", "Fireing Fireworks", &FIREWORKS, &fireworks1},
      {"Helix", "Double strand DNA", 0, &helix},
      {"Life", "Game of Life 3D", 0, &life},
      {"Mario", "Super Mario Run", 0, &mario},
      {"Plasma", "Perlin noise plasma field", 0, &plasma},
      {"Pong", "The classical game of Pong", 0, &pong},
      {"Scroller", "Circulair text scroller ", &SCROLLER, &scroller},
      {"Sinus", "3D Wave Function", 0, &sinus},
      {"Spectrum", "WiFi Spectrum Analyser", 0, &spectrum},
      {"Starfield", "To boldly go...", 0, &starfield},
      {"Fairylights", "Beautifull fairylights", &TWINKELS1, &twinkels},
      {"Multilights", "Multicolor fairylights", &TWINKELS2, &twinkels},
      {"Channel Test", "Physical channels 0 through 31", 0, &channeltest},
      {"Channel Color Test", "Fast colors on physical channels", 0,
       &channelcolortest},
      {"The Matrix", "Falling green digital rain", 0, &the_matrix},
      {"Spotted Sphere", "Accelerating spotted sphere", 0,
       &spotted_sphere},
      {"Supernova", "Collapsing star and expanding shockwave", 0,
       &supernova},
      {"Aurora", "Flowing curtains of colored light", 0, &aurora},
      {"Black Hole", "Spinning accretion disk and energy jets", 0,
       &black_hole},
      {"Metaballs", "Organic merging spheres of light", 0, &metaballs},
      {"Electric Storm", "Branching lightning across the cube", 0,
       &electric_storm},
      {"DNA Tunnel", "Double helix moving through a rotating tunnel", 0,
       &dna_tunnel},
      {"Eye of Sauron", "A fiery eye sweeping a red searchlight", 0,
       &eye_of_sauron},
      {"White Test", "All LEDs fade from black to full white and back", 0,
       &white_test},
      {"Red Test", "All LEDs pulse red", 0, &red_test},
      {0, 0, 0, 0}};
  const uint16_t JUMPITEMS = sizeof(jump_table) / sizeof(jump_item_t) - 1;
  if (index > JUMPITEMS)
    return jump_table[JUMPITEMS];
  else
    return jump_table[index];
}

void Animation::next(bool play_one, uint16_t index) {
  if (play_one) {
    // Play one specific animation endlessly
    jump_item_t jump = Animation::get_item(index);
    if (jump.object) {
      jump.object->init();
      jump.object->time_reduction = false;
      jump.object->timer_running = 0;
    }
    if (jump.custom_init) jump.custom_init();
  }
  else {
    // Play the next animation from the sequence
    jump_item_t jump = get_item(animation_sequence++);
    if (!jump.object) {
      animation_sequence = 0;
      jump = get_item(animation_sequence++);
    }
    if (jump.object) {
      jump.object->init();
      jump.object->time_reduction = false;
    }
    if (jump.custom_init) jump.custom_init();
  }
}
