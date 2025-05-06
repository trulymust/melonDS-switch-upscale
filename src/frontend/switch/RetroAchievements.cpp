#include <fstream>
#include <string>
#include <switch.h>
#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <cstring>
#include <cstdlib>
#include "NotificationSystem.h"
extern "C" {
    #include <rc_api_user.h>
    #include <rc_api_runtime.h>
    #include <rc_client.h>
}

    
static std::string g_response;
rc_client_t* g_client = NULL;
static bool g_login_successful = false;

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
    printf("DEBUG: Send HTTP request values: %s\n%s\n", url, post_data);
    fflush(stdout);

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

static uint32_t read_memory(uint32_t address, uint8_t* buffer, uint32_t num_bytes, rc_client_t* client)
{
  // TODO: implement later
  return 0;
}

static void server_call(const rc_api_request_t* request, rc_client_server_callback_t callback, void* callback_data, rc_client_t* client) {
  int status_code;
  const char* response = send_http_request(request->url, request->post_data, &status_code);

  rc_api_server_response_t server_response;
  memset(&server_response, 0, sizeof(server_response));
  server_response.http_status_code = status_code;
  server_response.body = response;
  server_response.body_length = response ? strlen(response) : 0;

  callback(&server_response, callback_data);

  printf("DEBUG: server_call values: %d\n%s\n", server_response.http_status_code, server_response.body);
  fflush(stdout);

  if (response) {
  free((void*)response);
  }
}

void initialize_retroachievements_client(void)
{
  g_client = rc_client_create(read_memory, server_call);

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

static void login_callback(int result, const char* error_message, rc_client_t* client, void* userdata)
{
  printf("DEBUG: login results: %d\n%s\n", result, error_message);
  fflush(stdout);
  if (result != RC_OK)
  {
    printf("DEBUG: login failed. Status: %d\n%s\n", result, error_message);
    fflush(stdout);
    return;
  }

  // Login was successful. Capture the token for future logins so we don't have to store the password anywhere.
  const rc_client_user_t* user = rc_client_get_user_info(client);
  // TODO store_retroachievements_credentials(user->username, user->token);

  // TODO Inform user of successful login
  printf("DEBUG: Successful login. \nLogged in as %s (%u points)\n", user->display_name, user->score);
  fflush(stdout);
  g_notification.Render();
}

void login_retroachievements_user(const char* username, const char* password)
{
  // This will generate an HTTP payload and call the server_call chain above.
  // Eventually, login_callback will be called to let us know if the login was successful.
  rc_client_begin_login_with_password(g_client, username, password, login_callback, NULL);
}

int InitRetroAchievements(const char* username, const char* password) {
  initialize_retroachievements_client();

  login_retroachievements_user(username, password);

  return g_login_successful ? 200 : 401;

}