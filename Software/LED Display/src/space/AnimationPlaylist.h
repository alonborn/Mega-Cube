#ifndef ANIMATIONPLAYLIST_H
#define ANIMATIONPLAYLIST_H

struct AnimationPlaylistEntry {
  uint8_t animation_id;
  float duration_seconds;
};

// Edit this list to choose the animations, their order and play time.
const AnimationPlaylistEntry ANIMATION_PLAYLIST[] = {
    {ANIMATION_SINUS, 15.0f},
    {ANIMATION_AURORA, 12.0f},
    {ANIMATION_THE_MATRIX, 12.0f},
    {ANIMATION_SUPERNOVA, 16.2f},
    {ANIMATION_METABALLS, 12.0f},
    {ANIMATION_BLACK_HOLE, 10.0f},
    {ANIMATION_ELECTRIC_STORM, 15.0f},
    {ANIMATION_EYE_OF_SAURON, 15.0f},
    {ANIMATION_DNA_TUNNEL, 15.0f},
    {ANIMATION_SPOTTED_SPHERE, 15.0f},
    {ANIMATION_FIREWORKS, 15.0f},
};

const uint8_t ANIMATION_PLAYLIST_SIZE =
    sizeof(ANIMATION_PLAYLIST) / sizeof(AnimationPlaylistEntry);

#endif
