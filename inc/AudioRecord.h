//
// Created by Sebastian Sandstig on 2024-12-15.
//

#ifndef AUDIORECORD_H
#define AUDIORECORD_H
#include <fstream>
#include <iostream>
#include <portaudio.h>
#include <thread>
#include <vector>
#include <string.h>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <glm/trigonometric.hpp>
#include <vibra/vibra.h>
#include <vibra/communication/shazam.h>
#include <fstream>
#include <json.hpp>

using namespace nlohmann;

struct RecordData {
    size_t next_sample_index = 0;  /* Index into sample array. */
    std::vector<float> recorded_samples;
};

class AudioRecordCallback {
public:
    explicit AudioRecordCallback(const size_t buffer_size) {
        std::lock_guard<std::mutex> guard(m_mutex);

        m_record_data = std::make_unique<RecordData>();
        const auto record_data = m_record_data;
        record_data->next_sample_index = 0;
        record_data->recorded_samples.resize(buffer_size);

        m_continue_recording = std::make_unique<bool>();
        // const auto continue_recording = m_continue_recording;
        // *continue_recording = true;
        *m_continue_recording = true;
    }
    ~AudioRecordCallback() = default;
    int callback(const void *input_buffer, void *output_buffer,
                           const unsigned long frames_per_buffer,
                           const PaStreamCallbackTimeInfo* time_info,
                           PaStreamCallbackFlags status_flags)
    {
        // auto arc = static_cast<AudioRecordCallback *>(user_data);
        std::lock_guard<std::mutex> guard(m_mutex);

        const auto *read_ptr = static_cast<const float *>(input_buffer);
        size_t i;

        const auto record_data = m_record_data;
        if(input_buffer == nullptr) {
            for( i=0; i<frames_per_buffer; i++ )
            {
                record_data->recorded_samples[record_data->next_sample_index++] = 0.0f;
                if (record_data->next_sample_index >= record_data->recorded_samples.size()) {
                    record_data->next_sample_index = 0;
                }
            }
        }
        else
        {
            for(i=0; i<frames_per_buffer; i++) {
                record_data->recorded_samples[record_data->next_sample_index++] = read_ptr[i];
                if (record_data->next_sample_index >= record_data->recorded_samples.size()) {
                    record_data->next_sample_index = 0;
                }
            }
        }

        const auto continue_recording = m_continue_recording;
        if (*continue_recording)
            return paContinue;

        return paComplete;
    }

    void get_buffer(std::vector<float>* output) {
        std::lock_guard<std::mutex> guard(m_mutex);
        const auto record_data = m_record_data;
        // output->insert(output->begin(), record_data->recorded_samples.begin(), record_data->recorded_samples.end());
        size_t oi = 0;
        for(size_t i=m_record_data->next_sample_index; i<record_data->recorded_samples.size(); i++) {
            (*output)[oi++] = record_data->recorded_samples[i];
        }
        for(size_t i=0; i<m_record_data->next_sample_index; i++) {
            (*output)[oi++] = record_data->recorded_samples[i];
        }
    }
    void stop_recording() {
        std::lock_guard<std::mutex> guard(m_mutex);
        const auto continue_recording = m_continue_recording;
        *continue_recording = false;
    }
    bool stopped_recording() {
        std::lock_guard<std::mutex> guard(m_mutex);
        const auto continue_recording = m_continue_recording;
        return !*continue_recording;
    }
private:
    std::shared_ptr<RecordData> m_record_data;
    std::shared_ptr<bool> m_continue_recording;
    std::mutex m_mutex;
};

class RecognizeSong {
public:
    RecognizeSong() = default;
    ~RecognizeSong() = default;

    void recognize(const Fingerprint* fp) {
        m_song_data = json::parse(Shazam::Recognize(fp));
        std::cout << "Recognize Song Data: " << m_song_data << std::endl;
        if (!m_song_data.contains("track")) {
            reset_fields();
            return;
        }
        auto track = m_song_data["track"];

        std::optional<std::string> cover_art = std::nullopt;
        if (track.contains("images")) {
            if (const auto images = track["images"]; images.contains("coverart")) {
                cover_art = images["coverart"].template get<std::string>();
            }
        }
        m_cover_art = cover_art;
    }

    [[nodiscard]] std::optional<std::string> get_cover_art_url() const { return m_cover_art; }


private:
    basic_json<> m_song_data = json::parse(R"({})");
    std::optional<std::string> m_cover_art;

    void reset_fields() {
        m_song_data = json::parse(R"({})");
        m_cover_art = std::nullopt;
    }
};

class AudioRecord : RecognizeSong {
public:
    explicit AudioRecord(const size_t sample_rate = 44100, const size_t frame_size = 512) : RecognizeSong(),
        m_sample_rate(sample_rate), m_frame_size(frame_size), m_dft_real(m_sample_rate), m_dft_imag(m_sample_rate),
        m_dft_out(m_sample_rate / 2)
    {
        m_err = Pa_Initialize();
        if(m_err != paNoError) {
            m_is_alive = false;
            return;
        }

        m_input_parameters = {};
        m_input_parameters.device = Pa_GetDefaultInputDevice(); /* default input device */
        if (m_input_parameters.device == paNoDevice) {
            fprintf(stderr,"Error: No default input device.\n");
            m_is_alive = false;
            return;
        }
        m_input_parameters.channelCount = 1;
        m_input_parameters.sampleFormat = paFloat32;
        m_input_parameters.suggestedLatency = Pa_GetDeviceInfo(m_input_parameters.device)->defaultLowInputLatency;
        m_input_parameters.hostApiSpecificStreamInfo = nullptr;
    }
    ~AudioRecord() {
        stop_recognition();
        Pa_Terminate();
        if(m_err != paNoError)
        {
            fprintf( stderr, "An error occurred while using the portaudio stream\n" );
            fprintf( stderr, "Error number: %d\n", m_err );
            fprintf( stderr, "Error message: %s\n", Pa_GetErrorText( m_err ) );
            m_err = 1;          /* Always return 0 or 1, but no other return codes. */
        }
        delete m_arc;
    }

    [[nodiscard]] bool start_recording() {
        if(!m_is_alive) return false;
        if (m_recording_thread != nullptr) return true;
        delete m_arc;
        m_arc = new AudioRecordCallback(m_sample_rate);
        m_recording_thread = new std::thread(auxiliary_start_recording, m_stream, &m_input_parameters,
                                             static_cast<double>(m_sample_rate),
                                             m_frame_size,
                                             m_arc);

        return m_is_alive;
    }
    [[nodiscard]] bool is_active() {
        if(!m_is_alive) return false;

        m_err = Pa_IsStreamActive(m_stream);
        if (m_err < 0) {
            m_is_alive = false;
            return false;
        }

        return true;
    }
    [[nodiscard]] bool stop_recording() {
        if(!m_is_alive) return false;

        if (m_arc != nullptr) {
            m_arc->stop_recording();
        }

        if (m_recording_thread != nullptr) {
            m_recording_thread->join();
            delete m_recording_thread;
            m_recording_thread = nullptr;
        }

        return m_is_alive;
    }
    [[nodiscard]] bool is_alive() const { return m_is_alive; }
    [[nodiscard]] std::vector<float> recorded_data() const {
        std::vector<float> recorded_samples;
        if (m_arc != nullptr) {
            recorded_samples.resize(m_sample_rate);
            m_arc->get_buffer(&recorded_samples);
        }
        return recorded_samples;
    }
    [[nodiscard]] std::vector<float>& current_frequencies() {
        const auto data = recorded_data();
        auto magnitude = static_cast<size_t>(glm::log2(static_cast<float>(data.size())));
        auto n = data.size();
        for (size_t k = 0; k < n; k++) {
            size_t rev_k = 0;
            for (size_t b = 0; b < magnitude; b++) {
                rev_k <<= 1;
                if (k & (1 << b)) rev_k |= 1;
            }

            m_dft_real[rev_k] = data[k];
            m_dft_imag[rev_k] = 0;
        }

        for (size_t s = 1; s < magnitude + 1; s++) {
            const auto float_s = static_cast<double>(s);
            const double m = glm::pow(2, float_s);
            const auto w_m = glm::vec2(glm::cos(- 2 * M_PI / m), glm::sin(- 2 * M_PI / m));

            for (size_t k = 0; k < n; k += static_cast<size_t>(m)) {
                auto w = glm::vec2(1, 0);
                const auto m_by_2 = static_cast<size_t>(m / 2);
                for (size_t j = 0; j < m_by_2; j++) {
                    const size_t t_index = k + j + m_by_2;
                    const auto t = glm::vec2(w.r * m_dft_real[t_index] - w.g * m_dft_imag[t_index],
                                             w.r * m_dft_imag[t_index] + w.g * m_dft_real[t_index]);
                    const size_t u_index = k + j;
                    const auto u = glm::vec2(m_dft_real[u_index], m_dft_imag[u_index]);

                    m_dft_real[u_index] = u.r + t.r;
                    m_dft_imag[u_index] = u.g + t.g;
                    m_dft_real[t_index] = u.r - t.r;
                    m_dft_imag[t_index] = u.g - t.g;

                    w.r = w.r * w_m.r - w.g * w_m.g;
                    w.g = w.r * w_m.g + w.g * w_m.r;
                }
            }
        }

        for (size_t i = 0; i < m_dft_out.size(); i++) {
            if (i == 0) {
                m_dft_out[i] = glm::length(glm::vec2(m_dft_real[i], m_dft_real[i]));
                continue;
            }
            m_dft_out[i] = glm::length(glm::vec2(m_dft_real[n - i + 1], m_dft_real[n - i + 1])) + glm::length(
                               glm::vec2(m_dft_real[i], m_dft_real[i]));
        }

        return m_dft_out;
    }
    void start_recognition() {
        std::lock_guard<std::mutex> guard(m_recognition_mutex);
        if (m_recognition_thread != nullptr) {
            using namespace std;
            cout << "Tried starting recognition when it has already started" << endl;
        }
        m_continue_recognition = true;
        m_recognition_thread = new std::thread(&AudioRecord::auxiliary_recognize, this);
    }
    void stop_recognition() {
        {
            std::lock_guard<std::mutex> guard(m_recognition_mutex);
            m_continue_recognition = false;
        }
        if (m_recognition_thread == nullptr) {
            return;
        }
        m_recognition_thread->join();
        delete m_recognition_thread;
        m_recognition_thread = nullptr;
    }
private:
    size_t m_sample_rate;
    size_t m_frame_size;
    PaError m_err = paNoError;
    PaStreamParameters m_input_parameters{};
    PaStream* m_stream = nullptr;

    AudioRecordCallback* m_arc = nullptr;
    std::thread* m_recording_thread = nullptr;

    bool m_continue_recognition;
    std::thread* m_recognition_thread;
    std::mutex m_recognition_mutex;


    std::vector<float> m_dft_real;
    std::vector<float> m_dft_imag;
    std::vector<float> m_dft_out;

    bool m_is_alive = true;

    struct RecognitionData {
        bool fill_buffer;
        std::vector<float> buffer;
        size_t buffer_limit;
        size_t index;
    };

    static int callback(const void *input_buffer, void *output_buffer,
                           const unsigned long frames_per_buffer,
                           const PaStreamCallbackTimeInfo* time_info,
                           PaStreamCallbackFlags status_flags,
                           void *user_data)
    {
        const auto arc = static_cast<AudioRecordCallback*>(user_data);
        return arc->callback(input_buffer, output_buffer, frames_per_buffer, time_info, status_flags);
    }

    static PaError auxiliary_start_recording(PaStream *stream, const PaStreamParameters *input_parameters,
                                             const double sample_rate,
                                             const size_t frame_size, AudioRecordCallback *arc) {
        PaError err = Pa_OpenStream(
            &stream,
            input_parameters,
            nullptr, /* &outputParameters, */
            sample_rate,
            frame_size,
            paClipOff, /* we won't output out of range samples so don't bother clipping them */
            &callback,
            arc);
        if(err != paNoError) {
            return err;
        }

        err = Pa_StartStream(stream);
        if( err != paNoError ) {
            return err;
        }

        while(!arc->stopped_recording()) {
            Pa_Sleep(100);
        }

        err = Pa_StopStream(stream);
        if(err != paNoError) {
            return err;
        }

        // delete *arc;
        // *arc = nullptr;
        return err;
    }

    static int recognize_callback(const void *input_buffer, void *output_buffer,
                           const unsigned long frames_per_buffer,
                           const PaStreamCallbackTimeInfo* time_info,
                           PaStreamCallbackFlags status_flags,
                           void *user_data)
    {

        const auto recognition_data = static_cast<RecognitionData*>(user_data);
        if (!recognition_data->fill_buffer) return paContinue;
        const auto *read_ptr = static_cast<const float *>(input_buffer);

        const auto free_space = recognition_data->buffer_limit - recognition_data->index;
        const auto end = std::min(free_space, frames_per_buffer);
        for(size_t i=0; i<end; i++) {
            recognition_data->buffer[recognition_data->index++] = read_ptr[i];
        }

        if (recognition_data->index >= recognition_data->buffer_limit) {
            recognition_data->fill_buffer = false;
        }

        return paContinue;
    }
    void auxiliary_recognize() {
        using namespace std;
        PaStream* stream;
        RecognitionData recognition_data {
            .fill_buffer = false,
            .buffer_limit = 44100*4,
            .index = 0
        };
        recognition_data.buffer = std::vector<float>(recognition_data.buffer_limit);
        cout << "Buffer max: " << ranges::max(recognition_data.buffer) << endl;
        cout << "Buffer min: " << ranges::min(recognition_data.buffer) << endl;
        PaError err = Pa_OpenStream(
            &stream,
            &m_input_parameters,
            nullptr, /* &outputParameters, */
            44100,
            512,
            paClipOff, /* we won't output out of range samples so don't bother clipping them */
            &recognize_callback,
            &recognition_data
        );
        if(err != paNoError) {
            // return err;
        }

        err = Pa_StartStream(stream);
        if( err != paNoError ) {
            // return err;
        }


        while(continue_recognition()) {
            cout << "new loop" << endl;

            recognition_data.index = 0;
            recognition_data.fill_buffer = true;
            cout << "recording" << endl;
            Pa_Sleep(5000);
            cout << "samples: " << recognition_data.index << " and " << recognition_data.buffer.size() << endl;
            cout << "Buffer max: " << ranges::max(recognition_data.buffer) << endl;
            cout << "Buffer min: " << ranges::min(recognition_data.buffer) << endl;
            recognition_data.fill_buffer = false;
            {
                cout << "Getting recognition lock in loop" << endl;
                std::lock_guard<std::mutex> guard(m_recognition_mutex);
                auto recorded_data = recognition_data.buffer;
                cout << "Getting finger print" << endl;
                auto fp = vibra_get_fingerprint_from_float_pcm(reinterpret_cast<const char *>(recorded_data.data()),
                                                               recognition_data.index * sizeof(float), 44100,
                                                               sizeof(float) * 8, 1);
                cout << "Fingerprint: " << fp->uri << endl;
                recognize(fp);
                cout << get_cover_art_url().value_or("No cover art") << endl;
            }

            cout << "Sleeping loop" << endl;
            Pa_Sleep(10000);
        }


        err = Pa_StopStream(stream);
        if(err != paNoError) {
            // return err;
        }
        // recognize_callback->call(fp);
        // std::ofstream outFile("/Users/sebastian/CLionProjects/soundscape/my_file.txt");
        // // the important part
        // for (const auto &e : data) outFile << e << ", ";
    }

    bool continue_recognition() {
        std::lock_guard<std::mutex> guard(m_recognition_mutex);
        return m_continue_recognition;
    }
};



#endif //AUDIORECORD_H
