// //
// // Created by Sebastian Sandstig on 2024-12-29.
// // Borrowed heavily from the Vibra repository: https://github.com/BayernMuller/vibra
// //
//
// #include <random>
// #include <sstream>
#include <fstream>
#include <iostream>
#include <string>
#include <communication/content_download.h>
#include <curl/curl.h>
// #define STB_IMAGE_IMPLEMENTATION
// #include <stb_image.h>
// #include <vibra/communication/shazam.h>
// #include <vibra/communication/timezones.h>
// #include <vibra/communication/user_agents.h>

// COPIED FROM VIBRA: https://github.com/BayernMuller/vibra
std::size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp)
{
    std::string *buffer = reinterpret_cast<std::string *>(userp);
    std::size_t realsize = size * nmemb;
    buffer->append(reinterpret_cast<char *>(contents), realsize);
    return realsize;
}
// PARTS COPIED FROM VIBRA: https://github.com/BayernMuller/vibra
std::string Communication::load_cover_art(const std::string &url) {
    CURL *curl = curl_easy_init();
    std::string buffer;

    if (curl == nullptr) throw std::runtime_error("curl_easy_init() failed");

    struct curl_slist *headers = nullptr;
    headers = curl_slist_append(headers, "Accept-Encoding: gzip, deflate, br");
    headers = curl_slist_append(headers, "Accept: */*");
    headers = curl_slist_append(headers, "Connection: keep-alive");
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, "Content-Language: en_US");
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buffer);

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK)
    {
        std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
    }

    std::int64_t http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    if (http_code != 200)
    {
        std::cerr << "HTTP code: " << http_code << std::endl;
    }
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    return buffer;
}



//
//
// std::string get_request_content()
// {
//     std::mt19937 gen(std::random_device{}());
//     std::uniform_int_distribution<> dis_float(0.0, 1.0);
//
//     const char* timezone;
//     {
//         std::mt19937 gen(std::random_device{}());
//         std::uniform_int_distribution<> dis_timezone(0, EUROPE_TIMEZONES_SIZE - 1);
//         timezone = EUROPE_TIMEZONES[dis_timezone(gen)];
//     }
//     double fuzz = dis_float(gen) * 15.3 - 7.65;
//
//     std::stringstream json_buf;
//     json_buf << "{";
//     json_buf << "\"geolocation\":{";
//     json_buf << "\"altitude\":" << dis_float(gen) * 400 + 100 + fuzz << ",";
//     json_buf << "\"latitude\":" << dis_float(gen) * 180 - 90 + fuzz << ",";
//     json_buf << "\"longitude\":" << dis_float(gen) * 360 - 180 + fuzz;
//     json_buf << "},";
//     json_buf << "\"signature\":{";
//     // json_buf << "\"samplems\":" << sample_ms << ",";
//     json_buf << "\"timestamp\":" << time(nullptr) * 1000ULL << ",";
//     // json_buf << "\"uri\":\"" << uri << "\"";
//     json_buf << "},";
//     json_buf << "\"timestamp\":" << time(nullptr) * 1000ULL << ",";
//     json_buf << "\"timezone\":"
//              << "\"" << timezone << "\"";
//     json_buf << "}";
//     std::string content = json_buf.str();
//     return content;
// }
//
// std::string cover_art(const std::string url)
// {
//     auto content = get_request_content();
//     std::string user_agent;
//     {
//         std::mt19937 gen(std::random_device{}());
//         std::uniform_int_distribution<> dis_useragent(0, USER_AGENTS_SIZE - 1);
//         user_agent = USER_AGENTS[dis_useragent(gen)];
//     }
//     std::string url = Shazam::getShazamHost();
//
//     CURL *curl = curl_easy_init();
//     std::string read_buffer;
//
//     if (curl)
//     {
//         struct curl_slist *headers = nullptr;
//         headers = curl_slist_append(headers, "Accept-Encoding: gzip, deflate, br");
//         headers = curl_slist_append(headers, "Accept: */*");
//         headers = curl_slist_append(headers, "Connection: keep-alive");
//         headers = curl_slist_append(headers, "Content-Type: application/json");
//         headers = curl_slist_append(headers, "Content-Language: en_US");
//         curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
//         curl_easy_setopt(curl, CURLOPT_POST, 1L);
//         curl_easy_setopt(curl, CURLOPT_USERAGENT, user_agent.c_str());
//         curl_easy_setopt(curl, CURLOPT_POSTFIELDS, content.c_str());
//         curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
//         curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
//         curl_easy_setopt(curl, CURLOPT_WRITEDATA, &read_buffer);
//
//         curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "gzip, deflate, br");
//         curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1);
//
//         CURLcode res = curl_easy_perform(curl);
//         if (res != CURLE_OK)
//         {
//             std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
//         }
//
//         std::int64_t http_code = 0;
//         curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
//         if (http_code != 200)
//         {
//             std::cerr << "HTTP code: " << http_code << std::endl;
//         }
//         curl_slist_free_all(headers);
//         curl_easy_cleanup(curl);
//     }
//     return read_buffer;
// }
//
// // std::string Shazam::getShazamHost()
// // {
// //     std::string host = HOST + uuid4::generate() + "/" + uuid4::generate();
// //     host += "?sync=true&"
// //             "webv3=true&"
// //             "sampling=true&"
// //             "connected=&"
// //             "shazamapiversion=v3&"
// //             "sharehub=true&"
// //             "video=v3";
// //     return host;
// // }
