#ifndef _BABY_API_H_
#define _BABY_API_H_

#include <Arduino.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <cmath>
#include <cstring>
#include <ezTime.h>

#define DEFAULT_SERVER_HOST "192.168.1.1"
#define DEFAULT_SERVER_PORT "8000"
#define ENDPOINT "/api/"

#define BMI_ENDPOINT "bmi/"
#define CHANGES_ENDPOINT "changes/"
#define CHILDREN_ENDPOINT "children/"
#define FEEDINGS_ENDPOINT "feedings/"
#define HEAD_CIRCUMFERENCE_ENDPOINT "head-circumference/"
#define HEIGHT_ENDPOINT "height/"
#define NOTES_ENDPOINT "notes/"
#define PUMPING_ENDPOINT "pumping/"
#define SLEEP_ENDPOINT "sleep/"
#define TAGS_ENDPOINT "tags/"
#define TEMPERATURE_ENDPOINT "temperature/"
#define TIMERS_ENDPOINT "timers/"
#define TUMMY_TIMES_ENDPOINT "tummy-times/"
#define WEIGHT_ENDPOINT "weight/"
#define PROFILE_ENDPOINT "profile"

#define BB_DATE_FORMAT "DATE FORMAT HERE"

#define JOSN_CAPACITY 1024 // close to wose case scenario. shold be sufficent to retrive a least 5 records wiht a search and everythign else should be smaller

#define MAX_TAGS 10

#define SEARCH_LIMIT 5

class BabyApi
{

public:
    BabyApi(const char * baby_api_key);

    BabyApi(const char * server_host, const char * baby_api_key);

    BabyApi(const char * server_host, const char * server_port, const char * baby_api_key);

    // SCHEMA
    const char *BabyApi::stoolColours[5] =
    {
        {},
        "black",
        "brown",
        "green",
        "yellow"
    };



    struct BMI
    {
        uint16_t id;
        uint16_t child;
        float bmi;
        tmElements_t date;
        const char * notes;
        const char * tags[MAX_TAGS];
    };

    struct DiaperChange
    {
        uint16_t id;
        uint16_t child;
        tmElements_t time;
        bool wet;
        bool solid;
        const char * color;
        float amount;
        const char * notes;
        const char * tags[MAX_TAGS];
    };

    struct Child
    {
        uint16_t id;
        const char * first_name;
        const char * last_name;
        tmElements_t birth_date;
        const char * slug;
        const char * picture;
    };

    const char *feedingTypes[5] =
        {
            {},
            "Breast Milk",
            "Formula",
            "Fortified Breast Milk",
            "Solid Food"};

    const char *feedingMethods[7] =
        {
            {},
            "Bottle",
            "Left Breast",
            "Right Breast",
            "Both Breasts",
            "Parent Fed",
            "Self Fed"};

    struct Feeding
    {
        uint16_t id;
        uint16_t child;    // Required unless a Timer value is provided.
        tmElements_t start; // Required unless a Timer value is provided.
        tmElements_t end;   // Required unless a Timer value is provided.
        uint16_t timer;    // May be used in place of the Start, End, and/or Child values.
        tmElements_t duration;
        const char * type;
        const char * method;
        float amount;
        const char * notes;
        const char * tags[MAX_TAGS];
    };

    struct HeadCircumference
    {
        uint16_t id;
        uint16_t child;
        float head_circumference;
        tmElements_t date;
        const char * notes;
        const char * tags[MAX_TAGS];
    };

    struct Height
    {
        uint16_t id;
        uint16_t child;
        float height;
        tmElements_t date;
        const char * notes;
        const char * tags[MAX_TAGS];
    };

    struct Note
    {
        uint16_t id;
        uint16_t child;
        tmElements_t date;
        const char * note;
        const char * tags[MAX_TAGS];
    };

    struct Pumping
    {
        uint16_t id;
        uint16_t child;
        float amount;
        tmElements_t time;
        const char * notes;
        const char * tags[MAX_TAGS];
    };

    struct Sleep
    {
        uint16_t id;
        uint16_t child;    // Required unless a Timer value is provided.
        tmElements_t start; // Required unless a Timer value is provided.
        tmElements_t end;   // Required unless a Timer value is provided.
        uint16_t timer;    // May be used in place of the Start, End, and/or Child values.
        tmElements_t duration;
        bool nap;
        const char * notes;
        const char * tags[MAX_TAGS];
    };

    struct Tag
    {
        const char * slug;
        const char * name;
        const char * color;
        const char * last_used;
    };

    struct Temperature
    {
        uint16_t id;
        uint16_t child;
        float temperature;
        tmElements_t time;
        const char * notes;
        const char * tags[MAX_TAGS];
    };

    struct Timer
    {
        uint16_t id; // read only
        uint16_t child;
        const char * name;
        tmElements_t start;
        tmElements_t end;      // read only
        tmElements_t duration; // read only
        bool active;     // read only
        uint16_t user;
    };

    struct TummyTime
    {
        uint16_t id;
        uint16_t child;    // Required unless a Timer value is provided.
        tmElements_t start; // Required unless a Timer value is provided.
        tmElements_t end;   // Required unless a Timer value is provided.
        uint16_t timer;    // May be used in place of the Start, End, and/or Child values.
        tmElements_t duration;
        const char * milestone;
        const char * tags[MAX_TAGS];
    };

    struct Weight
    {
        uint16_t id;
        uint16_t child;
        float weight;
        tmElements_t date;
        const char * notes;
        const char * tags[MAX_TAGS];
    };

    struct User
    {
        uint16_t id;
        const char * username;
        const char * first_name;
        const char * last_name;
        const char * email;
        bool is_staff;
    };

    struct Profile
    {
        User user;
        const char * language;
        const char * timezone;
        const char * api_key;
    };

    template <typename T>
    struct searchResults
    {
        long count;
        long next;
        long previous;
        T results[SEARCH_LIMIT];
    };

    // functions
    int httpRequest(
        const char *endpoint,
        const char *type,
        const char *parameters = {},
        const char *query = {},
        bool includeRequest = false);

    searchResults<BMI> findBMIRecords(
        uint16_t offset = 0,
        uint16_t child = 0,
        time_t date = {},
        const char * ordering = {});

    BMI getBMI(uint16_t id);

    BMI logBMI(
        uint16_t child,
        float bmi,
        time_t date,
        const char * notes = {},
        const char * tags[MAX_TAGS] = {});

    BMI updateBMI(
        uint16_t id,
        uint16_t child = 0,
        float bmi = NAN,
        time_t date = {},
        bool updateNotes = false,
        const char * notes = {},
        bool updateTags = false,
        const char * tags[MAX_TAGS] = {});

    bool deleteBMI(uint16_t id);

    searchResults<DiaperChange> findDiaperChanges(
        uint16_t offset = 0,
        uint16_t child = 0,
        const char * colour = {},
        time_t date = {},
        time_t date_max = {},
        time_t date_min = {},
        bool solid = {},
        bool wet = {},
        const char * tags[MAX_TAGS] = {},
        const char * ordering = {});

    BabyApi::DiaperChange BabyApi::logDiaperChange(
        uint16_t child,
        bool wet,
        bool solid,
        const char * color = {},
        float amount = NAN,
        const char * notes = {},
        const char * tags[MAX_TAGS] = {});

    DiaperChange logDiaperChange(
        uint16_t child,
        time_t time,
        bool wet = false,
        bool solid = false,
        const char * color = {},
        float amount = NAN,
        const char * notes = {},
        const char * tags[MAX_TAGS] = {});

    DiaperChange getDiaperChange(uint16_t id);

    DiaperChange updateDiaperChange(
        uint16_t id,
        uint16_t child = 0,
        time_t time = {},
        bool wet = {},
        bool solid = {},
        const char * color = {},
        float amount = NAN,
        bool updateNotes = false,
        const char * notes = {},
        bool updateTags = false,
        const char * tags[MAX_TAGS] = {});

    bool removeDiaperChange(uint16_t id);

    searchResults<Child> findChildren(
        uint16_t offset = 0,
        const char * first_name = {},
        const char * last_name = {},
        time_t birth_date = {},
        const char * slug = {},
        const char * ordering = {});

    Child newChild(
        const char * first_name,
        const char * last_name,
        time_t birth_date,
        const char * picture);

    Child getChild(const char * slug);

    Child updateChild(
        const char * slug,
        const char * first_name = {},
        const char * last_name = {},
        time_t birth_date = {},
        bool updatePicture = false,
        const char * picture = {});

    bool removeChild(const char * slug);

    searchResults<Feeding> findFeedingRecords(
        uint16_t offset = 0,
        uint16_t child = 0,
        time_t start = {},
        time_t start_max = {},
        time_t start_min = {},
        time_t end = {},
        time_t end_max = {},
        time_t end_min = {},
        const char * type = {},
        const char * method = {},
        const char * tags[MAX_TAGS] = {},
        const char * ordering = {});

    Feeding logFeeding(
        uint16_t child,    // Required unless a Timer value is provided.
        time_t start, // Required unless a Timer value is provided.
        time_t end,   // Required unless a Timer value is provided.
        const char * type,
        const char * method,
        float amount = NAN,
        const char * notes = {},
        const char * tags[MAX_TAGS] = {});

    Feeding logFeeding(
        uint16_t timer,
        const char * type,
        const char * method,
        float amount = NAN,
        const char * notes = {},
        const char * tags[MAX_TAGS] = {});

    Feeding logFeeding(
        uint16_t child = 0,          // Required unless a Timer value is provided.
        time_t start = {}, // Required unless a Timer value is provided.
        time_t end = {},   // Required unless a Timer value is provided.
        const char * type = {},
        const char * method = {},
        float amount = NAN,
        const char * notes = {},
        const char * tags[MAX_TAGS] = {});

    Feeding logFeeding(
        uint16_t child = 0,          // Required unless a Timer value is provided.
        time_t start = {}, // Required unless a Timer value is provided.
        time_t end = {},   // Required unless a Timer value is provided.
        uint16_t timer = 0,          // May be used in place of the Start, End, and/or Child values.
        const char * type = {},
        const char * method = {},
        float amount = NAN,
        const char * notes = {},
        const char * tags[MAX_TAGS] = {});

    Feeding getFeeding(uint16_t id);

    Feeding updateFeeding(
        uint16_t id = 0,
        uint16_t child = 0,          // Required unless a Timer value is provided.
        time_t start = {}, // Required unless a Timer value is provided.
        time_t end = {},   // Required unless a Timer value is provided.
        const char * method = {},
        const char * type = {},
        float amount = NAN,
        bool updateNotes = false,
        const char * notes = {},
        bool updateTags = false,
        const char * tags[MAX_TAGS] = {});

    bool removeFeeding(uint16_t id);

    searchResults<HeadCircumference> findHeadCircumferenceRecords(
        uint16_t offset = 0,
        uint16_t child = 0,
        time_t date = {},
        const char * ordering = {});

    HeadCircumference logHeadCircumference(
        uint16_t child,
        float head_circumference,
        time_t date,
        const char * notes = {},
        const char * tags[MAX_TAGS] = {});

    HeadCircumference getHeadCircumference(uint16_t id);

    HeadCircumference updateHeadCircumference(
        uint16_t id,
        uint16_t child = 0,
        float head_circumference = NAN,
        time_t date = {},
        bool updateNotes = false,
        const char * notes = {},
        bool updateTags = false,
        const char * tags[MAX_TAGS] = {});

    bool removeHeadCircumference(uint16_t id);

    searchResults<Height> findHeightRecords(
        uint16_t offset = 0,
        uint16_t child = 0,
        time_t date = {},
        const char * ordering = {});

    Height logHeight(
        uint16_t child,
        float height,
        time_t date,
        const char * notes = {},
        const char * tags[MAX_TAGS] = {});

    Height getHeight(uint16_t id);

    Height updateHeight(
        uint16_t id,
        uint16_t child = 0,
        float height = NAN,
        time_t date = {},
        bool updateNotes = false,
        const char * notes = {},
        bool updateTags = false,
        const char * tags[MAX_TAGS] = {});

    bool removeHeight(uint16_t id);

    searchResults<Note> findNotes(
        uint16_t offset = 0,
        uint16_t child = 0,
        time_t date = {},
        time_t date_max = {},
        time_t date_min = {},
        const char * tags[MAX_TAGS] = {},
        const char * ordering = {});

    Note createNote(
        uint16_t child,
        const char * note,
        time_t date,
        const char * tags[MAX_TAGS] = {});

    Note getNote(uint16_t id);

    Note updateNote(
        uint16_t id,
        uint16_t child = 0,
        time_t date = {},
        bool updateNote = false,
        const char * note = {},
        bool updateTags = false,
        const char * tags[MAX_TAGS] = {});

    bool removeNote(uint16_t id);

    searchResults<Pumping> findPumpingRecords(
        uint16_t offset = 0,
        uint16_t child = 0,
        time_t date = {},
        time_t date_max = {},
        time_t date_min = {},
        const char * ordering = {});

    Pumping logPumping(
        uint16_t child,
        float amount,
        time_t time = {},
        const char * notes = {},
        const char * tags[MAX_TAGS] = {});

    Pumping getPumping(uint16_t id);

    Pumping updatePumping(
        uint16_t id,
        uint16_t child = 0,
        float amount = NAN,
        time_t time = {},
        bool updateNotes = false,
        const char * notes = {},
        bool updateTags = false,
        const char * tags[MAX_TAGS] = {});

    bool removePumping(uint16_t id);

    searchResults<Sleep> findSleepRecords(
        uint16_t offset = 0,
        uint16_t child = 0,
        time_t start = {},
        time_t start_max = {},
        time_t start_min = {},
        time_t end = {},
        time_t end_max = {},
        time_t end_min = {},
        const char * tags[MAX_TAGS] = {},
        const char * ordering = {});

    Sleep logSleep(
        uint16_t child = 0,          // Required unless a Timer value is provided.
        time_t start = {}, // Required unless a Timer value is provided.
        time_t end = {},   // Required unless a Timer value is provided.
        uint16_t timer = 0,          // May be used in place of the Start, End, and/or Child values.
        const char * notes = {},
        const char * tags[MAX_TAGS] = {});

    Sleep getSleep(uint16_t id);

    Sleep updateSleep(
        uint16_t id,
        uint16_t child = 0,
        time_t start = {},
        time_t end = {},
        bool updateNotes = false,
        const char * notes = {},
        bool updateTags = false,
        const char * tags[MAX_TAGS] = {});

    bool removeSleep(uint16_t id);

    searchResults<Tag> findAllTags(
        uint16_t offset = 0,
        const char * name = {},
        time_t last_used = {},
        const char * ordering = {});

    Tag createTag(
        const char * name,
        const char * colour = {});

    Tag getTag(
        const char * slug);

    Tag updateTag(
        const char * slug,
        bool updateName = false,
        const char * name = {},
        bool updateColour = false,
        const char * colour = {});

    bool removeTag(const char * slug);

    searchResults<Temperature> findTemperatureRecords(
        uint16_t offset = 0,
        uint16_t child = 0,
        time_t date = {},
        time_t date_max = {},
        time_t date_min = {},
        const char * tags[MAX_TAGS] = {},
        const char * ordering = {});

    Temperature logTemperature(
        uint16_t child,
        float temperature,
        time_t time,
        const char * notes = {},
        const char * tags[MAX_TAGS] = {});

    Temperature getTemperature(uint16_t id);

    Temperature updateTemperature(
        uint16_t id = 0,
        uint16_t child = 0,
        float temperature = NAN,
        time_t time = {},
        bool updateNotes = false,
        const char * notes = {},
        bool updateTags = false,
        const char * tags[MAX_TAGS] = {});

    bool removeTemperature(uint16_t id);

    enum TimerState
    {
      UNKNOWN,
      INACTIVE,
      ACTIVE
    };

    searchResults<Timer> findTimers(
        uint16_t offset = 0,
        uint16_t child = 0,
        time_t start = {},
        time_t start_max = {},
        time_t start_min = {},
        time_t end = {},
        time_t end_max = {},
        time_t end_min = {},
        TimerState active = {},
        uint16_t user = 0,
        const char * ordering = {});

    Timer createTimer(
        uint16_t child);

    Timer createTimer(
        uint16_t child,
        const char * name);

    Timer createTimer(
        uint16_t child,
        time_t start);

    Timer createTimer(
        uint16_t child,
        const char * name,
        time_t start);

    Timer getTimer(uint16_t id);

    Timer updateTimer(
        uint16_t id,
        uint16_t child = 0,
        const char * name = {},
        time_t start = {},
        uint16_t user = 0);

    bool removeTimer(uint16_t id);

    Timer restartTimer(uint16_t id);

    Timer stopTimer(uint16_t id);

    searchResults<TummyTime> findTummyTimes(
        uint16_t offset = 0,
        uint16_t child = 0,
        time_t start = {},
        time_t start_max = {},
        time_t start_min = {},
        time_t end = {},
        time_t end_max = {},
        time_t end_min = {},
        const char * tags[MAX_TAGS] = {},
        const char * ordering = {});

    TummyTime logTummyTime(
        uint16_t child,    // Required unless a Timer value is provided.
        time_t start, // Required unless a Timer value is provided.
        time_t end,   // Required unless a Timer value is provided.
        const char * milestone = {},
        const char * tags[MAX_TAGS] = {});

    TummyTime logTummyTime(

        uint16_t timer, // May be used in place of the Start, End, and/or Child values.
         char * milestone = {},
         char * tags[MAX_TAGS] = {});

    TummyTime logTummyTime(
        uint16_t child = 0,          // Required unless a Timer value is provided.
        time_t start = {}, // Required unless a Timer value is provided.
        time_t end = {},   // Required unless a Timer value is provided.
        uint16_t timer = 0,          // May be used in place of the Start, End, and/or Child values.
        const char * milestone = {},
        const char * tags[MAX_TAGS] = {});

    TummyTime getTummyTime(uint16_t id);

    TummyTime updateTummyTime(
        uint16_t id,
        uint16_t child = 0,
        time_t start = {},
        time_t end = {},
        bool updateMilestone = false,
        const char * milestone = {},
        bool updateTags = false,
        const char * tags[MAX_TAGS] = {});

    bool removeTummyTime(uint16_t id);

    searchResults<Weight> findWeightRecords(
        uint16_t offset = 0,
        uint16_t child = 0,
        time_t date = {},
        const char * ordering = {});

    Weight logWeight(
        uint16_t child,
        float weight,
        time_t date,
        const char * notes = {},
        const char * tags[MAX_TAGS] = {});

    Weight getWeight(uint16_t id);

    Weight updateWeight(
        uint16_t id,
        uint16_t child = 0,
        float weight = NAN,
        time_t date = {},
        bool updateNotes = false,
        const char * notes = {},
        bool updateTags = false,
        const char * tags[MAX_TAGS] = {});

    bool removeWeight(uint16_t id);

    Profile getProfile();

    const char * getServerHost();
    const char * getServerPort();
    const char * getApiKey();

    void setServerHost(const char * server_host);
    void setServerPort(const char * server_port);
    void setApiKey(const char * apiKey);

    uint16_t startTimer(uint16_t childId, const char * name = {}, uint16_t timer = 0);

    void searchResultParser(long *count, long *next, long *previous);

    uint8_t getAllChildren(Child *children, uint8_t count);

    uint8_t recordFeeding(uint16_t timerId, uint8_t feedingType, uint8_t feedingMethod, float amount);

    uint8_t recordSleep(uint16_t timerId);

    uint8_t recordPumping(uint16_t timerId, float amount);

    uint8_t recordTummyTime(uint16_t timerId);

    uint8_t recordNappyChange(uint16_t child, bool wet, bool solid, uint16_t colour);



protected:
    char serverHost[256];
    char serverPort[8];
    char babyApiKey[129];
    char requestBody[2048];
    char responseBody[2048];
    char parameters[256];
    char query[256];
    StaticJsonDocument<JOSN_CAPACITY> doc;
    StaticJsonDocument<JOSN_CAPACITY> response;

    void ResponseParser(const char * parse);
};

extern BabyApi babyApi;

// #include "BabyApi.cpp"
#endif