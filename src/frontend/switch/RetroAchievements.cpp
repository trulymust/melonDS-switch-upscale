#include <fstream>
#include <string>
#include <switch.h>
#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <cstring>
#include <cstdlib>
#include "stb_image/stb_image.h"
#include "ROMMetaDatabase.h"
#include "NotificationSystem.h"

#define STB_IMAGE_IMPLEMENTATION

extern "C" {
    #include <rc_api_user.h>
    #include <rc_api_runtime.h>
    #include <rc_client.h>
    #include <rc_consoles.h>
}

    
static std::string g_response;
rc_client_t* g_client = NULL;
static bool g_login_successful = false;


/* -------------- SERVER COMUNICATION --------------*/

static size_t switch_curl_write_callback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t realsize = size * nmemb;
    g_response.append((char*)contents, realsize);
    return realsize;
}

static size_t switch_curl_write_image_callback(void* contents, size_t size, size_t nmemb, void* userp) {
  size_t realsize = size * nmemb;
  std::vector<unsigned char>* image_data = static_cast<std::vector<unsigned char>*>(userp);
  image_data->insert(image_data->end(), (unsigned char*)contents, (unsigned char*)contents + realsize);
  return realsize;
}


// This for download icons from rcheevos APIs
int DownloadAndPackAvatar(const char* url,  int* outWidth, int* outHeight) {
  CURL* curl = curl_easy_init();
  if (!curl)
      return -1;

  printf("DEBUG: inside downloadandpackavatar\n");
  fflush(stdout);
  
  std::vector<unsigned char> image_data;

  printf("DEBUG: image_data initialized\n");
  fflush(stdout);

  curl_easy_setopt(curl, CURLOPT_URL, url);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, switch_curl_write_image_callback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &image_data);
  curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L); 
  curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

  printf("DEBUG: curl settings: %s\n", url);
  fflush(stdout);

  CURLcode res = curl_easy_perform(curl);
  curl_easy_cleanup(curl);
  printf("DEBUG: curl for image result: %d\n", res);
  fflush(stdout);

  if (res != CURLE_OK || image_data.empty()) {
    printf("DEBUG: curl not OK or image data is empty. Returning null.");
    fflush(stdout);
    return -1;
  }

  int width, height, channels;
  unsigned char* pixels = stbi_load_from_memory(image_data.data(), image_data.size(), &width, &height, &channels, 4);
  if (!pixels)
      return -1;

  *outWidth = width;
  *outHeight = height;
    
  printf("DEBUG: pixels loaded.\n");
  fflush(stdout);

  int textureId = Gfx::TextureCreate(width, height, DkImageFormat_RGBA8_Unorm);
  Gfx::TextureUpload(textureId, 0, 0, width, height, pixels, width * 4);
  stbi_image_free(pixels);
  return textureId;
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

  const rc_client_user_t* user = rc_client_get_user_info(client);
  int avatarTexture = -1;
  int texWidth = 0, texHeight = 0;

  printf("DEBUG: rc_client obtained. Downloading avatar..\n");
  fflush(stdout);
  if (user->avatar_url) {
    printf("DEBUG: avatar obtained: %s\n", user->avatar_url);
    fflush(stdout);

    avatarTexture = DownloadAndPackAvatar(user->avatar_url,  &texWidth, &texHeight);
    printf("DEBUG: Showing ShowWithIcon: %d\n", avatarTexture);
    fflush(stdout);
    g_notification.ShowWithIcon(avatarTexture, texWidth, texHeight, "Welcome %s (%u points)", user->display_name, user->score);
    g_notification.Render();
    return;
  }
  // TODO store_retroachievements_credentials(user->username, user->token);
  g_notification.Show("Welcome %s (%u points)", user->display_name, user->score);
  g_notification.Render();
}

void login_retroachievements_user(const char* username, const char* password)
{
  // This will generate an HTTP payload and call the server_call chain above.
  // Eventually, login_callback will be called to let us know if the login was successful.
  rc_client_begin_login_with_password(g_client, username, password, login_callback, NULL);
}

/* -------------- STARTING GAME SESSION --------------*/

static void show_game_placard(void)
{
  char message[128], url[128];
  // async_image_data* image_data = NULL;
  const rc_client_game_t* game = rc_client_get_game_info(g_client);
  rc_client_user_game_summary_t summary;
  rc_client_get_user_game_summary(g_client, &summary);

  // Construct a message indicating the number of achievements unlocked by the user.
  if (summary.num_core_achievements == 0)
  {
    printf("This game has no achievements.\n");
    fflush(stdout);
  }
  else if (summary.num_unsupported_achievements)
  {
    printf("You have %u of %u achievements unlocked (%d unsupported).\n",
        summary.num_unlocked_achievements, summary.num_core_achievements,
        summary.num_unsupported_achievements);
    fflush(stdout);
  }
  else
  {
    printf("You have %u of %u achievements unlocked.\n", summary.num_unlocked_achievements, summary.num_core_achievements);
    fflush(stdout);
  }

  // The emulator is responsible for managing images. This uses rc_client to build
  // the URL where the image should be downloaded from.
  /*      TO DO      
  if (rc_client_game_get_image_url(game, url, sizeof(url)) == RC_OK)
  {
    // Generate a local filename to store the downloaded image.
    char game_badge[64];
    snprintf(game_badge, sizeof(game_badge), "game_%s.png", game->badge_name);

    // This function will download and cache the game image. It is up to the emulator
    // to implement this logic. Similarly, the emulator has to use image_data to
    // display the game badge in the placard, or a placeholder until the image is
    // downloaded. None of that logic is provided in this example.
    image_data = download_and_cache_image(game_badge, url);
  }
  */

  // show_popup_message(image_data, game->title, message);
}

static void load_game_callback(int result, const char* error_message, rc_client_t* client, void* userdata)
{
  printf("RetroAchievements game loaded, result: %d\n", result);
  fflush(stdout);
  if (result != RC_OK)
  {
    printf("RetroAchievements game load failed: %s\nResult: %d", error_message, result);
    fflush(stdout);
    return;
  }

  // announce that the game is ready. we'll cover this in the next section.
  show_game_placard();
}

void load_game_from_file(const char* path)
{
  printf("RetroAchievements game loading: %s\n", path);
  fflush(stdout);
  rc_client_begin_identify_and_load_game(g_client, RC_CONSOLE_NINTENDO_DS, 
      path, NULL, 0, load_game_callback, NULL);
}


/* -------------- INITIALIZATION --------------*/

int InitRetroAchievements(const char* username, const char* password) {
  initialize_retroachievements_client();

  login_retroachievements_user(username, password);

  return g_login_successful ? 200 : 401;

}