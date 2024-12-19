//
// Created by Sebastian Sandstig on 2024-12-15.
//

#ifndef AUDIORECORD_H
#define AUDIORECORD_H
#include <fstream>
#include <iostream>
#include <portaudio.h>
#include <thread>
#include <utility>
#include <vector>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <glm/trigonometric.hpp>
#include <vibra/vibra.h>
#include <vibra/communication/shazam.h>
#include <fstream>

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

class AudioRecord {
public:
    explicit AudioRecord(const size_t sample_rate = 44100, const size_t frame_size = 512) : m_sample_rate(sample_rate),
        m_frame_size(frame_size), m_dft_real(m_sample_rate), m_dft_imag(m_sample_rate), m_dft_out(m_sample_rate / 2)
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
    void recognize() {
        m_recording_thread = new std::thread(auxiliary_recognize, &m_input_parameters);
        // auxiliary_recognize(&m_input_parameters);
    }
private:
    size_t m_sample_rate;
    size_t m_frame_size;
    PaError m_err = paNoError;
    PaStreamParameters m_input_parameters{};
    PaStream* m_stream = nullptr;

    AudioRecordCallback* m_arc = nullptr;
    std::thread* m_recording_thread = nullptr;

    std::vector<float> m_dft_real;
    std::vector<float> m_dft_imag;
    std::vector<float> m_dft_out;

    bool m_is_alive = true;

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
        const auto recorded_data = static_cast<std::vector<float>*>(user_data);
        const auto *read_ptr = static_cast<const float *>(input_buffer);


        for(size_t i=0; i<frames_per_buffer; i++) {
            recorded_data->push_back(read_ptr[i]);
        }

        return paContinue;
    }
    static void auxiliary_recognize(const PaStreamParameters* input_parameters) {
        PaStream* stream;
        std::vector<float> recorded_data;
        PaError err = Pa_OpenStream(
            &stream,
            input_parameters,
            nullptr, /* &outputParameters, */
            44100,
            512,
            paClipOff, /* we won't output out of range samples so don't bother clipping them */
            &recognize_callback,
            &recorded_data
        );
        if(err != paNoError) {
            // return err;
        }

        err = Pa_StartStream(stream);
        if( err != paNoError ) {
            // return err;
        }

        Pa_Sleep(5000);

        err = Pa_StopStream(stream);
        if(err != paNoError) {
            // return err;
        }

        auto fp = vibra_get_fingerprint_from_float_pcm(reinterpret_cast<const char *>(recorded_data.data()), recorded_data.size() * sizeof(float), 44100, sizeof(float) * 8, 1);
        auto song_data = Shazam::Recognize(fp);
        using namespace std;
        cout << song_data << endl;
        // std::ofstream outFile("/Users/sebastian/CLionProjects/soundscape/my_file.txt");
        // // the important part
        // for (const auto &e : data) outFile << e << ", ";
    }
};



#endif //AUDIORECORD_H
