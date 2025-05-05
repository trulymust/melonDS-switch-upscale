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
    #include <rc_client.h>
}

    
static std::string g_response;
rc_client_t* g_client = NULL;

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

// This is the function the rc_client will use to read memory for the emulator. we don't need it yet,
// so just provide a dummy function that returns "no memory read".

static uint32_t read_memory(uint32_t address, uint8_t* buffer, uint32_t num_bytes, rc_client_t* client)
{
  // TODO: implement later
  return 0;
}
// This is the HTTP request dispatcher that is provided to the rc_client. Whenever the client
// needs to talk to the server, it will call this function.
static void server_call(const rc_api_request_t* request, rc_client_server_callback_t callback, void* callback_data, rc_client_t* client) {
  int status_code;
  const char* response = send_http_request(request->url, request->post_data, &status_code);

  rc_api_server_response_t server_response;
  memset(&server_response, 0, sizeof(server_response));
  server_response.http_status_code = status_code;
  server_response.body = response;

  callback(&server_response, callback_data);

  if (response) {
  free((void*)response);
  }
}


// Write log messages to the console
static void log_message(const char* message, const rc_client_t* client)
{
  printf("%s\n", message);
}

void initialize_retroachievements_client(void)
{
  // Create the client instance (using a global variable simplifies this example)
  g_client = rc_client_create(read_memory, server_call);

  // Provide a logging function to simplify debugging
  rc_client_enable_logging(g_client, RC_CLIENT_LOG_LEVEL_VERBOSE, log_message);

  // Disable hardcore - if we goof something up in the implementation, we don't want our
  // account disabled for cheating.
  rc_client_set_hardcore_enabled(g_client, 0);
}

void shutdown_retroachievements_client(void)
{
  if (g_client)
  {
    // Release resources associated to the client instance
    rc_client_destroy(g_client);
    g_client = NULL;
  }  
}

int InitRetroAchievements(const char* username, const char* password) {
    rc_api_login_request_t api_params;
    memset(&api_params, 0, sizeof(api_params));

    api_params.username = username;
    api_params.password = password;

    rc_api_request_t api_request;

    rc_api_init_login_request(&api_request, &api_params);

    int status_code;

    const char* response_body = send_http_request(api_request.url, api_request.post_data, &status_code);

    return status_code;

    // rc_api_request_destroy(&api_request);
    
}