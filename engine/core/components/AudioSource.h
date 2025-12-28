#pragma once

#include "core/AudioPlayer.h"

#include <glm/glm.hpp>
#include <memory>
#include <string>

#ifdef EDITOR_BUILD
#include <yaml-cpp/yaml.h>
#endif

// Not to be confused with OpenAL audio source.
// It contains the serializable data required to
// play audio. AudioPlayer is more inlign with the
// OpenAL audio source.
// TODO: think about how player are working now and
// see if we can have a consistent API across the board.
// For animations, and audio.
struct AudioSource
{
    std::string filePath; // path to .wav/.ogg
    bool looping = false;
    float gain = 1.0f;
    float pitch = 1.0f;

    // AudioPlayer runtime
    AudioPlayer audioPlayer;

#ifdef EDITOR_BUILD
    void serialize(YAML::Emitter &out);
    void deserialize(const YAML::Node &in);
    void drawInspector();
    void handleAudioDrop();
#endif
};
