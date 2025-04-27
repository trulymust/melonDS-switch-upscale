#include <fstream>
#include <string>
#include <switch.h>
#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <cstring>
#include <cstdlib>
extern "C" {
    #include <rc_api_user.h>
    #include <rc_api_runtime.h>
}

    

static std::string g_response;

static size_t switch_curl_write_callback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t realsize = size * nmemb;
    g_response.append((char*)contents, realsize);
    return realsize;
}

const char* send_http_request(const char* url, const char* post_data, int* status_code) {
    g_response.clear();

    CURL* curl = curl_easy_init();
    if (!curl) {
        *status_code = -1;
        return nullptr;
    }

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_data);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, switch_curl_write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, nullptr);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L); 
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L); 

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        *status_code = -1;
        curl_easy_cleanup(curl);
        return nullptr;
    }

    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    *status_code = static_cast<int>(http_code);

    curl_easy_cleanup(curl);

    char* response_cstr = static_cast<char*>(malloc(g_response.size() + 1));
    if (response_cstr) {
        std::memcpy(response_cstr, g_response.c_str(), g_response.size() + 1);
    }

    return response_cstr;
}

void InitRetroAchievements() {
    rc_api_login_request_t api_params;
    memset(&api_params, 0, sizeof(api_params));

    api_params.username = "Gheovgos";
    api_params.api_token = "PsyVsTzJE2eJBs8BikiXVzu2aCCrXQo7";
    api_params.password = "DagothUr7";

    rc_api_request_t api_request;

    rc_api_init_login_request(&api_request, &api_params);

    int status_code;

    const char* response_body = send_http_request(api_request.url, api_request.post_data, &status_code);

    // rc_api_request_destroy(&api_request);
    
}