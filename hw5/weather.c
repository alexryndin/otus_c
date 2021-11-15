#include "dbg.h"
#include <assert.h>
#include <bstrlib/bstrlib.h>
#include <curl/curl.h>
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

char *json_type_to_cstr(json_type json_type) {
    switch (json_type) {
    case json_none:
        return "json_none";
    case json_object:
        return "json_object";
    case json_array:
        return "json_array";
    case json_integer:
        return "json_integer";
    case json_double:
        return "json_double";
    case json_string:
        return "json_string";
    case json_boolean:
        return "json_boolean";
    case json_null:
        return "json_null";
    default:
        SENTINEL("Unknown json type");
    }
error:
    return NULL;
}

int json_check_print(json_object_entry cur, char *field_name,
                     json_type expected_type, char *msg, double mod) {
    if (!strcmp(cur.name, field_name)) {
        CHECK(cur.value->type == expected_type,
              "api error: wrong data type: expected %s",
              json_type_to_cstr(expected_type));
        switch (expected_type) {
        case json_none:
            printf(msg, "<none>");
            return 1;
        case json_object:
            printf(msg, "<object>");
            return 1;
        case json_array:
            printf(msg, "<array>");
            return 1;
        case json_integer:
            printf(msg, cur.value->u.integer * mod);
            return 1;
        case json_double:
            printf(msg, cur.value->u.dbl * mod);
            return 1;
        case json_string:
            printf(msg, cur.value->u.string.ptr);
            return 1;
        case json_boolean:
            printf(msg, cur.value->u.boolean);
            return 1;
        case json_null:
            printf(msg, "<null>");
            return 1;
        }
    }
error:
    return 0;
}

const char *const USAGE = "Usage: %s location\n";

static struct tagbstring _space = bsStatic(" ");
static struct tagbstring _plus = bsStatic("+");

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
        if (json_check_print(cur, "weather_state_name", json_string, "Weather:\t%s\n", 1)); else
        if (json_check_print(cur, "wind_direction_compass", json_string,"Wind compass:\t%s\n",1)); else
        if (json_check_print(cur, "max_temp", json_double, "Max temp:\t%0.2f°C\n", 1)); else
        if (json_check_print(cur, "min_temp", json_double,"Min temp:\t%0.2f°C\n",1)); else
        if (json_check_print(cur, "the_temp",  json_double, "Temp:\t%0.2f°C\n", 1)); else
        if (json_check_print(cur, "wind_speed", json_double,"Windspeed:\t%0.2f°C\n",1)); else
        if (json_check_print(cur, "wind_direction", json_double,"Wind direction:\t%0.2f°C\n",1)); else
        if (json_check_print(cur, "air_pressure", json_double,"Airpressure:\t%0.fmmhq\n",0.750062)); else
        if (json_check_print(cur, "visibility", json_double,"Visibility:\t%0.fkm\n",1.1609344)); else
        if (json_check_print(cur, "predictability", json_integer,"Predictability:\t%ld%%\n",1)); else
        if (json_check_print(cur, "humidity", json_integer,"Humidity:\t%ld%%\n",1)); else {
            LOG_DEBUG("Skipping %s", cur.name);
        }
        // clang-format on
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
