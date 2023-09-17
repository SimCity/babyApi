#ifndef _BABY_API_H_
#define _BABY_API_H_

#include <Arduino.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <cmath>
#include <ESPDateTime.h>

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

#define JOSN_CAPACITY 1024 // close to wose case scenario. shold be sufficent to retrive a least 5 records wiht a search and everythign else should be smaller

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
        char date[33];
        char notes[256];
        char tags[256];
    };

    struct DiaperChange
    {
        uint16_t id;
        uint16_t child;
        char time[27];
        bool wet;
        bool solid;
        char color[8];
        float amount;
        char notes[256];
        char tags[256];
    };

    struct Child
    {
        uint16_t id;
        char first_name[256];
        char last_name[256];
        char birth_date[11];
        char slug[101];
        char picture[];
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
        char start[33]; // Required unless a Timer value is provided.
        char end[33];   // Required unless a Timer value is provided.
        uint16_t timer;    // May be used in place of the Start, End, and/or Child values.
        char duration[256];
        char type[22];
        char method[13];
        float amount;
        char notes[256];
        char tags[256];
    };

    struct HeadCircumference
    {
        uint16_t id;
        uint16_t child;
        float head_circumference;
        char date[33];
        char notes[256];
        char tags[256];
    };

    struct Height
    {
        uint16_t id;
        uint16_t child;
        float height;
        char date[33];
        char notes[256];
        char tags[256];
    };

    struct Note
    {
        uint16_t id;
        uint16_t child;
        char note[256];
        char date[33];
        char tags[256];
    };

    struct Pumping
    {
        uint16_t id;
        uint16_t child;
        float amount;
        char time[33];
        char notes[256];
        char tags[256];
    };

    struct Sleep
    {
        uint16_t id;
        uint16_t child;    // Required unless a Timer value is provided.
        char start[33]; // Required unless a Timer value is provided.
        char end[33];   // Required unless a Timer value is provided.
        uint16_t timer;    // May be used in place of the Start, End, and/or Child values.
        char duration[17];
        bool nap;
        char notes[256];
        char tags[256];
    };

    struct Tag
    {
        char slug[101];
        char name[256];
        char color[8];
        char last_used[33];
    };

    struct Temperature
    {
        uint16_t id;
        uint16_t child;
        float temperature;
        char time[33];
        char notes[256];
        char tags[256];
    };

    struct Timer
    {
        uint16_t id; // read only
        uint16_t child;
        char name[256];
        char start[33];
        char end[33];      // read only
        char duration[17]; // read only
        bool active;     // read only
        uint16_t user;
    };

    struct TummyTime
    {
        uint16_t id;
        uint16_t child;    // Required unless a Timer value is provided.
        char start[33]; // Required unless a Timer value is provided.
        char end[33];   // Required unless a Timer value is provided.
        uint16_t timer;    // May be used in place of the Start, End, and/or Child values.
        char duration[17];
        char milestone[256];
        char tags[256];
    };

    struct Weight
    {
        uint16_t id;
        uint16_t child;
        float weight;
        char date[33];
        char notes[256];
        char tags[256];
    };

    struct User
    {
        uint16_t id;
        char username[151];
        char first_name[151];
        char last_name[151];
        char email[151];
        bool is_staff;
    };

    struct Profile
    {
        User user;
        char language[256];
        char timezone[101];
        char api_key[129];
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
        const char * requestBody = {});

    searchResults<BMI> findBMIRecords(
        uint16_t offset = 0,
        uint16_t child = 0,
        const char * date = {},
        const char * ordering = {});

    BMI getBMI(uint16_t id);

    BMI logBMI(
        uint16_t child,
        float bmi,
        char * date,
        char * notes = {},
        char * tags = {});

    BMI updateBMI(
        uint16_t id,
        uint16_t child = 0,
        float bmi = NAN,
         char * date = {},
        bool updateNotes = false,
         char * notes = {},
        bool updateTags = false,
         char * tags = {});

    bool deleteBMI(uint16_t id);

    searchResults<DiaperChange> findDiaperChanges(
        uint16_t offset = 0,
        uint16_t child = 0,
         char * colour = {},
         char * date = {},
         char * date_max = {},
         char * date_min = {},
         char * solid = {},
         char * wet = {},
         char * tags = {},
         char * ordering = {});

    BabyApi::DiaperChange BabyApi::logDiaperChange(
        uint16_t child,
        bool wet,
        bool solid,
         char * color = {},
        float amount = NAN,
         char * notes = {},
         char * tags = {});

    DiaperChange logDiaperChange(
        uint16_t child,
         char * time,
        bool wet = false,
        bool solid = false,
         char * color = {},
        float amount = NAN,
         char * notes = {},
         char * tags = {});

    DiaperChange getDiaperChange(uint16_t id);

    DiaperChange updateDiaperChange(
        uint16_t id,
        uint16_t child = 0,
         char * time = {},
         char * wet = {},
         char * solid = {},
         char * color = {},
        float amount = NAN,
        bool updateNotes = false,
         char * notes = {},
        bool updateTags = false,
         char * tags = {});

    bool removeDiaperChange(uint16_t id);

    searchResults<Child> findChildren(
        uint16_t offset = 0,
         char * first_name = {},
         char * last_name = {},
         char * birth_date = {},
         char * slug = {},
         char * ordering = {});

    Child newChild(
         char * first_name,
         char * last_name,
         char * birth_date,
         char * picture);

    Child getChild( char * slug);

    Child updateChild(
         char * slug,
         char * first_name = {},
         char * last_name = {},
         char * birth_date = {},
        bool updatePicture = false,
         char * picture = {});

    bool removeChild( char * slug);

    searchResults<Feeding> findFeedingRecords(
        uint16_t offset = 0,
        uint16_t child = 0,
         char * start = {},
         char * start_max = {},
         char * start_min = {},
         char * end = {},
         char * end_max = {},
         char * end_min = {},
         char * type = {},
         char * method = {},
         char * tags = {},
         char * ordering = {});

    Feeding logFeeding(
        uint16_t child,    // Required unless a Timer value is provided.
         char * start, // Required unless a Timer value is provided.
         char * end,   // Required unless a Timer value is provided.
         char * type,
         char * method,
        float amount = NAN,
         char * notes = {},
         char * tags = {});

    Feeding logFeeding(
        uint16_t timer,
         char * type,
         char * method,
        float amount = NAN,
         char * notes = {},
         char * tags = {});

    Feeding logFeeding(
        uint16_t child = 0,          // Required unless a Timer value is provided.
         char * start = {}, // Required unless a Timer value is provided.
         char * end = {},   // Required unless a Timer value is provided.
         char * type = {},
         char * method = {},
        float amount = NAN,
        const char * notes = {},
        const char * tags = {});

    Feeding logFeeding(
        uint16_t child = 0,          // Required unless a Timer value is provided.
        const char * start = {}, // Required unless a Timer value is provided.
        const char * end = {},   // Required unless a Timer value is provided.
        uint16_t timer = 0,          // May be used in place of the Start, End, and/or Child values.
        const char * type = {},
        const char * method = {},
        float amount = NAN,
        const char * notes = {},
        const char * tags = {});

    Feeding getFeeding(uint16_t id);

    Feeding updateFeeding(
        uint16_t id = 0,
        uint16_t child = 0,          // Required unless a Timer value is provided.
        const char * start = {}, // Required unless a Timer value is provided.
        const char * end = {},   // Required unless a Timer value is provided.
        const char * method = {},
        const char * type = {},
        float amount = NAN,
        bool updateNotes = false,
        const char * notes = {},
        bool updateTags = false,
        const char * tags = {});

    bool removeFeeding(uint16_t id);

    searchResults<HeadCircumference> findHeadCircumferenceRecords(
        uint16_t offset = 0,
        uint16_t child = 0,
        const char * date = {},
        const char * ordering = {});

    HeadCircumference logHeadCircumference(
        uint16_t child,
        float head_circumference,
        const char * date,
        const char * notes = {},
        const char * tags = {});

    HeadCircumference getHeadCircumference(uint16_t id);

    HeadCircumference updateHeadCircumference(
        uint16_t id,
        uint16_t child = 0,
        float head_circumference = NAN,
        const char * date = {},
        bool updateNotes = false,
        const char * notes = {},
        bool updateTags = false,
        const char * tags = {});

    bool removeHeadCircumference(uint16_t id);

    searchResults<Height> findHeightRecords(
        uint16_t offset = 0,
        uint16_t child = 0,
        const char * date = {},
        const char * ordering = {});

    Height logHeight(
        uint16_t child,
        float height,
        const char * date,
        const char * notes = {},
        const char * tags = {});

    Height getHeight(uint16_t id);

    Height updateHeight(
        uint16_t id,
        uint16_t child = 0,
        float height = NAN,
        const char * date = {},
        bool updateNotes = false,
        const char * notes = {},
        bool updateTags = false,
        const char * tags = {});

    bool removeHeight(uint16_t id);

    searchResults<Note> findNotes(
        uint16_t offset = 0,
        uint16_t child = 0,
        const char * date = {},
        const char * date_max = {},
        const char * date_min = {},
        const char * tags = {},
        const char * ordering = {});

    Note createNote(
        uint16_t child,
        const char * note,
        const char * date,
        const char * tags = {});

    Note getNote(uint16_t id);

    Note updateNote(
        uint16_t id,
        uint16_t child = 0,
        const char * date = {},
        bool updateNote = false,
        const char * note = {},
        bool updateTags = false,
        const char * tags = {});

    bool removeNote(uint16_t id);

    searchResults<Pumping> findPumpingRecords(
        uint16_t offset = 0,
        uint16_t child = 0,
        const char * date = {},
        const char * date_max = {},
        const char * date_min = {},
        const char * ordering = {});

    Pumping logPumping(
        uint16_t child,
        float amount,
        const char * time = {},
        const char * notes = {},
        const char * tags = {});

    Pumping getPumping(uint16_t id);

    Pumping updatePumping(
        uint16_t id,
        uint16_t child = 0,
        float amount = NAN,
        const char * time = {},
        bool updateNotes = false,
        const char * notes = {},
        bool updateTags = false,
        const char * tags = {});

    bool removePumping(uint16_t id);

    searchResults<Sleep> findSleepRecords(
        uint16_t offset = 0,
        uint16_t child = 0,
        const char * start = {},
        const char * start_max = {},
        const char * start_min = {},
        const char * end = {},
        const char * end_max = {},
        const char * end_min = {},
        const char * tags = {},
        const char * ordering = {});

    Sleep logSleep(
        uint16_t child,
        const char * start,
        const char * end,
        const char * notes = {},
        const char * tags = {});

    Sleep logSleep(
        uint16_t timer,
        const char * notes = {},
        const char * tags = {});

    Sleep logSleep(
        uint16_t child = 0,          // Required unless a Timer value is provided.
        const char * start = {}, // Required unless a Timer value is provided.
        const char * end = {},   // Required unless a Timer value is provided.
        uint16_t timer = 0,          // May be used in place of the Start, End, and/or Child values.
        const char * notes = {},
        const char * tags = {});

    Sleep getSleep(uint16_t id);

    Sleep updateSleep(
        uint16_t id,
        uint16_t child = 0,
        const char * start = {},
        const char * end = {},
        bool updateNotes = false,
        const char * notes = {},
        bool updateTags = false,
        const char * tags = {});

    bool removeSleep(uint16_t id);

    searchResults<Tag> findAllTags(
        uint16_t offset = 0,
        const char * name = {},
        const char * last_used = {},
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
        const char * date = {},
        const char * date_max = {},
        const char * date_min = {},
        const char * tags = {},
        const char * ordering = {});

    Temperature logTemperature(
        uint16_t child,
        float temperature,
        const char * time,
        const char * notes = {},
        const char * tags = {});

    Temperature getTemperature(uint16_t id);

    Temperature updateTemperature(
        uint16_t id = 0,
        uint16_t child = 0,
        float temperature = NAN,
        const char * time = {},
        bool updateNotes = false,
        const char * notes = {},
        bool updateTags = false,
        const char * tags = {});

    bool removeTemperature(uint16_t id);

    searchResults<Timer> findTimers(
        uint16_t offset = 0,
        uint16_t child = 0,
        const char * start = {},
        const char * start_max = {},
        const char * start_min = {},
        const char * end = {},
        const char * end_max = {},
        const char * end_min = {},
        const char * active = {},
        uint16_t user = 0,
        const char * ordering = {});

    Timer createTimer(
        uint16_t child);

    Timer createTimer(
        uint16_t child,
        const char * name);

    Timer createTimer(
        uint16_t child,
        const char * start);

    Timer createTimer(
        uint16_t child,
        const char * name,
        const char * start);

    Timer getTimer(uint16_t id);

    Timer updateTimer(
        uint16_t id,
        uint16_t child = 0,
        const char * name = {},
        const char * start = {},
        uint16_t user = 0);

    bool removeTimer(uint16_t id);

    Timer restartTimer(uint16_t id);

    Timer stopTimer(uint16_t id);

    searchResults<TummyTime> findTummyTimes(
        uint16_t offset = 0,
        uint16_t child = 0,
        const char * start = {},
        const char * start_max = {},
        const char * start_min = {},
        const char * end = {},
        const char * end_max = {},
        const char * end_min = {},
        const char * tags = {},
        const char * ordering = {});

    TummyTime logTummyTime(
        uint16_t child,    // Required unless a Timer value is provided.
        const char * start, // Required unless a Timer value is provided.
        const char * end,   // Required unless a Timer value is provided.
        const char * milestone = {},
        const char * tags = {});

    TummyTime logTummyTime(

        uint16_t timer, // May be used in place of the Start, End, and/or Child values.
        const char * milestone = {},
        const char * tags = {});

    TummyTime logTummyTime(
        uint16_t child = 0,          // Required unless a Timer value is provided.
        const char * start = {}, // Required unless a Timer value is provided.
        const char * end = {},   // Required unless a Timer value is provided.
        uint16_t timer = 0,          // May be used in place of the Start, End, and/or Child values.
        const char * milestone = {},
        const char * tags = {});

    TummyTime getTummyTime(uint16_t id);

    TummyTime updateTummyTime(
        uint16_t id,
        uint16_t child = 0,
        const char * start = {},
        const char * end = {},
        bool updateMilestone = false,
        const char * milestone = {},
        bool updateTags = false,
        const char * tags = {});

    bool removeTummyTime(uint16_t id);

    searchResults<Weight> findWeightRecords(
        uint16_t offset = 0,
        uint16_t child = 0,
        const char * date = {},
        const char * ordering = {});

    Weight logWeight(
        uint16_t child,
        float weight,
        const char * date,
        const char * notes = {},
        const char * tags = {});

    Weight getWeight(uint16_t id);

    Weight updateWeight(
        uint16_t id,
        uint16_t child = 0,
        float weight = NAN,
        const char * date = {},
        bool updateNotes = false,
        const char * notes = {},
        bool updateTags = false,
        const char * tags = {});

    bool removeWeight(uint16_t id);

    Profile getProfile();

    const char * getServerHost();
    const char * getServerPort();
    const char * getApiKey();

    void setServerHost(const char * server_host);
    void setServerPort(const char * server_port);
    void setApiKey(const char * apiKey);

    uint16_t startTimer(uint16_t childId, String name = {}, uint16_t timer = 0);

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

    void ResponseParser(String parse);
};

extern BabyApi babyApi;

// #include "BabyApi.cpp"
#endif