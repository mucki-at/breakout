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

    class Variations : public AudioManager::IStream
    {
    public:
        Variations();
        void addVariation(AudioManager::Audio var);
        virtual void play() noexcept;
        virtual void stop() noexcept;
    
    private:
        vector<AudioManager::Audio> variations;
        vector<AudioManager::Audio>::iterator current;
    };

public:
    AudioManager();
    ~AudioManager();

    Audio createSimpleAudio(size_t sampleRate, span<float> samples);
    Audio createTone(float frequency, float length, size_t sampleRate);
    Audio loadWav(const filesystem::path& file);

    template<typename... U>
    inline Audio loadWavWithVariations(U&&... files)
    {
        auto result=make_shared<Variations>();
        auto vars = {loadWav(std::forward<U>(files))... };
        for (auto&& v : vars) result->addVariation(v);
        return result;
    }


private:
    uint32_t device;
};