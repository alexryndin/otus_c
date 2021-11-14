#include <bstrlib/bstrlib.h>
#include <curl/curl.h>
#include "dbg.h"
#include <json-parser/json.h>
#include <stdio.h>

// EXAMPLE
// CURL *curl = curl_easy_init();
// if(curl) {
//  CURLcode res;
//  curl_easy_setopt(curl, CURLOPT_URL, "https://example.com");
//  res = curl_easy_perform(curl);
//  curl_easy_cleanup(curl);
//}

const char *const USAGE = "Usage: %s location\n";

static struct tagbstring _space = bsStatic (" ");
static struct tagbstring _plus = bsStatic ("+");

size_t get_bstr_cb(char *ptr, size_t size, size_t nmemb, bstring str) {
    CHECK(bcatblk(str, ptr, size * nmemb) == BSTR_OK, "Buffer copy failed");
    return size * nmemb;
error:
    return 0;
}

int main(int argc, char *argv[]) {
    int rc = 0;

    CURLcode res;
    CURL *curl = NULL;

    bstring output = bfromcstr("");
    bstring loc_type = bfromcstr("");
    bstring title = bfromcstr("");
    bstring url = NULL;

    json_value *joutput = NULL;
    json_value *consolidated_weather = NULL;

    int woeid = 0;

    if (argc < 2) {
        printf(USAGE, argv[0]);
        goto exit;
    }

    CHECKRC(curl_global_init(CURL_GLOBAL_ALL) == CURLE_OK, 1,
            "curl_global_init failed");
    curl = curl_easy_init();
    CHECKRC(curl != NULL, 1, "curl_easy_init failed");

    url = bformat("https://www.metaweather.com/api/location/search/?query=%s",
                  argv[1]);

    bfindreplace(url, &_space, &_plus, 0);

    CHECKRC(curl_easy_setopt(curl, CURLOPT_URL, bdata(url)) == CURLE_OK, 1,
            "curl_easy_setopt failed");
    CHECKRC(curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, get_bstr_cb) ==
                CURLE_OK,
            -1, "curl_easy_setopt failed");
    CHECKRC(curl_easy_setopt(curl, CURLOPT_WRITEDATA, output) == CURLE_OK, 1,
            "curl_easy_setopt failed");

    LOG_DEBUG("Requesting %s...", bdata(url));
    res = curl_easy_perform(curl);

    bdestroy(url);
    url = NULL;

    CHECKRC(res == CURLE_OK, 1, "curl_easy_perform failed, err %s",
            curl_easy_strerror(res));

    joutput = json_parse(bdata(output), blength(output));
    CHECKRC(joutput != NULL, 1, "json_parse failed");
    CHECKRC(joutput->type == json_array, 1, "api error: array expected");
    if (joutput->u.array.length < 1) {
        puts("Not found.");
        goto exit;
    }
    json_value *fr = joutput->u.array.values[0];
    CHECKRC(fr->type == json_object, 1, "api error: object expected");
    CHECKRC(fr->u.object.length == 4, 1,
            "api error: expected loc description of length 4");
    json_object_entry cur;
    for (unsigned int i = 0; i < fr->u.object.length; i++) {
        cur = fr->u.object.values[i];
        LOG_DEBUG("%s: ", cur.name);
        if (!strcmp(cur.name, "woeid")) {
            CHECKRC(cur.value->type == json_integer, 1,
                    "api error: woeid: wrong type, expected integer");
            woeid = cur.value->u.integer;
            continue;
        }
        if (!strcmp(cur.name, "title")) {
            CHECKRC(cur.value->type == json_string, 1,
                    "api error: title: wrong type, expected string");
            bcatcstr(title, cur.value->u.string.ptr);
            continue;
        }
        if (!strcmp(cur.name, "location_type")) {
            CHECKRC(cur.value->type == json_string, 1,
                    "api error: location_type: wrong type, expected string");
            bcatcstr(loc_type, cur.value->u.string.ptr);
            continue;
        }
    }
    CHECKRC(woeid != 0, 1, "api error: woeid not found");
    CHECKRC(blength(loc_type) > 0, 1, "api error: empty location type");
    CHECKRC(blength(title) > 0, 1, "api error: empty title");

    printf("I hope you was looking for the %s of %s, woeid %d\n",
           bdata(loc_type), bdata(title), woeid);

    json_value_free(joutput);
    joutput = NULL;
    bdestroy(output);

    output = bfromcstr("");
    CHECKRC(curl_easy_setopt(curl, CURLOPT_WRITEDATA, output) == CURLE_OK, 1,
            "curl_easy_setopt failed");

    url = bformat("https://www.metaweather.com/api/location/%d/", woeid);
    res = curl_easy_setopt(curl, CURLOPT_URL, bdata(url));
    CHECKRC(res == CURLE_OK, 1, "curl_easy_setopt failed, err %s",
            curl_easy_strerror(res));

    LOG_DEBUG("Requesting url %s", bdata(url));
    res = curl_easy_perform(curl);
    CHECKRC(res == CURLE_OK, 1, "curl_easy_perform failed, err %s",
            curl_easy_strerror(res));
    joutput = json_parse(bdata(output), blength(output));
    CHECKRC(joutput != NULL, 1, "json_parse failed");
    CHECKRC(joutput->type == json_object, 1, "api error: map expected");
    CHECKRC(joutput->u.object.length > 0, 1, "api error: got empty answer");

    for (unsigned int i = 0; i < joutput->u.object.length; i++) {
        cur = joutput->u.object.values[i];
        LOG_DEBUG("name %s", cur.name);
        if (!strcmp(cur.name, "consolidated_weather")) {
            consolidated_weather = cur.value;
            break;
        }
    }
    CHECKRC(consolidated_weather != NULL, 1, "api error: weather not found");
    CHECKRC(consolidated_weather->type == json_array, 1,
            "api error: got wrong json, expected array");
    CHECKRC(consolidated_weather->u.array.length > 0, 1,
            "api error: weather not found");
    fr = consolidated_weather->u.array.values[0];
    for (unsigned int i = 0; i < fr->u.object.length; i++) {
        cur = fr->u.object.values[i];
        if (!strcmp(cur.name, "weather_state_name")) {
            CHECKRC(cur.value->type == json_string, 1, "api error: wrong data type: expected stirng");
            printf("Weather:\t%s\n", cur.value->u.string.ptr);
        } else if (!strcmp(cur.name, "wind_direction_compass")) {
            CHECKRC(cur.value->type == json_string, 1, "api error: wrong data type: expected stirng");
            printf("Wind compass:\t%s\n", cur.value->u.string.ptr);
        } else if (!strcmp(cur.name, "min_temp")) {
            CHECKRC(cur.value->type == json_double, 1, "api error: wrong data type: expected double");
            printf("Min temp:\t%0.2f°C\n", cur.value->u.dbl);
        } else if (!strcmp(cur.name, "max_temp")) {
            CHECKRC(cur.value->type == json_double, 1, "api error: wrong data type: expected double");
            printf("Max temp:\t%0.2f°C\n", cur.value->u.dbl);
        } else if (!strcmp(cur.name, "the_temp")) {
            CHECKRC(cur.value->type == json_double, 1, "api error: wrong data type: expected double");
            printf("Temp:\t%0.2f°C\n", cur.value->u.dbl);
        } else if (!strcmp(cur.name, "wind_speed")) {
            CHECKRC(cur.value->type == json_double, 1, "api error: wrong data type: expected double");
            printf("Wind speed:\t%f km/h\n", cur.value->u.dbl*1.1609344);
        } else if (!strcmp(cur.name, "wind_direction")) {
            CHECKRC(cur.value->type == json_double, 1, "api error: wrong data type: expected double");
            printf("Wind direction:\t%0.f°\n", cur.value->u.dbl);
        } else if (!strcmp(cur.name, "air_pressure")) {
            CHECKRC(cur.value->type == json_double, 1, "api error: wrong data type: expected double");
            printf("Air pressure:\t%0.f mmhq\n", cur.value->u.dbl*0.750062);
        } else if (!strcmp(cur.name, "visibility")) {
            CHECKRC(cur.value->type == json_double, 1, "api error: wrong data type: expected double");
            printf("Visibility:\t%0.f km\n", cur.value->u.dbl*1.1609344);
        } else if (!strcmp(cur.name, "predictability")) {
            CHECKRC(cur.value->type == json_integer, 1, "api error: wrong data type: expected integer");
            printf("Predictability:\t%ld%%\n", cur.value->u.integer);
        } else if (!strcmp(cur.name, "humidity")) {
            CHECKRC(cur.value->type == json_integer, 1, "api error: wrong data type: expected integer");
            printf("Humidity:\t%ld%%\n", cur.value->u.integer);
        } else {
            LOG_DEBUG("Skipping %s", cur.name);
        }
    }

    // fallthrough
exit:
error:
    if (output != NULL)
        bdestroy(output);
    if (curl != NULL)
        curl_easy_cleanup(curl);
    if (joutput != NULL)
        json_value_free(joutput);
    if (url != NULL)
        bdestroy(url);
    if (title != NULL)
        bdestroy(title);
    if (loc_type != NULL)
        bdestroy(loc_type);
    return rc;
}
