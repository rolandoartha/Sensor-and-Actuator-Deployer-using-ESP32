#ifndef AUTH_HPP
#define AUTH_HPP

#include <ESPAsyncWebServer.h>
#include <definitions.h>
#include <FS.h>
#include <Hash.h>
const uint8_t SESSION_TOKEN_LEN = 5;
const char PORTAL_SESSION_TOKEN_CHARS[] PROGMEM = "abcdefghijklmnopqrstuvwxyz0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ\0";

String _token;

void handleLoginPage(AsyncWebServerRequest *request);
String generateToken(int len);
bool isAuthentified(AsyncWebServerRequest *request);
bool clearAuthentications();
bool handleLoginAuth(AsyncWebServerRequest *request);
void handleLogout(AsyncWebServerRequest *request);

bool clearAuthentications()
{
    _token.clear();
    //   lastLoginMs = 0;
}

bool isAuthentified(AsyncWebServerRequest *request)
{
    return true;
    // DEBUG(WM_CTX,F("Enter isAuthentified"));
    if (request->hasHeader("Cookie") && _token.length() > 0)
    {
        // DEBUG(WM_CTX,F("Found cookie: "));
        String cookie = request->header("Cookie");
        // DEBUG(WM_CTX,cookie);
        int tokenIndex = cookie.indexOf("t=");
        if (tokenIndex >= 0)
        {
            String token = cookie.substring(tokenIndex + 2, tokenIndex + SESSION_TOKEN_LEN + 2);
            if (!token.isEmpty() && _token.indexOf(WEB_PAGE_INDEX + token) >= 0)
            {
                // DEBUG(WM_CTX, F("Auth Successful"));
#ifdef SINGLE_USER_LOGIN_WITH_EXPIRY
                if (isSingleUserLoggedIn())
                {
                    lastLoginMs = millis();
                    return true;
                }
                else
                {
                    lastLoginMs = 0;
                    return false;
                }
#else
                return true;
#endif
            }
        }
    }
    return false;
}

String generateToken(int len)
{
    String letters = FPSTR(PORTAL_SESSION_TOKEN_CHARS);
    int lettersLen = letters.length();
    String token = "";
    for (int i = 0; i < len; i++)
    {
        token += letters.charAt(random(0, lettersLen - 1));
    }
    return token;
}

void handleLoginPage(AsyncWebServerRequest *request)
{
    String msg = "";
    if (request->hasHeader("Cookie"))
    {
        String cookie = request->header("Cookie");
        if (isAuthentified(request))
        {
            AsyncWebServerResponse *response = request->beginResponse(301, CONTENT_TYPE_HTML);
            response->addHeader("Location", WEB_PAGE_INDEX);
            response->addHeader("Cache-Control", "no-cache");
            request->send(response);
            return;
        }
    }

#ifdef SINGLE_USER_LOGIN_WITH_EXPIRY
    if (request->hasArg("u") && request->hasArg("p") && !isSingleUserLoggedIn())
#else
    if (request->hasArg("u") && request->hasArg("p"))
#endif
    {
        if (request->arg("u") == DeviceConfig.PortalUser && request->arg("p") == DeviceConfig.PortalPwd)
        {
#ifdef SINGLE_USER_LOGIN_WITH_EXPIRY
            clearAuthentications();
#endif
            // login ok, redirect
            AsyncWebServerResponse *response = request->beginResponse(301, CONTENT_TYPE_HTML);
            response->addHeader("Location", WEB_PAGE_INDEX);
            response->addHeader("Cache-Control", "no-cache");
            String newToken = generateToken(SESSION_TOKEN_LEN);
            while (_token.indexOf(WEB_PAGE_INDEX + newToken) >= 0)
            {
                newToken = generateToken(SESSION_TOKEN_LEN);
            }
            _token += WEB_PAGE_INDEX + newToken;
            response->addHeader("Set-Cookie", "t=" + newToken);
            request->send(response);
            // lastLoginMs = millis();
            //Matchbox::Inst().PrintSysInfo();
            return;
        }
        else
        {
            // not authorized
            AsyncWebServerResponse *response =
                request->beginResponse(SPIFFS, WEB_FILE_LOGIN, CONTENT_TYPE_HTML, false, [](const String &var) -> String {
                    if (var == "MSG")
                    {
                        return "<p id=\"msg\" style=\"color:red; text-align:center\">Login Failed - Invalid credentials</p>";
                    }
                    if (var == "DNM")
                    {
                        return DeviceConfig.DeviceName;
                    }
                    return String();
                });
            response->addHeader("Set-Cookie", "t=");

            request->send(response);
        }
    }
    else
    {
#ifdef SINGLE_USER_LOGIN_WITH_EXPIRY
        if (isSingleUserLoggedIn())
        {
            String page = readFile(FPSTR(WM_PORTAL_LOGIN_BODY));
            msg = String(WM_PORTAL_LOGIN_FAIL_SINGLE_USED_LOGGED_IN_MSG);
            page.replace("{msg}", msg);

            AsyncWebServerResponse *response = request->beginResponse(200, FPSTR(WM_HTTP_HEAD_CT), page);
            response->addHeader("Set-Cookie", "t=");

            DEBUG(WM_CTX, F("Log in Failed: Other user already logged in"));

            request->send(response);
            return;
        }
#endif
        request->send(SPIFFS, WEB_FILE_LOGIN, CONTENT_TYPE_HTML, false, [](const String &var) -> String {
            if (var == "MSG")
            {
                return "";
            }
            if (var == "DNM")
            {
                return DeviceConfig.DeviceName;
            }
            return String();
        });
    }
}

bool handleLoginAuth(AsyncWebServerRequest *request)
{
    if (!isAuthentified(request))
    {
        request->redirect(WEB_PAGE_LOGIN);
        return false;
    }
    return true;
}

void handleRedirect(AsyncWebServerRequest *request)
{
    if (!isAuthentified(request))
    {
        request->redirect(WEB_PAGE_LOGIN);
        return;
    }
    else
    {
        request->redirect(WEB_PAGE_INDEX);
        return;
    }
}

void handleLogout(AsyncWebServerRequest *request)
{
    AsyncWebServerResponse *response = request->beginResponse(301, CONTENT_TYPE_HTML);
    response->addHeader("Location", WEB_PAGE_LOGIN);
    response->addHeader("Cache-Control", "no-cache");
    response->addHeader("Set-Cookie", "t=");
    request->send(response);
}

#endif