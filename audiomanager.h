//!@author mucki (code@mucki.dev)
//!@copyright Copyright (c) 2025
//! please see LICENSE file in root folder for licensing terms.
#pragma once

#include "common.h"

class AudioManager
{
public:
    class IStream
    {
    public:
        virtual ~IStream() noexcept {}

        virtual void play() noexcept=0;
        virtual void stop() noexcept=0;
    };

    using Audio=shared_ptr<IStream>;

public:
    AudioManager();
    ~AudioManager();

    Audio createSimpleAudio(size_t sampleRate, span<float> samples);
    Audio createTone(float frequency, float length, size_t sampleRate);
    Audio loadWav(const filesystem::path& file);
    
private:
    uint32_t device;
};