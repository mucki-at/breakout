//!@author mucki (code@mucki.dev)
//!@copyright Copyright (c) 2025
//! please see LICENSE file in root folder for licensing terms.

#include "audiomanager.h"
#include <SDL3/SDL_audio.h>

namespace
{

class SimpleStream : public AudioManager::IStream
{
public:
    SimpleStream(SDL_AudioDeviceID device, const SDL_AudioSpec& spec, span<const byte> data) :
        device(device),
        buffer(data.begin(), data.end())
    {
        SDL_AudioSpec mixSpec;
        SDL_GetAudioDeviceFormat(device, &mixSpec, nullptr);
        stream=SDL_CreateAudioStream(&spec, &mixSpec); 
        if (!stream) throw runtime_error("Failed to create audio stream: "s+SDL_GetError());
    }

    virtual ~SimpleStream()
    {
        if (stream)
        {
            SDL_DestroyAudioStream(stream);
        } 
    }

    virtual void play() noexcept
    {
        stop();

        if (!SDL_PutAudioStreamData(stream, buffer.data(), buffer.size()))
            cerr << "Failed to initialize audio stream: " << SDL_GetError() <<endl;
        SDL_BindAudioStream(device, stream);
    }

    virtual void stop() noexcept
    {
        SDL_UnbindAudioStream(stream);
        SDL_ClearAudioStream(stream);
    }

private:
    SDL_AudioDeviceID device;
    vector<byte> buffer;
    SDL_AudioStream* stream;
};

}

AudioManager::AudioManager() :
    device(0)
{
    device=SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, nullptr);
    if (device==0) throw runtime_error("Failed to open auto device: "s+SDL_GetError());
}

AudioManager::~AudioManager()
{
    if (device)
    {
        SDL_CloseAudioDevice(device);
    }
}


AudioManager::Audio AudioManager::createSimpleAudio(size_t sampleRate, span<float> samples)
{
    return make_shared<SimpleStream>(
        device,
        SDL_AudioSpec{
            .format=SDL_AudioFormat::SDL_AUDIO_F32,
            .channels=1,
            .freq=static_cast<int>(sampleRate)
        },
        as_bytes(samples));
}

AudioManager::Audio AudioManager::createTone(float frequency, float length, size_t sampleRate)
{
    // Step 1: fudge length so that we will have a zero crossing at the end
    auto cycles=floor(frequency*length);
    length=cycles/frequency;
    auto samples=vector<float>(floor(static_cast<float>(sampleRate)*length));
    size_t t=0.0;
    float step=(frequency*2.0*M_PI)/static_cast<float>(sampleRate);
    for (auto& s : samples)
    {
        s=sin(static_cast<float>(t)*step);
        ++t;
    }

    return make_shared<SimpleStream>(device,
        SDL_AudioSpec{
            .format=SDL_AudioFormat::SDL_AUDIO_F32,
            .channels=1,
            .freq=static_cast<int>(sampleRate)
        },
        as_bytes(span(samples))
    );
}

AudioManager::Audio AudioManager::loadWav(const filesystem::path& file)
{
    SDL_AudioSpec spec;
    Uint8* data;
    Uint32 length;

    if (!SDL_LoadWAV(file.c_str(), &spec, &data, &length)) throw runtime_error("Failed to open auto device: "s+SDL_GetError());
    auto result=make_shared<SimpleStream>(device,spec,span<const byte>(reinterpret_cast<const byte*>(data), length));
    SDL_free(data);
    return result;
}

