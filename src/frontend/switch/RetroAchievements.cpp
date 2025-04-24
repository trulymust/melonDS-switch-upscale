#include <fstream>
#include <string>
#include <switch.h>
#include <stdio.h>
#include <cstring>

#include "../../../external/rcheevos/src/rapi/rc_api_user.c"

static std::string ra_username = "Username";
static std::string ra_token = "Token";

void InitRetroAchievements() {
    rc_api_login_request_t api_params;
    memset(&api_params, 0, sizeof(api_params));

    api_params.username = ra_username;
    api_params.api_token = ra_token;
    api_params.password = "Password"

    rc_api_request_t api_request;

    rc_api_init_login_request(&api_request, &api_params);

    printf("RA login: %s\n", ra_username.c_str());

    httpcContext context;
    Result rc = httpcOpenContext(&context, HTTPC_METHOD_POST, "https://retroachievements.org/dorequest.php", 0);
    if (R_FAILED(rc)) {
        printf("httpcOpenContext failed: 0x%x\n", rc);
        return;
    }

    httpcSetKeepAlive(&context, true);
    httpcSetSSLOpt(&context, SSLCOPT_DisableVerify);
    httpcAddRequestHeaderField(&context, "Content-Type", "application/x-www-form-urlencoded");
    httpcSetPostDataRaw(&context, login_req.post_data, strlen(login_req.post_data));

    rc = httpcBeginRequest(&context);
    if (R_FAILED(rc)) {
        printf("httpcBeginRequest failed: 0x%x\n", rc);
        httpcCloseContext(&context);
        return;
    }

    u32 status_code = 0;
    httpcGetResponseStatusCode(&context, &status_code, 0);
    printf("HTTP status: %d\n", status_code);

    char buffer[2048];
    size_t outsize = 0;
    rc = httpcDownloadData(&context, buffer, sizeof(buffer) - 1, &outsize);
    buffer[outsize] = '\0';

    if (R_FAILED(rc)) {
        printf("httpcDownloadData failed: 0x%x\n", rc);
        httpcCloseContext(&context);
        return;
    }

    printf("RA response: %s\n", buffer);

    int ret = rc_api_process_login_response(&login_res, buffer);
    if (ret == RC_OK && login_res.succeeded) {
        printf("RA login successful! User: %s, Score: %d\n", login_res.username, login_res.score);
    } else {
        printf("RA login failed: %s\n", login_res.error_message ? login_res.error_message : "Unknown error");
    }

    httpcCloseContext(&context);
}
