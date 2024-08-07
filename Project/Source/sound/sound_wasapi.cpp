
#include "sound/sound_generic.h"

#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN
#include "windows.h"

#include <mmsystem.h>
#include <mmdeviceapi.h>
#include <audioclient.h>

#define Wasapi_SafeRelease(ppT) do { if (*ppT) { (*ppT)->Release(); *ppT = NULL; } } while(0)

struct SoundManager
{
    
};

static void output_test_sine_wave_f32(s32 samples_per_second, s32 sample_count, f32 *samples, f32 tone_hz = 440, f32 tone_volume = 0.5) {
    static f64 t_sine = 0;
    int wave_period = (int)(samples_per_second / tone_hz);
    
    
    f32 *sample_out = samples;
    for (int sample_index = 0; sample_index < sample_count; sample_index++) {
        f32 sine_value = (float)sin(t_sine);
        f32 sample_value = (f32)(sine_value * tone_volume);
        *sample_out++ = sample_value;
        *sample_out++ = sample_value;
        
        t_sine += Pi * 2.0f / (f32)wave_period;
        if (t_sine >= Pi * 2.0f) {
            t_sine -= Pi * 2.0f;
        }
    }
}

DWORD Wasapi_ThreadProc(void* arg)
{
    // From:
    // https://hero.handmade.network/forums/code-discussion/t/8433-correct_implementation_of_wasapi
    // Used mainly as a starting example
    
    HRESULT hr;
    hr = CoInitializeEx(0, COINIT_SPEED_OVER_MEMORY);
    assert(SUCCEEDED(hr));
    
    IMMDevice* device = nullptr;
    IMMDeviceEnumerator* deviceEnumerator = nullptr;
    
    hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&deviceEnumerator));
    assert(SUCCEEDED(hr));
    
    hr = deviceEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &device);
    assert(SUCCEEDED(hr));
    
    IAudioClient* audioClient = NULL;
    hr = device->Activate(__uuidof(IAudioClient), CLSCTX_INPROC_SERVER, NULL, (void**)(&audioClient));
    assert(SUCCEEDED(hr));
    
    WAVEFORMATEX* mixFormat = NULL;
    audioClient->GetMixFormat(&mixFormat);
    assert(mixFormat->nChannels == 2);
    assert(mixFormat->wBitsPerSample == 32);
    
    REFERENCE_TIME bufferDuration = 30 * 10000; // 30ms
    DWORD flags = AUDCLNT_STREAMFLAGS_EVENTCALLBACK | AUDCLNT_STREAMFLAGS_NOPERSIST;
    hr = audioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, flags, bufferDuration, 0, mixFormat, NULL);
    assert(SUCCEEDED(hr));
    
    IAudioRenderClient* renderClient = NULL;
    hr = audioClient->GetService(IID_PPV_ARGS(&renderClient));
    assert(SUCCEEDED(hr));
    
    UINT32 bufferFrameCount = 0;
    hr = audioClient->GetBufferSize(&bufferFrameCount);
    assert(SUCCEEDED(hr));
    
    HANDLE refillEvent = CreateEventEx(NULL, NULL, 0, EVENT_MODIFY_STATE | SYNCHRONIZE);
    hr = audioClient->SetEventHandle(refillEvent);
    assert(SUCCEEDED(hr));
    
    {
        BYTE* data = nullptr;
        hr = renderClient->GetBuffer(bufferFrameCount, &data);
        assert(SUCCEEDED(hr));
        
        hr = renderClient->ReleaseBuffer(bufferFrameCount, AUDCLNT_BUFFERFLAGS_SILENT);
        assert(SUCCEEDED(hr));
    }
    
    u64 totalFramesWritten = 0;
    
    hr = audioClient->Start();
    assert(SUCCEEDED(hr));
    
    while(true)
    {
        auto res = WaitForSingleObject(refillEvent, INFINITE);
        if(res != WAIT_OBJECT_0) return 0;
        
        IAudioClock* audioClock = nullptr;
        audioClient->GetService(IID_PPV_ARGS(&audioClock));
        if(audioClock)
        {
            u64 freq = 0;
            u64 position = 0;
            audioClock->GetFrequency(&freq);
            audioClock->GetPosition(&position, nullptr);
            
            double sec = (double)position / (double)freq;
            u64 totalBytesWritten = totalFramesWritten * mixFormat->nBlockAlign;
            audioClock->Release();
        }
        
        bool defaultDeviceChanged = false;
        {
            IMMDevice* currentDefaultDevice = nullptr;
            deviceEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &currentDefaultDevice);
            
            LPWSTR id1;
            LPWSTR id2;
            device->GetId(&id1);
            currentDefaultDevice->GetId(&id2);
            
            defaultDeviceChanged = wcscmp(id1, id2) != 0;
            
            CoTaskMemFree(id1);
            CoTaskMemFree(id2);
            currentDefaultDevice->Release();
        }
        
        // See how much buffer space is available
        u32 numFramesPadding = 0;
        HRESULT hr = audioClient->GetCurrentPadding(&numFramesPadding);
        
        // Check for device change
        if (hr == AUDCLNT_E_DEVICE_INVALIDATED || defaultDeviceChanged)
        {
            hr = audioClient->Stop();
            assert(SUCCEEDED(hr));
            
            Wasapi_SafeRelease(&renderClient);
            Wasapi_SafeRelease(&audioClient);
            Wasapi_SafeRelease(&device);
            
            {
                hr = deviceEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &device);
                assert(SUCCEEDED(hr));
                
                hr = device->Activate(__uuidof(IAudioClient), CLSCTX_INPROC_SERVER, NULL, (void**)(&audioClient));
                assert(SUCCEEDED(hr));
                
                audioClient->GetMixFormat(&mixFormat);
                assert(mixFormat->nChannels == 2);
                assert(mixFormat->wBitsPerSample == 32);
                
                hr = audioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, flags, bufferDuration, 0, mixFormat, NULL);
                assert(SUCCEEDED(hr));
                
                hr = audioClient->GetService(IID_PPV_ARGS(&renderClient));
                assert(SUCCEEDED(hr));
                
                hr = audioClient->GetBufferSize(&bufferFrameCount);
                assert(SUCCEEDED(hr));
                
                hr = audioClient->SetEventHandle(refillEvent);
                assert(SUCCEEDED(hr));
            } 
            
            hr = audioClient->Start();
            continue;
        }
        
        // Output sound
        UINT32 sampleCount = bufferFrameCount - numFramesPadding;
        if (sampleCount > 0)
        {
            // Grab all the available space in the shared buffer.
            BYTE *data = NULL;
            hr = renderClient->GetBuffer(sampleCount, &data);
            
            f32* fData = (f32 *)data;
            
            auto samplesPerSecond = mixFormat->nSamplesPerSec;
            f32 *samples = (f32 *)data;
            {
                //output_test_sine_wave_f32(samplesPerSecond, sampleCount, samples, 440, 0.5);
            }
            
            hr = renderClient->ReleaseBuffer(sampleCount, 0);
            totalFramesWritten += sampleCount;
        }
    }
    
    return 0;
}

void S_Init()
{
    HANDLE stdoutHandle = GetStdHandle(STD_OUTPUT_HANDLE);
    if (!stdoutHandle)
    {
        AttachConsole(ATTACH_PARENT_PROCESS);
    }
    
    DWORD threadId = 0;
    HANDLE soundThread = CreateThread(0, 0, Wasapi_ThreadProc, 0, 0, &threadId);
    SetThreadPriority(soundThread, THREAD_PRIORITY_TIME_CRITICAL);
}

void S_Cleanup()
{
    
}
