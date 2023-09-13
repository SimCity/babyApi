#ifndef _BABY_API_H_
#define _BABY_API_H_

#include <Arduino.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <cmath>

#define DEFAULT_SERVER_HOST "192.168.1.1"
#define DEFAULT_SERVER_PORT "8000"
#define ENDPOINT "/api"

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

String a;

class BabyApi
{

public:
    BabyApi(const char * baby_api_key);

    BabyApi(const char * server_host, const char * baby_api_key);

    BabyApi(const char * server_host, const char * server_port, const char * baby_api_key);

    // SCHEMA
    char *BabyApi::stoolColours[5] =
    {
        "",
        "black",
        "brown",
        "green",
        "yellow"
    };

    struct BMI
    {
        int id;
        int child;
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

    char *feedingTypes[5] =
        {
            "",
            "Breast Milk",
            "Formula",
            "Fortified Breast Milk",
            "Solid Food"};

    char *feedingMethods[7] =
        {
            "",
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
        char name;
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
    String httpRequest(
        const char *endpoint,
        const char *type,
        const char *parameters = "",
        const char *query = "",
        char * requestBody = "",
        int *responseCode = nullptr);

    searchResults<BMI> findBMIRecords(
        int offset = -1,
        char * child = "",
        char * date = "",
        char * ordering = "");

    BMI getBMI(int id);

    BMI logBMI(
        int child,
        float bmi,
        char * date,
        char * notes = "",
        char * tags = "");

    BMI updateBMI(
        int id,
        int child = -1,
        float bmi = NAN,
        char * date = "",
        bool updateNotes = false,
        char * notes = "",
        bool updateTags = false,
        char * tags = "");

    bool deleteBMI(int id);

    searchResults<DiaperChange> findDiaperChanges(
        int offset = -1,
        int child = -1,
        StoolColour colour = null,
        char * date = "",
        char * date_max = "",
        char * date_min = "",
        char * solid = "",
        char * wet = "",
        char * tags = "",
        char * ordering = "");

    BabyApi::DiaperChange BabyApi::logDiaperChange(
        int child,
        bool wet,
        bool solid,
        StoolColour color = StoolColour::null,
        float amount = NAN,
        char * notes = "",
        char * tags[] = {});

    DiaperChange logDiaperChange(
        int child,
        char * time,
        bool wet = false,
        bool solid = false,
        StoolColour color = null,
        float amount = NAN,
        char * notes = "",
        char * tags[] = {});

    DiaperChange getDiaperChange(int id);

    DiaperChange updateDiaperChange(
        int id,
        int child = -1,
        char * time = "",
        char * wet = "",
        char * solid = "",
        StoolColour color = null,
        float amount = NAN,
        bool updateNotes = false,
        char * notes = "",
        bool updateTags = false,
        char * tags = "");

    bool removeDiaperChange(int id);

    searchResults<Child> findChildren(
        int offset = -1,
        char * first_name = "",
        char * last_name = "",
        char * birth_date = "",
        char * slug = "",
        char * ordering = "");

    Child newChild(
        char * first_name,
        char * last_name,
        char * birth_date,
        char * picture);

    Child getChild(String slug);

    Child updateChild(
        char * slug,
        char * first_name = "",
        char * last_name = "",
        char * birth_date = "",
        bool updatePicture = false,
        char * picture = "");

    bool removeChild(String slug);

    searchResults<Feeding> findFeedingRecords(
        int offset = -1,
        int child = -1,
        char * start = "",
        char * start_max = "",
        char * start_min = "",
        char * end = "",
        char * end_max = "",
        char * end_min = "",
        char * type = "",
        char * method = "",
        char * tags = "",
        char * ordering = "");

    Feeding logFeeding(
        int child,    // Required unless a Timer value is provided.
        char * start, // Required unless a Timer value is provided.
        char * end,   // Required unless a Timer value is provided.
        FeedingType type,
        FeedingMethod method,
        float amount = NAN,
        char * notes = "",
        char * tags[] = {});

    Feeding logFeeding(
        int timer,
        FeedingType type,
        FeedingMethod method,
        float amount = NAN,
        char * notes = "",
        char * tags[] = {});

    Feeding logFeeding(
        int child = -1,          // Required unless a Timer value is provided.
        char * start = "", // Required unless a Timer value is provided.
        char * end = "",   // Required unless a Timer value is provided.
        FeedingType type = FeedingType::empty,
        FeedingMethod method = FeedingMethod::empty,
        float amount = NAN,
        char * notes = "",
        char * tags[] = {});

    Feeding logFeeding(
        int child = -1,          // Required unless a Timer value is provided.
        char * start = "", // Required unless a Timer value is provided.
        char * end = "",   // Required unless a Timer value is provided.
        int timer = -1,          // May be used in place of the Start, End, and/or Child values.
        FeedingType type = FeedingType::empty,
        FeedingMethod method = FeedingMethod::empty,
        float amount = NAN,
        char * notes = "",
        char * tags[] = {});

    Feeding getFeeding(int id);

    Feeding updateFeeding(
        int id = -1,
        int child = -1,          // Required unless a Timer value is provided.
        char * start = "", // Required unless a Timer value is provided.
        char * end = "",   // Required unless a Timer value is provided.
        FeedingMethod method = FeedingMethod::null,
        FeedingType type = FeedingType::null,
        float amount = NAN,
        bool updateNotes = false,
        char * notes = "",
        bool updateTags = false,
        char * tags = "");

    bool removeFeeding(int id);

    searchResults<HeadCircumference> findHeadCircumferenceRecords(
        int offset = -1,
        int child = -1,
        char * date = "",
        char * ordering = "");

    HeadCircumference logHeadCircumference(
        int child,
        float head_circumference,
        char * date,
        char * notes,
        char * tags[]);

    HeadCircumference getHeadCircumference(int id);

    HeadCircumference updateHeadCircumference(
        int id,
        int child = -1,
        float head_circumference = NAN,
        char * date = "",
        bool updateNotes = false,
        char * notes = "",
        bool updateTags = false,
        char * tags = "");

    bool removeHeadCircumference(int id);

    searchResults<Height> findHeightRecords(
        int offset = -1,
        int child = -1,
        char * date = "",
        char * ordering = "");

    Height logHeight(
        int child,
        float height,
        char * date,
        char * notes,
        char * tags[]);

    Height getHeight(int id);

    Height updateHeight(
        int id,
        int child = -1,
        float height = NAN,
        char * date = "",
        bool updateNotes = false,
        char * notes = "",
        bool updateTags = false,
        char * tags = "");

    bool removeHeight(int id);

    searchResults<Note> findNotes(
        int offset = -1,
        int child = -1,
        char * date = "",
        char * date_max = "",
        char * date_min = "",
        char * tags = "",
        char * ordering = "");

    Note createNote(
        int child,
        char * note,
        char * date,
        char * tags[] = {});

    Note getNote(int id);

    Note updateNote(
        int id,
        int child = -1,
        char * date = "",
        bool updateNote = false,
        char * note = "",
        bool updateTags = false,
        char * tags = "");

    bool removeNote(int id);

    searchResults<Pumping> findPumpingRecords(
        int offset = -1,
        int child = -1,
        char * date = "",
        char * date_max = "",
        char * date_min = "",
        char * ordering = "");

    Pumping logPumping(
        int child,
        float amount,
        char * time = "",
        char * notes = "",
        char * tags[] = {});

    Pumping getPumping(int id);

    Pumping updatePumping(
        int id,
        int child = -1,
        float amount = NAN,
        char * time = "",
        bool updateNotes = false,
        char * notes = "",
        bool updateTags = false,
        char * tags = "");

    bool removePumping(int id);

    searchResults<Sleep> findSleepRecords(
        int offset = -1,
        int child = -1,
        char * start = "",
        char * start_max = "",
        char * start_min = "",
        char * end = "",
        char * end_max = "",
        char * end_min = "",
        char * tags = "",
        char * ordering = "");

    Sleep logSleep(
        int child,
        char * start,
        char * end,
        char * notes = "",
        char * tags[] = {});

    Sleep logSleep(
        int timer,
        char * notes = "",
        char * tags[] = {});

    Sleep logSleep(
        int child = -1,          // Required unless a Timer value is provided.
        char * start = "", // Required unless a Timer value is provided.
        char * end = "",   // Required unless a Timer value is provided.
        int timer = -1,          // May be used in place of the Start, End, and/or Child values.
        char * notes = "",
        char * tags[] = {});

    Sleep getSleep(int id);

    Sleep updateSleep(
        int id,
        int child = -1,
        char * start = "",
        char * end = "",
        bool updateNotes = false,
        char * notes = "",
        bool updateTags = false,
        char * tags = "");

    bool removeSleep(int id);

    searchResults<Tag> findAllTags(
        int offset = -1,
        char * name = "",
        char * last_used = "",
        char * ordering = "");

    Tag createTag(
        char * name,
        char * colour = "");

    Tag getTag(
        char * slug);

    Tag updateTag(
        char * slug,
        bool updateName = false,
        char * name = "",
        bool updateColour = false,
        char * colour = "");

    bool removeTag(String slug);

    searchResults<Temperature> findTemperatureRecords(
        int offset = -1,
        int child = -1,
        char * date = "",
        char * date_max = "",
        char * date_min = "",
        char * tags = "",
        char * ordering = "");

    Temperature logTemperature(
        int child,
        float temperature,
        char * time,
        char * notes,
        char * tags[]);

    Temperature getTemperature(int id);

    Temperature updateTemperature(
        int id = -1,
        int child = -1,
        float temperature = NAN,
        char * time = "",
        bool updateNotes = false,
        char * notes = "",
        bool updateTags = false,
        char * tags = "");

    bool removeTemperature(int id);

    searchResults<Timer> findTimers(
        int offset = -1,
        int child = -1,
        char * start = "",
        char * start_max = "",
        char * start_min = "",
        char * end = "",
        char * end_max = "",
        char * end_min = "",
        char * active = "",
        int user = -1,
        char * ordering = "");

    Timer createTimer(
        int child);

    Timer createTimer(
        int child,
        char * name);

    Timer createTimer(
        int child,
        char * start);

    Timer createTimer(
        int child,
        char * name,
        char * start);

    Timer getTimer(int id);

    Timer updateTimer(
        int id,
        int child = -1,
        char * name = "",
        char * start = "",
        int user = -1);

    bool removeTimer(int id);

    Timer restartTimer(int id);

    Timer stopTimer(int id);

    searchResults<TummyTime> findTummyTimes(
        int offset = -1,
        int child = -1,
        char * start = "",
        char * start_max = "",
        char * start_min = "",
        char * end = "",
        char * end_max = "",
        char * end_min = "",
        char * tags = "",
        char * ordering = "");

    TummyTime logTummyTime(
        int child,    // Required unless a Timer value is provided.
        char * start, // Required unless a Timer value is provided.
        char * end,   // Required unless a Timer value is provided.
        char * milestone = "",
        char * tags[] = {});

    TummyTime logTummyTime(

        int timer, // May be used in place of the Start, End, and/or Child values.
        char * milestone = "",
        char * tags[] = {});

    TummyTime logTummyTime(
        int child = -1,          // Required unless a Timer value is provided.
        char * start = "", // Required unless a Timer value is provided.
        char * end = "",   // Required unless a Timer value is provided.
        int timer = -1,          // May be used in place of the Start, End, and/or Child values.
        char * milestone = "",
        char * tags[] = {});

    TummyTime getTummyTime(int id);

    TummyTime updateTummyTime(
        int id,
        int child = -1,
        char * start = "",
        char * end = "",
        bool updateMilestone = false,
        char * milestone = "",
        bool updateTags = false,
        char * tags = "");

    bool removeTummyTime(int id);

    searchResults<Weight> findWeightRecords(
        int offset = -1,
        int child = -1,
        char * date = "",
        char * ordering = "");

    Weight logWeight(
        int child,
        float weight,
        char * date,
        char * notes = "",
        char * tags[] = {});

    Weight getWeight(int id);

    Weight updateWeight(
        int id,
        int child = -1,
        float weight = NAN,
        char * date = "",
        bool updateNotes = false,
        char * notes = "",
        bool updateTags = false,
        char * tags = "");

    bool removeWeight(int id);

    Profile getProfile();

    const char * getServerHost();
    const char * getServerPort();
    const char * getApiKey();

    void setServerHost(const char * server_host);
    void setServerPort(const char * server_port);
    void setApiKey(const char * apiKey);

    int startTimer(int childId, char * name = "", int timer = -1);

    void searchResultParser(DynamicJsonDocument result, long *count, long *next, long *previous);

    uint8_t getAllChildren(Child *children, uint8_t count);

    uint8_t recordFeeding(uint16_t timerId, FeedingType feedingType, FeedingMethod feedingMethod, float amount);

    uint8_t recordSleep(uint16_t timerId);

    uint8_t recordPumping(uint16_t timerId, float amount);

    uint8_t recordTummyTime(uint16_t timerId);

    uint8_t recordNappyChange(uint16_t child, bool wet, bool solid, uint16_t colour);

protected:
    const char * serverHost;
    const char * serverPort;
    const char * babyApiKey;
};

extern BabyApi babyApi;

// #include "BabyApi.cpp"
#endif