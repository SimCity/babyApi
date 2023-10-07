#ifndef _BABY_API_H_
#define _BABY_API_H_

#include <Arduino.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <cmath>
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
         char * notes = {},
         char * tags = {});

    Feeding logFeeding(
        uint16_t child = 0,          // Required unless a Timer value is provided.
         char * start = {}, // Required unless a Timer value is provided.
         char * end = {},   // Required unless a Timer value is provided.
        uint16_t timer = 0,          // May be used in place of the Start, End, and/or Child values.
         char * type = {},
         char * method = {},
        float amount = NAN,
         char * notes = {},
         char * tags = {});

    Feeding getFeeding(uint16_t id);

    Feeding updateFeeding(
        uint16_t id = 0,
        uint16_t child = 0,          // Required unless a Timer value is provided.
         char * start = {}, // Required unless a Timer value is provided.
         char * end = {},   // Required unless a Timer value is provided.
         char * method = {},
         char * type = {},
        float amount = NAN,
        bool updateNotes = false,
         char * notes = {},
        bool updateTags = false,
         char * tags = {});

    bool removeFeeding(uint16_t id);

    searchResults<HeadCircumference> findHeadCircumferenceRecords(
        uint16_t offset = 0,
        uint16_t child = 0,
         char * date = {},
         char * ordering = {});

    HeadCircumference logHeadCircumference(
        uint16_t child,
        float head_circumference,
         char * date,
         char * notes = {},
         char * tags = {});

    HeadCircumference getHeadCircumference(uint16_t id);

    HeadCircumference updateHeadCircumference(
        uint16_t id,
        uint16_t child = 0,
        float head_circumference = NAN,
         char * date = {},
        bool updateNotes = false,
         char * notes = {},
        bool updateTags = false,
         char * tags = {});

    bool removeHeadCircumference(uint16_t id);

    searchResults<Height> findHeightRecords(
        uint16_t offset = 0,
        uint16_t child = 0,
         char * date = {},
         char * ordering = {});

    Height logHeight(
        uint16_t child,
        float height,
         char * date,
         char * notes = {},
         char * tags = {});

    Height getHeight(uint16_t id);

    Height updateHeight(
        uint16_t id,
        uint16_t child = 0,
        float height = NAN,
         char * date = {},
        bool updateNotes = false,
         char * notes = {},
        bool updateTags = false,
         char * tags = {});

    bool removeHeight(uint16_t id);

    searchResults<Note> findNotes(
        uint16_t offset = 0,
        uint16_t child = 0,
         char * date = {},
         char * date_max = {},
         char * date_min = {},
         char * tags = {},
         char * ordering = {});

    Note createNote(
        uint16_t child,
         char * note,
         char * date,
         char * tags = {});

    Note getNote(uint16_t id);

    Note updateNote(
        uint16_t id,
        uint16_t child = 0,
         char * date = {},
        bool updateNote = false,
         char * note = {},
        bool updateTags = false,
         char * tags = {});

    bool removeNote(uint16_t id);

    searchResults<Pumping> findPumpingRecords(
        uint16_t offset = 0,
        uint16_t child = 0,
         char * date = {},
         char * date_max = {},
         char * date_min = {},
         char * ordering = {});

    Pumping logPumping(
        uint16_t child,
        float amount,
         char * time = {},
         char * notes = {},
         char * tags = {});

    Pumping getPumping(uint16_t id);

    Pumping updatePumping(
        uint16_t id,
        uint16_t child = 0,
        float amount = NAN,
         char * time = {},
        bool updateNotes = false,
         char * notes = {},
        bool updateTags = false,
         char * tags = {});

    bool removePumping(uint16_t id);

    searchResults<Sleep> findSleepRecords(
        uint16_t offset = 0,
        uint16_t child = 0,
         char * start = {},
         char * start_max = {},
         char * start_min = {},
         char * end = {},
         char * end_max = {},
         char * end_min = {},
         char * tags = {},
         char * ordering = {});

    Sleep logSleep(
        uint16_t child,
         char * start,
         char * end,
         char * notes = {},
         char * tags = {});

    Sleep logSleep(
        uint16_t timer,
         char * notes = {},
         char * tags = {});

    Sleep logSleep(
        uint16_t child = 0,          // Required unless a Timer value is provided.
         char * start = {}, // Required unless a Timer value is provided.
         char * end = {},   // Required unless a Timer value is provided.
        uint16_t timer = 0,          // May be used in place of the Start, End, and/or Child values.
         char * notes = {},
         char * tags = {});

    Sleep getSleep(uint16_t id);

    Sleep updateSleep(
        uint16_t id,
        uint16_t child = 0,
         char * start = {},
         char * end = {},
        bool updateNotes = false,
         char * notes = {},
        bool updateTags = false,
         char * tags = {});

    bool removeSleep(uint16_t id);

    searchResults<Tag> findAllTags(
        uint16_t offset = 0,
         char * name = {},
         char * last_used = {},
         char * ordering = {});

    Tag createTag(
         char * name,
         char * colour = {});

    Tag getTag(
         char * slug);

    Tag updateTag(
         char * slug,
        bool updateName = false,
         char * name = {},
        bool updateColour = false,
         char * colour = {});

    bool removeTag( char * slug);

    searchResults<Temperature> findTemperatureRecords(
        uint16_t offset = 0,
        uint16_t child = 0,
         char * date = {},
         char * date_max = {},
         char * date_min = {},
         char * tags = {},
         char * ordering = {});

    Temperature logTemperature(
        uint16_t child,
        float temperature,
         char * time,
         char * notes = {},
         char * tags = {});

    Temperature getTemperature(uint16_t id);

    Temperature updateTemperature(
        uint16_t id = 0,
        uint16_t child = 0,
        float temperature = NAN,
         char * time = {},
        bool updateNotes = false,
         char * notes = {},
        bool updateTags = false,
         char * tags = {});

    bool removeTemperature(uint16_t id);

    searchResults<Timer> findTimers(
        uint16_t offset = 0,
        uint16_t child = 0,
         char * start = {},
         char * start_max = {},
         char * start_min = {},
         char * end = {},
         char * end_max = {},
         char * end_min = {},
         char * active = {},
        uint16_t user = 0,
         char * ordering = {});

    Timer createTimer(
        uint16_t child);

    Timer createTimer(
        uint16_t child,
         char * name);

    Timer createTimer(
        uint16_t child,
         char * start);

    Timer createTimer(
        uint16_t child,
         char * name,
         char * start);

    Timer getTimer(uint16_t id);

    Timer updateTimer(
        uint16_t id,
        uint16_t child = 0,
         char * name = {},
         char * start = {},
        uint16_t user = 0);

    bool removeTimer(uint16_t id);

    Timer restartTimer(uint16_t id);

    Timer stopTimer(uint16_t id);

    searchResults<TummyTime> findTummyTimes(
        uint16_t offset = 0,
        uint16_t child = 0,
         char * start = {},
         char * start_max = {},
         char * start_min = {},
         char * end = {},
         char * end_max = {},
         char * end_min = {},
         char * tags = {},
         char * ordering = {});

    TummyTime logTummyTime(
        uint16_t child,    // Required unless a Timer value is provided.
         char * start, // Required unless a Timer value is provided.
         char * end,   // Required unless a Timer value is provided.
         char * milestone = {},
         char * tags = {});

    TummyTime logTummyTime(

        uint16_t timer, // May be used in place of the Start, End, and/or Child values.
         char * milestone = {},
         char * tags = {});

    TummyTime logTummyTime(
        uint16_t child = 0,          // Required unless a Timer value is provided.
         char * start = {}, // Required unless a Timer value is provided.
         char * end = {},   // Required unless a Timer value is provided.
        uint16_t timer = 0,          // May be used in place of the Start, End, and/or Child values.
         char * milestone = {},
         char * tags = {});

    TummyTime getTummyTime(uint16_t id);

    TummyTime updateTummyTime(
        uint16_t id,
        uint16_t child = 0,
         char * start = {},
         char * end = {},
        bool updateMilestone = false,
         char * milestone = {},
        bool updateTags = false,
         char * tags = {});

    bool removeTummyTime(uint16_t id);

    searchResults<Weight> findWeightRecords(
        uint16_t offset = 0,
        uint16_t child = 0,
         char * date = {},
         char * ordering = {});

    Weight logWeight(
        uint16_t child,
        float weight,
         char * date,
         char * notes = {},
         char * tags = {});

    Weight getWeight(uint16_t id);

    Weight updateWeight(
        uint16_t id,
        uint16_t child = 0,
        float weight = NAN,
         char * date = {},
        bool updateNotes = false,
         char * notes = {},
        bool updateTags = false,
         char * tags = {});

    bool removeWeight(uint16_t id);

    Profile getProfile();

    const char * getServerHost();
    const char * getServerPort();
    const char * getApiKey();

    void setServerHost(const char * server_host);
    void setServerPort(const char * server_port);
    void setApiKey(const char * apiKey);

    uint16_t startTimer(uint16_t childId, char * name = {}, uint16_t timer = 0);

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