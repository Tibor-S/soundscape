#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <AudioRecord.h>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

#include <iostream>
#include <chrono>
#include "HelloTriangleApplication.h"
#include <cstdlib>
#include <Visual.h>
#include <portaudio.h>

void printEnv(const char* var) {
    const char* value = std::getenv(var);
    if (value) {
        std::cout << var << ": " << value << std::endl;
    } else {
        std::cout << var << " is not set!" << std::endl;
    }
}


class Application : public InterFrame {
public:
    explicit Application(Visual* vis) : InterFrame(), m_vis(vis) {
        m_bone_displacement.resize(m_bar_count);
        m_image_count = vis->get_image_count();
        m_audio_record = new AudioRecord(32768);

        m_last_frame = std::chrono::steady_clock::now();

    }
    ~Application() override {
        delete m_audio_record;
    }

    void start_audio_recording() const {
        if (!m_audio_record->start_recording()) {
            throw std::runtime_error("Failed to start audio recording");
        }
    }

    void load_bars() const {
        const float bar_total_width = m_bars_width - m_bar_margin * (static_cast<float>(m_bar_count) - 1);
        const float bar_diameter = bar_total_width / static_cast<float>(m_bar_count);
        const float bar_scale_factor = bar_diameter / 2.0f;

        std::map<const char*, SpriteKind> sprite_load = {};
        for (size_t i = 0; i < m_bar_count; i++) {
            sprite_load[BAR_NAMES[i]] = BAR_SPRITE;
        }
        m_vis->load_sprites(sprite_load);
        auto initial_bone_buffer = BoneBuffer {
            .bone = {glm::mat4(1.0f), glm::mat4(1.0f)}
        };

        const float low = (- m_bars_width + bar_diameter) / 2.0f;
        for (int i = 0; i < m_bar_count; i++) {
            const auto sp = m_vis->get_sprite(BAR_NAMES[i]);
            SpriteModel data = {
                .model_matrix = scale(
                    translate(glm::mat4(1.0f),
                              glm::vec3(low + static_cast<float>(i) * (bar_diameter + m_bar_margin), 0.0f, 0.0f)),
                    glm::vec3(bar_scale_factor, bar_scale_factor, 1.0f)),
            };
            for (size_t j = 0; j < m_image_count; j++) {
                sp->set_buffer(j, 1, &data, sizeof(SpriteModel));
                sp->set_buffer(j, 2, &initial_bone_buffer, sizeof(BoneBuffer));
            }
        }
    }

    void inter_frame() override {
        m_frame += 1;

        const size_t first_frequency = 24;
        const size_t last_frequency = 4187;
        const size_t frequency_count = last_frequency - first_frequency;
        const auto current_frequencies = m_audio_record->current_frequencies();
        const size_t bin_count = m_bar_count;
        size_t frequencies_per_bin = frequency_count / bin_count;
        std::vector<float> amps(m_bar_count);



        float max_amp = 4.0f;
        for (size_t bin = 0; bin < bin_count; bin++) {
            float avg_amp = 0.0f;
            for (size_t i = 0; i < frequencies_per_bin; i++)
                avg_amp += current_frequencies[bin * frequencies_per_bin + i + first_frequency];
            amps[bin] = avg_amp / static_cast<float>(frequencies_per_bin);
            if (bin == 0) {
                amps[bin] / 2.0f;
            }
            if (amps[bin] > max_amp) {
                amps[bin] = max_amp;
            }
        }

        for (size_t i = 0; i < m_bar_count; i++) {
            const auto amp = amps[i];
            // using namespace std;
            // cout << "i: " << i << " amp: " << amp << endl;
            auto bone_buffer = BoneBuffer {};
            bone_buffer.bone[0] = translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, amp));
            bone_buffer.bone[1] = translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -amp));
            const auto sp = m_vis->get_sprite(BAR_NAMES[i]);

            for (size_t j = 0; j < m_image_count; j++) {
                sp->set_buffer(j, 2, &bone_buffer, sizeof(BoneBuffer));
            }
        }

        while (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - m_last_frame).
               count() < 1000 / m_frame_rate) {
        }
        m_last_frame = std::chrono::steady_clock::now();
    }

private:
    Visual* m_vis;
    size_t m_image_count;
    std::vector<float> m_bone_displacement;
    size_t m_frame = 0;
    size_t m_bar_count = 32;
    float m_bars_width = 16;
    float m_bar_margin = 0.1;

    AudioRecord* m_audio_record;

    size_t m_frame_rate = 60;
    std::chrono::time_point<std::chrono::steady_clock> m_last_frame;

};

#define 	SAMPLE_RATE   (44100)
#define 	FRAMES_PER_BUFFER   (512)
#define 	NUM_SECONDS   (5)
#define 	NUM_CHANNELS   (1)
#define 	DITHER_FLAG   (0)
#define 	WRITE_TO_FILE   (0)
#define 	PA_SAMPLE_TYPE   paFloat32
#define 	SAMPLE_SILENCE   (0.0f)
#define 	PRINTF_S_FORMAT   "%.8f"
typedef float 	SAMPLE;

typedef struct
{
    int          frameIndex;  /* Index into sample array. */
    int          maxFrameIndex;
    SAMPLE      *recordedSamples;
}
paTestData;
/* This routine will be called by the PortAudio engine when audio is needed.
 * It may call at interrupt level on some machines so don't do anything
 * that could mess up the system like calling malloc() or free().
*/
static int recordCallback( const void *inputBuffer, void *outputBuffer,
                           unsigned long framesPerBuffer,
                           const PaStreamCallbackTimeInfo* timeInfo,
                           PaStreamCallbackFlags statusFlags,
                           void *userData )
{
    paTestData *data = (paTestData*)userData;
    const SAMPLE *rptr = (const SAMPLE*)inputBuffer;
    SAMPLE *wptr = &data->recordedSamples[data->frameIndex * NUM_CHANNELS];
    long framesToCalc;
    long i;
    int finished;
    unsigned long framesLeft = data->maxFrameIndex - data->frameIndex;

    (void) outputBuffer; /* Prevent unused variable warnings. */
    (void) timeInfo;
    (void) statusFlags;
    (void) userData;

    if( framesLeft < framesPerBuffer )
    {
        framesToCalc = framesLeft;
        finished = paComplete;
    }
    else
    {
        framesToCalc = framesPerBuffer;
        finished = paContinue;
    }

    if( inputBuffer == NULL )
    {
        for( i=0; i<framesToCalc; i++ )
        {
            *wptr++ = SAMPLE_SILENCE;  /* left */
            if( NUM_CHANNELS == 2 ) *wptr++ = SAMPLE_SILENCE;  /* right */
        }
    }
    else
    {
        for( i=0; i<framesToCalc; i++ )
        {
            *wptr++ = *rptr++;  /* left */
            if( NUM_CHANNELS == 2 ) *wptr++ = *rptr++;  /* right */
        }
    }
    data->frameIndex += framesToCalc;
    return finished;
}
static int playCallback( const void *inputBuffer, void *outputBuffer,
                         unsigned long framesPerBuffer,
                         const PaStreamCallbackTimeInfo* timeInfo,
                         PaStreamCallbackFlags statusFlags,
                         void *userData )
{
    paTestData *data = (paTestData*)userData;
    SAMPLE *rptr = &data->recordedSamples[data->frameIndex * NUM_CHANNELS];
    SAMPLE *wptr = (SAMPLE*)outputBuffer;
    unsigned int i;
    int finished;
    unsigned int framesLeft = data->maxFrameIndex - data->frameIndex;

    (void) inputBuffer; /* Prevent unused variable warnings. */
    (void) timeInfo;
    (void) statusFlags;
    (void) userData;

    if( framesLeft < framesPerBuffer )
    {
        /* final buffer... */
        for( i=0; i<framesLeft; i++ )
        {
            *wptr++ = *rptr++;  /* left */
            if( NUM_CHANNELS == 2 ) *wptr++ = *rptr++;  /* right */
        }
        for( ; i<framesPerBuffer; i++ )
        {
            *wptr++ = 0;  /* left */
            if( NUM_CHANNELS == 2 ) *wptr++ = 0;  /* right */
        }
        data->frameIndex += framesLeft;
        finished = paComplete;
    }
    else
    {
        for( i=0; i<framesPerBuffer; i++ )
        {
            *wptr++ = *rptr++;  /* left */
            if( NUM_CHANNELS == 2 ) *wptr++ = *rptr++;  /* right */
        }
        data->frameIndex += framesPerBuffer;
        finished = paContinue;
    }
    return finished;
}
static paTestData data;
void done(PaError err) {
    Pa_Terminate();
    if( data.recordedSamples )       /* Sure it is NULL or valid. */
        free( data.recordedSamples );
    if( err != paNoError )
    {
        fprintf( stderr, "An error occurred while using the portaudio stream\n" );
        fprintf( stderr, "Error number: %d\n", err );
        fprintf( stderr, "Error message: %s\n", Pa_GetErrorText( err ) );
        err = 1;          /* Always return 0 or 1, but no other return codes. */
    }
}
int main() {
    // {
    //     std::vector<float> data(8);
    //
    //     for (int i = 0; i < data.size(); i++) {
    //         data[i] = static_cast<float>(glm::sin(2 * M_PI * static_cast<float>(i) / 8));
    //     }
    //
    //     std::vector<float> real(8);
    //     std::vector<float> imag(8);
    //     std::vector<float> out(8);
    //
    //     for (size_t k = 0; k < data.size(); k++) {
    //         for (size_t n = 0; n < data.size(); n++) {
    //             auto exp_real = glm::cos(- 2 * M_PI * (float)k * n / (float)data.size());
    //             auto exp_imag = glm::sin(- 2 * M_PI * (float)k * n / (float)data.size());
    //
    //             real[k] += data[n] * exp_real;
    //             imag[k] += data[n] * exp_imag;
    //         }
    //         out[k] = glm::length(glm::vec2(real[k], imag[k]));
    //     }
    //
    //
    //     using namespace std;
    //     cout << "out: {" << endl;
    //     for (const auto o : out) {
    //         cout << "\t" << o << endl;
    //     }
    //     cout << "}" << endl;
    // }

    // auto audio_record = new AudioRecord(32768);
    // if (!audio_record->is_alive()) {
    //     delete audio_record;
    //     return 1;
    // }
    //
    // if (!audio_record->start_recording()) {
    //     delete audio_record;
    //     return 1;
    // }
    //
    // // AudioRecord::sleep(5000);
    // std::cout << "Starting recording..." << std::endl;
    // Pa_Sleep(5000);
    // std::cout << "Stopping recording..." << std::endl;
    //
    // if (!audio_record->stop_recording()) {
    //     delete audio_record;
    //     return 1;
    // }
    //
    // const auto recorded_data = audio_record->recorded_data();
    // float max_val = 0;
    // for (int i = 0; i < recorded_data.size(); i++) {
    //     // std::cout << "index " << i << ": " << recorded_data[i] << std::endl;
    //     if (recorded_data[i] > max_val) {
    //         max_val = recorded_data[i];
    //     }
    // }
    // std::cout << "max val: " <<  max_val << std::endl;
    //
    //
    //
    // return 0;
    // PaStreamParameters  inputParameters,
    //                     outputParameters;
    // PaStream*           stream;
    // PaError             err = paNoError;
    // paTestData          data;
    // int                 i;
    // int                 totalFrames;
    // int                 numSamples;
    // int                 numBytes;
    // SAMPLE              max, val;
    // double              average;
    //
    // printf("patest_record.c\n"); fflush(stdout);
    //
    // data.maxFrameIndex = totalFrames = NUM_SECONDS * SAMPLE_RATE; /* Record for a few seconds. */
    // data.frameIndex = 0;
    // numSamples = totalFrames * NUM_CHANNELS;
    // numBytes = numSamples * sizeof(SAMPLE);
    // data.recordedSamples = (SAMPLE *) malloc( numBytes ); /* From now on, recordedSamples is initialised. */
    // if( data.recordedSamples == NULL )
    // {
    //     printf("Could not allocate record array.\n");
    //     done(err);
    //     return 0;
    // }
    // for( i=0; i<numSamples; i++ ) data.recordedSamples[i] = 0;
    //
    // err = Pa_Initialize();
    // if( err != paNoError ) {
    //     done(err);
    //     return 0;
    // }
    //
    // inputParameters.device = Pa_GetDefaultInputDevice(); /* default input device */
    // if (inputParameters.device == paNoDevice) {
    //     fprintf(stderr,"Error: No default input device.\n");
    //     done(err);
    //     return 0;
    // }
    // inputParameters.channelCount = NUM_CHANNELS;
    // inputParameters.sampleFormat = PA_SAMPLE_TYPE;
    // inputParameters.suggestedLatency = Pa_GetDeviceInfo( inputParameters.device )->defaultLowInputLatency;
    // inputParameters.hostApiSpecificStreamInfo = NULL;
    //
    // /* Record some audio. -------------------------------------------- */
    // err = Pa_OpenStream(
    //           &stream,
    //           &inputParameters,
    //           NULL,                  /* &outputParameters, */
    //           SAMPLE_RATE,
    //           FRAMES_PER_BUFFER,
    //           paClipOff,      /* we won't output out of range samples so don't bother clipping them */
    //           recordCallback,
    //           &data );
    // if( err != paNoError ) {
    //     done(err);
    //     return 0;
    // }
    //
    // err = Pa_StartStream( stream );
    // if( err != paNoError ) {
    //     done(err);
    //     return 0;
    // }
    // printf("\n=== Now recording!! Please speak into the microphone. ===\n"); fflush(stdout);
    //
    // while( ( err = Pa_IsStreamActive( stream ) ) == 1 )
    // {
    //     Pa_Sleep(1000);
    //     printf("index = %d\n", data.frameIndex ); fflush(stdout);
    // }
    // if( err < 0 ) {
    //     done(err);
    //     return 0;
    // }
    //
    // err = Pa_CloseStream( stream );
    // if( err != paNoError ) {
    //     done(err);
    //     return 0;
    // }
    //
    // /* Measure maximum peak amplitude. */
    // max = 0;
    // average = 0.0;
    // for( i=0; i<numSamples; i++ )
    // {
    //     val = data.recordedSamples[i];
    //     if( val < 0 ) val = -val; /* ABS */
    //     if( val > max )
    //     {
    //         max = val;
    //     }
    //     average += val;
    // }
    //
    // average = average / (double)numSamples;
    //
    // std::cout << "sample max amplitude = " << max << std::endl;
    // printf("sample average = %lf\n", average );
    //
    // return 0;
    printEnv("VULKAN_SDK");
    printEnv("VK_ICD_FILENAMES");
    printEnv("VK_LAYER_PATH");
    printEnv("DYLD_LIBRARY_PATH");
    auto vis = new Visual(1);

    auto camera = vis->get_camera();
    auto camera_data = camera->get_data();
    camera_data.view = glm::lookAt(glm::vec3(0.0f, 20.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f),
                                   glm::vec3(0.0f, 0.0f, 1.0f));
    camera->set_data(camera_data);

    Application app(vis);

    app.load_bars();
    app.start_audio_recording();

    try {
        vis->run(&app);
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
