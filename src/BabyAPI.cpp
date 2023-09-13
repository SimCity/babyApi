
#include "BabyAPI.h"


BabyApi::BabyApi(const char * baby_api_key)
{
  serverHost = DEFAULT_SERVER_HOST;
  serverPort = DEFAULT_SERVER_PORT;
  babyApiKey = baby_api_key;
}

BabyApi::BabyApi(const char * server_host, const char * baby_api_key)
{
  serverHost = server_host;
  serverPort = DEFAULT_SERVER_PORT;
  babyApiKey = baby_api_key;
}

BabyApi::BabyApi(const char * server_host, const char * server_port, const char * baby_api_key)
{
  serverHost = server_host;
  serverPort = server_port;
  babyApiKey = baby_api_key;
}

const char * BabyApi::getServerHost()
{
  return serverHost;
}
const char * BabyApi::getServerPort()
{
  return serverPort;
}
const char * BabyApi::getApiKey()
{
  return babyApiKey;
}

void BabyApi::setServerHost(const char * server_host)
{
  serverHost = server_host;
}

void BabyApi::setServerPort(const char * server_port)
{
  serverPort = server_port;
}
void BabyApi::setApiKey(const char * apiKey)
{
  babyApiKey = apiKey;
}

String BabyApi::httpRequest(const char *endpoint, const char *type, const char *parameters = "", const char *query = "", char * requestBody = "", int *responseCode = nullptr)
{
  WiFiClientSecure client;
  HTTPClient https;



  https.addHeader("Authorization", "Token " + babyApiKey);

  https.begin(client, "https://" + serverHost + ":" + serverPort + ENDPOINT + "/" + endpoint + "/" + parameters + query);

  // Send HTTP POST request
  int httpsResponseCode = https.sendRequest(type, requestBody);

  if (responseCode != nullptr)
  {
    *responseCode = httpsResponseCode;
  }

  String payload = "{}";

  if (httpsResponseCode > 0)
  {
    Serial.print("HTTP Response code: ");
    Serial.println(httpsResponseCode);
    payload = https.getString();
  }
  else
  {
    Serial.print("Error code: ");
    Serial.println(httpsResponseCode);
  }
  // Free resources
  https.end();

  return payload;
}

DynamicJsonDocument JsonParser(String parse)
{
  DynamicJsonDocument doc(JOSN_CAPACITY);
  DeserializationError err = deserializeJson(doc, parse);
  if (err)
  {
    Serial.print(F("deserializeJson() failed with code "));
    Serial.println(err.f_str());
  }
}


JsonArray serialiseTags(char * tags)
{
  JsonArray tagsArray;

  char *token;

  if(String(tags).length() > 0)
  {
    token = strtok(tags, ",");
    
    if(token == NULL)
    {
      tagsArray.add(tags);
    }

    while(token != NULL)
    {
      tagsArray.add(token);
      token = strtok(tags, ",");
    } 
  }

  return tagsArray;
}

String deserialiseTags(JsonArray tags)
{
  String tagList = "";

  for (JsonPair tagRecord : tags)
  {
    if (tagList.length() == 0)
    {
      tagList = tagRecord.value().as<String>();
    }
    else
    {
      tagList += "," + tagRecord.value().as<String>();
    }
  }

  return tagList;
}

void BabyApi::searchResultParser(DynamicJsonDocument result, long *count, long *next, long *previous)
{
  int offsetLocation;

  *count = result["count"];

  offsetLocation = result["next"].as<String>().indexOf("offset=");

  *next = (offsetLocation > -1) ? result["next"].as<String>().substring(offsetLocation + 7).toInt() : -1;

  offsetLocation = result["previous"].as<String>().indexOf("offset=");

  *previous = (offsetLocation > -1) ? result["previous"].as<String>().substring(offsetLocation + 7).toInt() : -1;
}

BabyApi::searchResults<BabyApi::BMI> BabyApi::findBMIRecords(
    uint16_t offset = 0,
    uint16_t child = 0,
    char * date = "",
    char * ordering = "")
{
  BabyApi::searchResults<BabyApi::BMI> outcome;
  int count = 0;

  String query = "limit=" + SEARCH_LIMIT +
                         (offset > 0)
                     ? ",offset=" + String(offset)
                 : "" +
                         (child > 0)
                     ? ",child=" + String(child)
                 : "" +
                         (date[0] != '\0')
                     ? ",date=" + String(date)
                 : "" +
                         (ordering[0] != '\0')
                     ? ",ordering=" + String(ordering)
                     : "";

  String jsonBuffer;
  jsonBuffer = httpRequest(BMI_ENDPOINT, "GET", "", query.c_str());
  Serial.println(jsonBuffer);

  DynamicJsonDocument result = JsonParser(jsonBuffer);

  searchResultParser(result, &outcome.count, &outcome.next, &outcome.previous);

  JsonArray results = result["results"].as<JsonArray>();

  for (JsonObject bmiRecord : results)
  {
    outcome.results[count].id = bmiRecord["id"];
    outcome.results[count].child = bmiRecord["child"];
    bmiRecord["date"].as<String>().toCharArray(outcome.results[count].date,26);
    outcome.results[count].bmi = bmiRecord["bmi"];
    bmiRecord["notes"].as<String>().toCharArray(outcome.results[count].notes,256);
    outcome.results[count].tags = deserialiseTags(bmiRecord["tags"].as<JsonArray>());

    count++;
  }

  return outcome;
}

BabyApi::BMI BabyApi::getBMI(int id)
{
  BabyApi::BMI outcome;

  String parameters = "/" + String(id) + "/";

  String jsonBuffer;
  jsonBuffer = httpRequest(BMI_ENDPOINT, "GET", parameters.c_str());
  Serial.println(jsonBuffer);

  DynamicJsonDocument result = JsonParser(jsonBuffer);

  JsonArray results = result["results"].as<JsonArray>();

  outcome.id = result["id"];
  outcome.child = result["child"];
  result["date"].as<String>().toCharArray(outcome.date,33);
  outcome.bmi = result["bmi"];
  result["notes"].as<String>().toCharArray(outcome.notes,256);
  deserialiseTags(result["tags"].as<JsonArray>()).toCharArray(outcome.tags, 256);

  return outcome;
}


BabyApi::BMI BabyApi::logBMI(
    int child,
    float bmi,
    char * date,
    char * notes = "",
    char * tags = "")
{
  DynamicJsonDocument doc(JOSN_CAPACITY);
  BabyApi::BMI outcome;
  char requestBody[JOSN_CAPACITY] = "";

  doc["child"] = child;
  doc["date"] = date;
  doc["bmi"] = bmi;
  doc["notes"] = notes;
  doc["tags"] = serialiseTags(tags);
  
  serializeJson(doc, requestBody);

  String jsonBuffer;
  jsonBuffer = httpRequest(BMI_ENDPOINT, "POST", "", "", requestBody);
  Serial.println(jsonBuffer);

  DynamicJsonDocument result = JsonParser(jsonBuffer);

  outcome.id = result["id"];
  outcome.child = result["child"];
  result["date"].as<String>().toCharArray(outcome.date,33);
  outcome.bmi = result["bmi"];
  result["notes"].as<String>().toCharArray(outcome.notes,256);
  deserialiseTags(result["tags"].as<JsonArray>()).toCharArray(outcome.tags, 256);

  return outcome;
}

BabyApi::BMI BabyApi::updateBMI(
    int id,
    int child = -1,
    float bmi = NAN,
    String date = String(),
    bool updateNotes = false,
    String notes = String(),
    bool updateTags = false,
    String tags = String())
{
  DynamicJsonDocument doc(JOSN_CAPACITY);
  BabyApi::BMI outcome;
  char requestBody[JOSN_CAPACITY] = "";

  if (child > -1)
    doc["child"] = child;
  if (!date.isEmpty())
    doc["date"] = date;
  if (!isnan(bmi))
    doc["bmi"] = bmi;
  if (updateNotes)
    doc["notes"] = notes;
  if (updateTags)
    serialiseTags(tags, doc["tags"]);

  String parameters = "/" + String(id) + "/";


  serializeJson(doc, requestBody);

  String jsonBuffer;
  jsonBuffer = httpRequest(BMI_ENDPOINT, "PATCH", parameters.c_str(), "", requestBody);
  Serial.println(jsonBuffer);

  DynamicJsonDocument result = JsonParser(jsonBuffer);

  JsonArray results = result["results"].as<JsonArray>();

  outcome.id = result["id"];
  outcome.child = result["child"];
  result["date"].as<String>().toCharArray(outcome.date,33);
  outcome.bmi = result["bmi"];
  result["notes"].as<String>().toCharArray(outcome.notes,256);
  deserialiseTags(result["tags"].as<JsonArray>()).toCharArray(outcome.tags, 256);

  return outcome;
}

bool BabyApi::deleteBMI(int id)
{
  String parameters = "/" + String(id) + "/";
  int responseCode;
  String jsonBuffer;
  jsonBuffer = httpRequest(BMI_ENDPOINT, "DELETE", parameters.c_str(), "", "", &responseCode);
  Serial.println(jsonBuffer);

  return responseCode == 204;
}

BabyApi::searchResults<BabyApi::DiaperChange> BabyApi::findDiaperChanges(
    int offset = -1,
    int child = -1,
    StoolColour colour = StoolColour::null,
    char * date = "",
    char * date_max = "",
    char * date_min = "",
    char * solid = "",
    char * wet = "",
    char * tags = String(),
    char * ordering = "")
{
  BabyApi::searchResults<BabyApi::DiaperChange> outcome;
  int count = 0;

  String query = "?limit=" + SEARCH_LIMIT +
                 ((offset > -1) ? "&offset=" + String(offset) : "") +
                 ((offset > -1) ? "&child=" + String(child) : "") +
                 ((date.length() > 0) ? "&date=" + date : "") +
                 ((date_max.length() > 0) ? "&date_max=" + date_max : "") +
                 ((date_min.length() > 0) ? "&date_min=" + date_min : "") +
                 ((String(stoolColours[colour]).length() > 0) ? "&colour=" + String(stoolColours[colour]) : "") +
                 ((solid.length() > 0) ? "&solid=" + solid : "") +
                 ((wet.length() > 0) ? "&wet=" + wet : "") +
                 ((tags.length() > 0) ? "&tags=" + tags : "") +
                 ((ordering.length() > 0) ? "&ordering=" + ordering : "");

  String jsonBuffer;
  jsonBuffer = httpRequest(CHANGES_ENDPOINT, "GET", "", query.c_str());
  Serial.println(jsonBuffer);

  DynamicJsonDocument result = JsonParser(jsonBuffer);

  searchResultParser(result, &outcome.count, &outcome.next, &outcome.previous);

  JsonArray results = result["results"].as<JsonArray>();

  for (JsonObject diaperChangeRecord : results)
  {
    outcome.results[count].id = diaperChangeRecord["id"];
    outcome.results[count].child = diaperChangeRecord["child"];
    outcome.results[count].amount = diaperChangeRecord["amount"];
    outcome.results[count].color = diaperChangeRecord["color"];
    outcome.results[count].solid = diaperChangeRecord["solid"];
    outcome.results[count].wet = diaperChangeRecord["wet"];
    outcome.results[count].time = diaperChangeRecord["time"].as<String>();
    outcome.results[count].notes = diaperChangeRecord["notes"].as<String>();
    outcome.results[count].tags = deserialiseTags(diaperChangeRecord["tags"].as<JsonArray>());

    count++;
  }

  return outcome;
}

BabyApi::DiaperChange BabyApi::logDiaperChange(
    int child,
    bool wet,
    bool solid,
    StoolColour color = StoolColour::null,
    float amount = NAN,
    char * notes = "",
    char * tags = {})
{
  return logDiaperChange(child, "", wet, solid, color, amount, notes, tags);
}

BabyApi::DiaperChange BabyApi::logDiaperChange(
    int child,
    char * time,
    bool wet = false,
    bool solid = false,
    StoolColour color = StoolColour::null,
    float amount = NAN,
    char * notes = "",
    char * tags = {})
{
  DynamicJsonDocument doc(JOSN_CAPACITY);
  BabyApi::DiaperChange outcome;

  doc["child"] = child;
  doc["time"] = time;
  doc["wet"] = wet;
  doc["solid"] = solid;
  if (color != null)
    doc["color"] = stoolColours[color];
  if (!isnan(amount))
    doc["amount"] = amount;
  doc["notes"] = notes;
  serialiseTags(tags, doc["tags"]);

  String requestBody = "";
  serializeJson(doc, requestBody);

  String jsonBuffer;
  jsonBuffer = httpRequest(CHANGES_ENDPOINT, "POST", "", "", requestBody);
  Serial.println(jsonBuffer);

  DynamicJsonDocument result = JsonParser(jsonBuffer);

  outcome.id = result["id"];
  outcome.child = result["child"];
  outcome.amount = result["amount"];
  outcome.color = result["color"];
  outcome.solid = result["solid"];
  outcome.wet = result["wet"];
  outcome.time = result["time"].as<String>();
  result["notes"].as<String>().toCharArray(outcome.notes,256);
  deserialiseTags(result["tags"].as<JsonArray>()).toCharArray(outcome.tags, 256);

  return outcome;
}

BabyApi::DiaperChange BabyApi::getDiaperChange(int id)
{
  BabyApi::DiaperChange outcome;

  String parameters = "/" + String(id) + "/";

  String jsonBuffer;
  jsonBuffer = httpRequest(CHANGES_ENDPOINT, "GET", parameters.c_str());
  Serial.println(jsonBuffer);

  DynamicJsonDocument result = JsonParser(jsonBuffer);

  outcome.id = result["id"];
  outcome.child = result["child"];
  outcome.amount = result["amount"];
  outcome.color = result["color"];
  outcome.solid = result["solid"];
  outcome.wet = result["wet"];
  outcome.time = result["time"].as<String>();
  result["notes"].as<String>().toCharArray(outcome.notes,256);
  deserialiseTags(result["tags"].as<JsonArray>()).toCharArray(outcome.tags, 256);

  return outcome;
}

BabyApi::DiaperChange BabyApi::updateDiaperChange(
    int id,
    int child = -1,
    char * time = String(),
    char * wet = String(),
    char * solid = String(),
    StoolColour color = StoolColour::null,
    float amount = NAN,
    bool updateNotes = false,
    char * notes = String(),
    bool updateTags = false,
    char * tags = String())
{
  DynamicJsonDocument doc(JOSN_CAPACITY);
  BabyApi::DiaperChange outcome;

  if (child > -1)
    doc["child"] = child;
  if (!time.isEmpty())
    doc["time"] = time;
  if (!wet.isEmpty())
    doc["wet"] = wet;
  if (!solid.isEmpty())
    doc["solid"] = solid;
  if (color != null)
    doc["color"] = stoolColours[color];
  if (!isnan(amount))
    doc["amount"] = amount;
  if (updateNotes)
    doc["notes"] = notes;
  if (updateTags)
    serialiseTags(tags, doc["tags"]);

  String parameters = "/" + String(id) + "/";

  String requestBody = "";
  serializeJson(doc, requestBody);

  String jsonBuffer;
  jsonBuffer = httpRequest(CHANGES_ENDPOINT, "PATCH", parameters.c_str(), "", requestBody);
  Serial.println(jsonBuffer);

  DynamicJsonDocument result = JsonParser(jsonBuffer);

  outcome.id = result["id"];
  outcome.child = result["child"];
  outcome.amount = result["amount"];
  outcome.color = result["color"];
  outcome.solid = result["solid"];
  outcome.wet = result["wet"];
  outcome.time = result["time"].as<String>();
  result["notes"].as<String>().toCharArray(outcome.notes,256);
  deserialiseTags(result["tags"].as<JsonArray>()).toCharArray(outcome.tags, 256);

  return outcome;
}

bool BabyApi::removeDiaperChange(int id)
{
  String parameters = "/" + String(id) + "/";
  int responseCode;
  String jsonBuffer;
  jsonBuffer = httpRequest(CHANGES_ENDPOINT, "DELETE", parameters.c_str(), "", "", &responseCode);
  Serial.println(jsonBuffer);

  return responseCode == 204;
}

BabyApi::searchResults<BabyApi::Child> BabyApi::findChildren(
    int offset = -1,
    char * first_name = String(),
    char * last_name = String(),
    char * birth_date = String(),
    char * slug = String(),
    char * ordering = String())
{
  BabyApi::searchResults<BabyApi::Child> outcome;
  int count = 0;

  String query = "?limit=" + SEARCH_LIMIT +
                         (!offset > -1)
                     ? "&offset=" + String(offset)
                 : "" +
                         (!first_name.isEmpty())
                     ? "&first_name=" + first_name
                 : "" +
                         (!last_name.isEmpty())
                     ? "&last_name=" + last_name
                 : "" +
                         (!birth_date.isEmpty())
                     ? "&birth_date=" + birth_date
                 : "" +
                         (!slug.isEmpty())
                     ? "&slug=" + slug
                 : "" +
                         (!ordering.isEmpty())
                     ? "&ordering=" + ordering
                     : "";

  String jsonBuffer;
  jsonBuffer = httpRequest(CHILDREN_ENDPOINT, "GET", "", query.c_str());
  Serial.println(jsonBuffer);

  DynamicJsonDocument result = JsonParser(jsonBuffer);

  searchResultParser(result, &outcome.count, &outcome.next, &outcome.previous);

  JsonArray results = result["results"].as<JsonArray>();

  for (JsonObject childeRecord : results)
  {
    outcome.results[count].id = childeRecord["id"];
    outcome.results[count].first_name = childeRecord["first_name"].as<String>();
    outcome.results[count].last_name = childeRecord["last_name"].as<String>();
    outcome.results[count].birth_date = childeRecord["birth_date"].as<String>();
    outcome.results[count].picture = childeRecord["picture"].as<String>();
    count++;
  }

  return outcome;
}

BabyApi::Child BabyApi::newChild(
    char * first_name,
    char * last_name,
    char * birth_date,
    char * picture = String())
{
  DynamicJsonDocument doc(JOSN_CAPACITY);
  BabyApi::Child outcome;

  doc["first_name"] = first_name;
  doc["last_name"] = last_name;
  doc["birth_date"] = birth_date;
  doc["picture"] = picture;

  String requestBody = "";
  serializeJson(doc, requestBody);

  String jsonBuffer;
  jsonBuffer = httpRequest(CHILDREN_ENDPOINT, "POST", "", "", requestBody);
  Serial.println(jsonBuffer);

  DynamicJsonDocument result = JsonParser(jsonBuffer);

  outcome.id = result["id"];
  outcome.first_name = result["first_name"].as<String>();
  outcome.last_name = result["last_name"].as<String>();
  outcome.birth_date = result["birth_date"].as<String>();
  outcome.picture = result["picture"].as<String>();
  outcome.slug = result["slug"].as<String>();

  return outcome;
}

BabyApi::Child BabyApi::getChild(String slug)
{

  BabyApi::Child outcome;

  String parameters = "/" + slug + "/";

  String jsonBuffer;
  jsonBuffer = httpRequest(CHILDREN_ENDPOINT, "GET", parameters.c_str());
  Serial.println(jsonBuffer);

  DynamicJsonDocument result = JsonParser(jsonBuffer);

  outcome.id = result["id"];
  outcome.first_name = result["first_name"].as<String>();
  outcome.last_name = result["last_name"].as<String>();
  outcome.birth_date = result["birth_date"].as<String>();
  outcome.picture = result["picture"].as<String>();
  outcome.slug = result["slug"].as<String>();

  return outcome;
}

BabyApi::Child BabyApi::updateChild(
    char * slug,
    char * first_name = String(),
    char * last_name = String(),
    char * birth_date = String(),
    bool updatePicture = false,
    char * picture = String())
{
  DynamicJsonDocument doc(JOSN_CAPACITY);
  BabyApi::Child outcome;

  if (!first_name.isEmpty())
    doc["first_name"] = first_name;
  if (!last_name.isEmpty())
    doc["last_name"] = last_name;
  if (!birth_date.isEmpty())
    doc["birth_date"] = birth_date;
  if (updatePicture)
    doc["picture"] = picture;

  String requestBody = "";
  serializeJson(doc, requestBody);

  String parameters = "/" + slug + "/";

  String jsonBuffer;
  jsonBuffer = httpRequest(CHILDREN_ENDPOINT, "PATCH", parameters.c_str(), "", requestBody);
  Serial.println(jsonBuffer);

  DynamicJsonDocument result = JsonParser(jsonBuffer);

  outcome.id = result["id"];
  outcome.first_name = result["first_name"].as<String>();
  outcome.last_name = result["last_name"].as<String>();
  outcome.birth_date = result["birth_date"].as<String>();
  outcome.picture = result["picture"].as<String>();
  outcome.slug = result["slug"].as<String>();

  return outcome;
}
bool BabyApi::removeChild(String slug)
{
  String parameters = "/" + slug + "/";
  int responseCode;
  String jsonBuffer;
  jsonBuffer = httpRequest(CHILDREN_ENDPOINT, "DELETE", parameters.c_str(), "", "", &responseCode);
  Serial.println(jsonBuffer);

  return responseCode == 204;
}

BabyApi::searchResults<BabyApi::Feeding> BabyApi::findFeedingRecords(
    int offset = -1,
    int child = -1,
    char * start = String(),
    char * start_max = String(),
    char * start_min = String(),
    char * end = String(),
    char * end_max = String(),
    char * end_min = String(),
    char * type = String(),
    char * method = String(),
    char * tags = String(),
    char * ordering = String())
{
  BabyApi::searchResults<BabyApi::Feeding> outcome;
  int count = 0;

  String query = "?limit=" + SEARCH_LIMIT +
                         (offset > -1)
                     ? "&offset=" + String(offset)
                 : "" +
                         (child > -1)
                     ? "&child=" + String(child)
                 : "" +
                         (start.length() > 0)
                     ? "&start=" + start
                 : "" +
                         (start_max.length() > 0)
                     ? "&start_max=" + start_max
                 : "" +
                         (start_min.length() > 0)
                     ? "&date_min=" + start_min
                 : "" +
                         (end.length() > 0)
                     ? "&end=" + end
                 : "" +
                         (end_max.length() > 0)
                     ? "&end_max=" + end_max
                 : "" +
                         (end_min.length() > 0)
                     ? "&end_min=" + end_min
                 : "" +
                         (type.length() > 0)
                     ? "&type=" + type
                 : "" +
                         (method.length() > 0)
                     ? "&method=" + method
                 : "" +
                         (tags.length() > 0)
                     ? "&tags=" + tags
                 : "" +
                         (ordering.length() > 0)
                     ? "&ordering=" + ordering
                     : "";

  String jsonBuffer;
  jsonBuffer = httpRequest(FEEDINGS_ENDPOINT, "GET", "", query.c_str());
  Serial.println(jsonBuffer);

  DynamicJsonDocument result = JsonParser(jsonBuffer);

  searchResultParser(result, &outcome.count, &outcome.next, &outcome.previous);

  JsonArray results = result["results"].as<JsonArray>();

  for (JsonObject feedingRecord : results)
  {
    outcome.results[count].id = feedingRecord["id"];
    outcome.results[count].child = feedingRecord["child"].as<int>();
    outcome.results[count].amount = feedingRecord["amount"].as<float>();
    outcome.results[count].duration = feedingRecord["duration"].as<String>();
    outcome.results[count].end = feedingRecord["end"].as<String>();
    outcome.results[count].method = feedingRecord["method"].as<String>();
    outcome.results[count].notes = feedingRecord["notes"].as<String>();
    outcome.results[count].start = feedingRecord["start"].as<String>();
    outcome.results[count].type = feedingRecord["type"].as<String>();
    outcome.results[count].tags = deserialiseTags(feedingRecord["tags"].as<JsonArray>());

    count++;
  }

  return outcome;
}

BabyApi::Feeding BabyApi::logFeeding(
    int timer,
    FeedingType type,
    FeedingMethod method,
    float amount,
    char * notes = String(),
    char * tags[] = {})
{
  return logFeeding(
      -1,
      String(),
      String(),
      timer,
      type,
      method,
      amount,
      notes,
      tags);
}

BabyApi::Feeding BabyApi::logFeeding(
    int child,    // Required unless a Timer value is provided.
    char * start, // Required unless a Timer value is provided.
    char * end,   // Required unless a Timer value is provided.
    FeedingType type,
    FeedingMethod method,
    float amount = NAN,
    char * notes = String(),
    char * tags[] = {})
{
  return logFeeding(
      child,
      start,
      end,
      -1,
      type,
      method,
      amount,
      notes,
      tags);
}

BabyApi::Feeding BabyApi::logFeeding(
    int child = -1,          // Required unless a Timer value is provided.
    char * start = String(), // Required unless a Timer value is provided.
    char * end = String(),   // Required unless a Timer value is provided.
    int timer = -1,          // May be used in place of the Start, End, and/or Child values.
    FeedingType type = FeedingType::empty,
    FeedingMethod method = FeedingMethod::empty,
    float amount = NAN,
    char * notes = String(),
    char * tags[] = {})
{
  BabyApi::Feeding outcome;
  DynamicJsonDocument doc(JOSN_CAPACITY);

  // if no tmer value present, child start and end are required fields
  if (timer == -1 && (child == -1 || start.isEmpty() || end.isEmpty()))
  {
    Serial.println("Missing child, start and end, these are required if no timer id provided:");
    Serial.print("child: ");
    Serial.println(child);
    Serial.print("start: ");
    Serial.println(start);
    Serial.print("end: ");
    Serial.println(end);
    return outcome;
  }

  if (child > -1)
    doc["child"] = child;
  doc["start"] = start;
  doc["end"] = end;
  if (timer > -1)
    doc["timer"] = timer;
  doc["type"] = feedingTypes[type];
  doc["method"] = feedingMethods[method];
  if (!isnan(amount))
    doc["amount"] = amount;
  doc["notes"] = notes;
  serialiseTags(tags, doc["tags"]);

  String requestBody = "";
  serializeJson(doc, requestBody);

  String jsonBuffer;
  jsonBuffer = httpRequest(FEEDINGS_ENDPOINT, "POST", "", "", requestBody);
  Serial.println(jsonBuffer);

  DynamicJsonDocument result = JsonParser(jsonBuffer);

  outcome.id = result["id"];
  outcome.child = result["child"].as<int>();
  outcome.amount = result["amount"].as<float>();
  outcome.duration = result["duration"].as<String>();
  outcome.end = result["end"].as<String>();
  outcome.method = result["method"].as<String>();
  result["notes"].as<String>().toCharArray(outcome.notes,256);
  outcome.start = result["start"].as<String>();
  outcome.type = result["type"].as<String>();
  deserialiseTags(result["tags"].as<JsonArray>()).toCharArray(outcome.tags, 256);

  return outcome;
}

BabyApi::Feeding BabyApi::getFeeding(int id)
{
  BabyApi::Feeding outcome;

  String parameters = "/" + String(id) + "/";

  String jsonBuffer;
  jsonBuffer = httpRequest(FEEDINGS_ENDPOINT, "GET", parameters.c_str());
  Serial.println(jsonBuffer);

  DynamicJsonDocument result = JsonParser(jsonBuffer);

  outcome.id = result["id"];
  outcome.child = result["child"].as<int>();
  outcome.amount = result["amount"].as<float>();
  outcome.duration = result["duration"].as<String>();
  outcome.end = result["end"].as<String>();
  outcome.method = result["method"].as<String>();
  result["notes"].as<String>().toCharArray(outcome.notes,256);
  outcome.start = result["start"].as<String>();
  outcome.type = result["type"].as<String>();
  deserialiseTags(result["tags"].as<JsonArray>()).toCharArray(outcome.tags, 256);

  return outcome;
}

BabyApi::Feeding BabyApi::updateFeeding(
    int id,
    int child = -1,          // Required unless a Timer value is provided.
    char * start = String(), // Required unless a Timer value is provided.
    char * end = String(),   // Required unless a Timer value is provided.
    FeedingMethod method = FeedingMethod::null,
    FeedingType type = FeedingType::null,
    float amount = NAN,
    bool updateNotes = false,
    char * notes = String(),
    bool updateTags = false,
    char * tags = String())
{
  BabyApi::Feeding outcome;
  DynamicJsonDocument doc(JOSN_CAPACITY);

  if (child > -1)
    doc["child"] = child;
  if (!start.isEmpty())
    doc["start"] = start;
  if (!start.isEmpty())
    doc["end"] = end;
  if (type != FeedingType::null)
    doc["type"] = feedingTypes[type];
  if (method != FeedingMethod::null)
    doc["method"] = feedingMethods[method];
  if (!isnan(amount))
    doc["amount"] = amount;
  if (updateNotes)
    doc["notes"] = notes;
  if (updateTags)
    serialiseTags(tags, doc["tags"]);

  String requestBody = "";
  serializeJson(doc, requestBody);

  String parameters = "/" + String(id) + "/";

  String jsonBuffer;
  jsonBuffer = httpRequest(FEEDINGS_ENDPOINT, "PATCH", parameters.c_str(), "", requestBody);
  Serial.println(jsonBuffer);

  DynamicJsonDocument result = JsonParser(jsonBuffer);

  outcome.id = result["id"];
  outcome.child = result["child"].as<int>();
  outcome.amount = result["amount"].as<float>();
  outcome.duration = result["duration"].as<String>();
  outcome.end = result["end"].as<String>();
  outcome.method = result["method"].as<String>();
  result["notes"].as<String>().toCharArray(outcome.notes,256);
  outcome.start = result["start"].as<String>();
  outcome.type = result["type"].as<String>();
  deserialiseTags(result["tags"].as<JsonArray>()).toCharArray(outcome.tags, 256);

  return outcome;
}

bool BabyApi::removeFeeding(int id)
{
  String parameters = "/" + String(id) + "/";
  int responseCode;
  String jsonBuffer;
  jsonBuffer = httpRequest(CHANGES_ENDPOINT, "DELETE", parameters.c_str(), "", "", &responseCode);
  Serial.println(jsonBuffer);

  return responseCode == 204;
}

BabyApi::searchResults<BabyApi::HeadCircumference> BabyApi::findHeadCircumferenceRecords(
    int offset = -1,
    int child = -1,
    String date = String(),
    String ordering = String())
{
  BabyApi::searchResults<BabyApi::HeadCircumference> outcome;
  int count = 0;

  String query = "?limit=" + SEARCH_LIMIT +
                         (offset > -1)
                     ? "&offset=" + String(offset)
                 : "" +
                         (child > -1)
                     ? "&child=" + String(child)
                 : "" +
                         (date.length() > 0)
                     ? "&date=" + date
                 : "" +
                         (ordering.length() > 0)
                     ? "&ordering=" + ordering
                     : "";

  String jsonBuffer;
  jsonBuffer = httpRequest(HEAD_CIRCUMFERENCE_ENDPOINT, "GET", "", query.c_str());
  Serial.println(jsonBuffer);

  DynamicJsonDocument result = JsonParser(jsonBuffer);

  searchResultParser(result, &outcome.count, &outcome.next, &outcome.previous);

  JsonArray results = result["results"].as<JsonArray>();

  for (JsonObject headCircumferenceRecord : results)
  {
    outcome.results[count].id = headCircumferenceRecord["id"];
    outcome.results[count].child = headCircumferenceRecord["child"];
    outcome.results[count].date = headCircumferenceRecord["date"].as<String>();
    outcome.results[count].head_circumference = headCircumferenceRecord["head_circumference"];
    outcome.results[count].notes = headCircumferenceRecord["notes"].as<String>();
    outcome.results[count].tags = deserialiseTags(headCircumferenceRecord["tags"].as<JsonArray>());

    count++;
  }

  return outcome;
}

BabyApi::HeadCircumference BabyApi::logHeadCircumference(
    int child,
    float head_circumference,
    char * date,
    char * notes = "",
    char * tags[] = {})
{
  BabyApi::HeadCircumference outcome;

  DynamicJsonDocument doc(JOSN_CAPACITY);

  doc["child"] = child;
  doc["date"] = date;
  doc["head_circumference"] = head_circumference;
  doc["notes"] = notes;
  serialiseTags(tags, doc["tags"]);

  String requestBody = "";
  serializeJson(doc, requestBody);

  String jsonBuffer;
  jsonBuffer = httpRequest(HEAD_CIRCUMFERENCE_ENDPOINT, "POST", "", "", requestBody);
  Serial.println(jsonBuffer);

  DynamicJsonDocument result = JsonParser(jsonBuffer);

  outcome.id = result["id"];
  outcome.child = result["child"];
  result["date"].as<String>().toCharArray(outcome.date,33);
  outcome.head_circumference = result["head_circumference"];
  result["notes"].as<String>().toCharArray(outcome.notes,256);
  deserialiseTags(result["tags"].as<JsonArray>()).toCharArray(outcome.tags, 256);

  return outcome;
}

BabyApi::HeadCircumference BabyApi::getHeadCircumference(int id)
{
  BabyApi::HeadCircumference outcome;

  String parameters = "/" + String(id) + "/";

  String jsonBuffer;
  jsonBuffer = httpRequest(HEAD_CIRCUMFERENCE_ENDPOINT, "GET", parameters.c_str());
  Serial.println(jsonBuffer);

  DynamicJsonDocument result = JsonParser(jsonBuffer);

  outcome.id = result["id"];
  outcome.child = result["child"];
  result["date"].as<String>().toCharArray(outcome.date,33);
  outcome.head_circumference = result["head_circumference"];
  result["notes"].as<String>().toCharArray(outcome.notes,256);
  deserialiseTags(result["tags"].as<JsonArray>()).toCharArray(outcome.tags, 256);

  return outcome;
}

BabyApi::HeadCircumference BabyApi::updateHeadCircumference(
    int id,
    int child = -1,
    float head_circumference = NAN,
    char * date = String(),
    bool updateNotes = false,
    char * notes = String(),
    bool updateTags = false,
    char * tags = String())
{
  BabyApi::HeadCircumference outcome;

  DynamicJsonDocument doc(JOSN_CAPACITY);

  if (child > -1)
    doc["child"] = child;
  if (!date.isEmpty())
    doc["date"] = date;
  if (!isnan(head_circumference))
    doc["head_circumference"] = head_circumference;
  if (updateNotes)
    doc["notes"] = notes;
  if (updateTags)
    serialiseTags(tags, doc["tags"]);

  String requestBody = "";
  serializeJson(doc, requestBody);

  String parameters = "/" + String(id) + "/";

  String jsonBuffer;
  jsonBuffer = httpRequest(HEAD_CIRCUMFERENCE_ENDPOINT, "PATCH", parameters.c_str(), "", requestBody);
  Serial.println(jsonBuffer);

  DynamicJsonDocument result = JsonParser(jsonBuffer);

  outcome.id = result["id"];
  outcome.child = result["child"];
  result["date"].as<String>().toCharArray(outcome.date,33);
  outcome.head_circumference = result["head_circumference"];
  result["notes"].as<String>().toCharArray(outcome.notes,256);
  deserialiseTags(result["tags"].as<JsonArray>()).toCharArray(outcome.tags, 256);

  return outcome;
}

bool BabyApi::removeHeadCircumference(int id)
{
  String parameters = "/" + String(id) + "/";
  int responseCode;
  String jsonBuffer;
  jsonBuffer = httpRequest(CHANGES_ENDPOINT, "DELETE", parameters.c_str(), "", "", &responseCode);
  Serial.println(jsonBuffer);

  return responseCode == 204;
}

BabyApi::searchResults<BabyApi::Height> BabyApi::findHeightRecords(
    int offset = -1,
    int child = -1,
    char * date = String(),
    char * ordering = String())
{
  BabyApi::searchResults<BabyApi::Height> outcome;
  int count = 0;

  String query = "?limit=" + SEARCH_LIMIT +
                         (offset > -1)
                     ? "&offset=" + String(offset)
                 : "" +
                         (child > -1)
                     ? "&child=" + String(child)
                 : "" +
                         (date.length() > 0)
                     ? "&date=" + date
                 : "" +
                         (ordering.length() > 0)
                     ? "&ordering=" + ordering
                     : "";

  String jsonBuffer;
  jsonBuffer = httpRequest(HEIGHT_ENDPOINT, "GET", "", query.c_str());
  Serial.println(jsonBuffer);

  DynamicJsonDocument result = JsonParser(jsonBuffer);

  searchResultParser(result, &outcome.count, &outcome.next, &outcome.previous);

  JsonArray results = result["results"].as<JsonArray>();

  for (JsonObject heightRecord : results)
  {
    outcome.results[count].id = heightRecord["id"];
    outcome.results[count].child = heightRecord["child"];
    outcome.results[count].date = heightRecord["date"].as<String>();
    outcome.results[count].height = heightRecord["height"];
    outcome.results[count].notes = heightRecord["notes"].as<String>();
    outcome.results[count].tags = deserialiseTags(heightRecord["tags"].as<JsonArray>());

    count++;
  }

  return outcome;
}

BabyApi::Height BabyApi::logHeight(
    int child,
    float height,
    char * date,
    char * notes = "",
    char * tags = "")
{
  BabyApi::Height outcome;

  DynamicJsonDocument doc(JOSN_CAPACITY);

  doc["child"] = child;
  doc["date"] = date;
  doc["height"] = height;
  doc["notes"] = notes;
  serialiseTags(tags, doc["tags"]);

  String requestBody = "";
  serializeJson(doc, requestBody);

  String jsonBuffer;
  jsonBuffer = httpRequest(HEIGHT_ENDPOINT, "POST", "", "", requestBody);
  Serial.println(jsonBuffer);

  DynamicJsonDocument result = JsonParser(jsonBuffer);

  outcome.id = result["id"];
  outcome.child = result["child"];
  result["date"].as<String>().toCharArray(outcome.date,33);
  outcome.height = result["height"];
  result["notes"].as<String>().toCharArray(outcome.notes,256);
  deserialiseTags(result["tags"].as<JsonArray>()).toCharArray(outcome.tags, 256);

  return outcome;
}

BabyApi::Height BabyApi::getHeight(int id)
{
  BabyApi::Height outcome;

  String parameters = "/" + String(id) + "/";

  String jsonBuffer;
  jsonBuffer = httpRequest(HEIGHT_ENDPOINT, "GET", parameters.c_str());
  Serial.println(jsonBuffer);

  DynamicJsonDocument result = JsonParser(jsonBuffer);

  outcome.id = result["id"];
  outcome.child = result["child"];
  result["date"].as<String>().toCharArray(outcome.date,33);
  outcome.height = result["height"];
  result["notes"].as<String>().toCharArray(outcome.notes,256);
  deserialiseTags(result["tags"].as<JsonArray>()).toCharArray(outcome.tags, 256);

  return outcome;
}

BabyApi::Height BabyApi::updateHeight(
    int id,
    int child = -1,
    float height = NAN,
    String date = String(),
    bool updateNotes = false,
    String notes = String(),
    bool updateTags = false,
    String tags = String())
{
  Height outcome;

  DynamicJsonDocument doc(JOSN_CAPACITY);

  if (child > -1)
    doc["child"] = child;
  if (!date.isEmpty())
    doc["date"] = date;
  if (!isnan(height))
    doc["height"] = height;
  if (updateNotes)
    doc["notes"] = notes;
  if (updateTags)
    serialiseTags(tags, doc["tags"]);

  String requestBody = "";
  serializeJson(doc, requestBody);

  String parameters = "/" + String(id) + "/";

  String jsonBuffer;
  jsonBuffer = httpRequest(HEIGHT_ENDPOINT, "PATCH", parameters.c_str(), "", requestBody);
  Serial.println(jsonBuffer);

  DynamicJsonDocument result = JsonParser(jsonBuffer);

  outcome.id = result["id"];
  outcome.child = result["child"];
  result["date"].as<String>().toCharArray(outcome.date,33);
  outcome.height = result["height"];
  result["notes"].as<String>().toCharArray(outcome.notes,256);
  deserialiseTags(result["tags"].as<JsonArray>()).toCharArray(outcome.tags, 256);

  return outcome;
}

bool BabyApi::removeHeight(int id)
{
  String parameters = "/" + String(id) + "/";
  int responseCode;
  String jsonBuffer;
  jsonBuffer = httpRequest(CHANGES_ENDPOINT, "DELETE", parameters.c_str(), "", "", &responseCode);
  Serial.println(jsonBuffer);

  return responseCode == 204;
}

BabyApi::searchResults<BabyApi::Note> BabyApi::findNotes(
    int offset = -1,
    int child = -1,
    String date = String(),
    String date_max = String(),
    String date_min = String(),
    String tags = String(),
    String ordering = String())
{
  BabyApi::searchResults<BabyApi::Note> outcome;
  int count = 0;

  String query = "?limit=" + SEARCH_LIMIT +
                         (offset > -1)
                     ? "&offset=" + String(offset)
                 : "" +
                         (child > -1)
                     ? "&child=" + String(child)
                 : "" +
                         (date.length() > 0)
                     ? "&date=" + date
                 : "" +
                         (date_max.length() > 0)
                     ? "&date_max=" + date_max
                 : "" +
                         (date_min.length() > 0)
                     ? "&date_min=" + date_min
                 : "" +
                         (tags.length() > 0)
                     ? "&tags=" + tags
                 : "" +
                         (ordering.length() > 0)
                     ? "&ordering=" + ordering
                     : "";

  String jsonBuffer;
  jsonBuffer = httpRequest(NOTES_ENDPOINT, "GET", "", query.c_str());
  Serial.println(jsonBuffer);

  DynamicJsonDocument result = JsonParser(jsonBuffer);

  searchResultParser(result, &outcome.count, &outcome.next, &outcome.previous);

  JsonArray results = result["results"].as<JsonArray>();

  for (JsonObject noteRecord : results)
  {
    outcome.results[count].id = noteRecord["id"];
    outcome.results[count].child = noteRecord["child"];
    outcome.results[count].date = noteRecord["date"].as<String>();
    outcome.results[count].note = noteRecord["note"].as<String>();
    outcome.results[count].tags = deserialiseTags(noteRecord["tags"].as<JsonArray>());

    count++;
  }

  return outcome;
}

BabyApi::Note BabyApi::createNote(
    int child,
    String note,
    String date,
    String tags[] = {})
{
  BabyApi::Note outcome;

  DynamicJsonDocument doc(JOSN_CAPACITY);

  doc["child"] = child;
  doc["date"] = date;
  doc["note"] = note;
  serialiseTags(tags, doc["tags"]);

  String requestBody = "";
  serializeJson(doc, requestBody);

  String jsonBuffer;
  jsonBuffer = httpRequest(NOTES_ENDPOINT, "POST", "", "", requestBody);
  Serial.println(jsonBuffer);

  DynamicJsonDocument result = JsonParser(jsonBuffer);

  outcome.id = result["id"];
  outcome.child = result["child"];
  result["date"].as<String>().toCharArray(outcome.date,33);
  outcome.note = result["note"].as<String>();
  deserialiseTags(result["tags"].as<JsonArray>()).toCharArray(outcome.tags, 256);

  return outcome;
}

BabyApi::Note BabyApi::getNote(int id)
{
  BabyApi::Note outcome;

  String parameters = "/" + String(id) + "/";

  String jsonBuffer;
  jsonBuffer = httpRequest(NOTES_ENDPOINT, "GET", parameters.c_str());
  Serial.println(jsonBuffer);

  DynamicJsonDocument result = JsonParser(jsonBuffer);

  outcome.id = result["id"];
  outcome.child = result["child"];
  result["date"].as<String>().toCharArray(outcome.date,33);
  outcome.note = result["note"].as<String>();
  deserialiseTags(result["tags"].as<JsonArray>()).toCharArray(outcome.tags, 256);

  return outcome;
}

BabyApi::Note BabyApi::updateNote(
    int id,
    int child = -1,
    String date = String(),
    bool updateNote = false,
    String note = String(),
    bool updateTags = false,
    String tags = String())
{
  BabyApi::Note outcome;

  DynamicJsonDocument doc(JOSN_CAPACITY);

  if (child > -1)
    doc["child"] = child;
  if (!date.isEmpty())
    doc["date"] = date;
  if (updateNote)
    doc["note"] = note;
  if (updateTags)
    serialiseTags(tags, doc["tags"]);

  String requestBody = "";
  serializeJson(doc, requestBody);

  String parameters = "/" + String(id) + "/";

  String jsonBuffer;
  jsonBuffer = httpRequest(NOTES_ENDPOINT, "PATCH", parameters.c_str(), "", requestBody);
  Serial.println(jsonBuffer);

  DynamicJsonDocument result = JsonParser(jsonBuffer);

  outcome.id = result["id"];
  outcome.child = result["child"];
  result["date"].as<String>().toCharArray(outcome.date,33);
  outcome.note = result["note"].as<String>();
  deserialiseTags(result["tags"].as<JsonArray>()).toCharArray(outcome.tags, 256);

  return outcome;
}

bool BabyApi::removeNote(int id)
{
  String parameters = "/" + String(id) + "/";
  int responseCode;
  String jsonBuffer;
  jsonBuffer = httpRequest(CHANGES_ENDPOINT, "DELETE", parameters.c_str(), "", "", &responseCode);
  Serial.println(jsonBuffer);

  return responseCode == 204;
}

BabyApi::searchResults<BabyApi::Pumping> BabyApi::findPumpingRecords(
    int offset = -1,
    int child = -1,
    String date = String(),
    String date_max = String(),
    String date_min = String(),
    String ordering = String())
{
  BabyApi::searchResults<BabyApi::Pumping> outcome;
  int count = 0;

  String query = "?limit=" + SEARCH_LIMIT +
                         (offset > -1)
                     ? "&offset=" + String(offset)
                 : "" +
                         (child > -1)
                     ? "&child=" + String(child)
                 : "" +
                         (date.length() > 0)
                     ? "&date=" + date
                 : "" +
                         (date_max.length() > 0)
                     ? "&date_max=" + date_max
                 : "" +
                         (date_min.length() > 0)
                     ? "&date_min=" + date_min
                 : "" +
                         (ordering.length() > 0)
                     ? "&ordering=" + ordering
                     : "";

  String jsonBuffer;
  jsonBuffer = httpRequest(PUMPING_ENDPOINT, "GET", "", query.c_str());
  Serial.println(jsonBuffer);

  DynamicJsonDocument result = JsonParser(jsonBuffer);

  searchResultParser(result, &outcome.count, &outcome.next, &outcome.previous);

  JsonArray results = result["results"].as<JsonArray>();

  for (JsonObject pumpingRecord : results)
  {
    outcome.results[count].id = pumpingRecord["id"];
    outcome.results[count].child = pumpingRecord["child"];
    outcome.results[count].time = pumpingRecord["time"].as<String>();
    outcome.results[count].amount = pumpingRecord["amount"];
    outcome.results[count].notes = pumpingRecord["notes"].as<String>();
    outcome.results[count].tags = deserialiseTags(pumpingRecord["tags"].as<JsonArray>());

    count++;
  }

  return outcome;
}

BabyApi::Pumping BabyApi::logPumping(
    int child,
    float amount,
    String time = String(),
    String notes = String(),
    String tags[] = {})
{
  BabyApi::Pumping outcome;

  DynamicJsonDocument doc(JOSN_CAPACITY);

  doc["child"] = child;
  doc["time"] = time;
  doc["amount"] = amount;
  doc["notes"] = notes;
  serialiseTags(tags, doc["tags"]);

  String requestBody = "";
  serializeJson(doc, requestBody);

  String jsonBuffer;
  jsonBuffer = httpRequest(PUMPING_ENDPOINT, "POST", "", "", requestBody);
  Serial.println(jsonBuffer);

  DynamicJsonDocument result = JsonParser(jsonBuffer);

  outcome.id = result["id"];
  outcome.child = result["child"];
  outcome.time = result["time"].as<String>();
  outcome.amount = result["amount"];
  result["notes"].as<String>().toCharArray(outcome.notes,256);
  deserialiseTags(result["tags"].as<JsonArray>()).toCharArray(outcome.tags, 256);

  return outcome;
}

BabyApi::Pumping BabyApi::getPumping(int id)
{
  BabyApi::Pumping outcome;

  String parameters = "/" + String(id) + "/";

  String jsonBuffer;
  jsonBuffer = httpRequest(PUMPING_ENDPOINT, "GET", parameters.c_str());
  Serial.println(jsonBuffer);

  DynamicJsonDocument result = JsonParser(jsonBuffer);

  outcome.id = result["id"];
  outcome.child = result["child"];
  outcome.time = result["time"].as<String>();
  outcome.amount = result["amount"];
  result["notes"].as<String>().toCharArray(outcome.notes,256);
  deserialiseTags(result["tags"].as<JsonArray>()).toCharArray(outcome.tags, 256);

  return outcome;
}

BabyApi::Pumping BabyApi::updatePumping(
    int id,
    int child = -1,
    float amount = NAN,
    String time = String(),
    bool updateNotes = false,
    String notes = String(),
    bool updateTags = false,
    String tags = String())
{
  BabyApi::Pumping outcome;

  DynamicJsonDocument doc(JOSN_CAPACITY);

  if (child > -1)
    doc["child"] = child;
  if (!time.isEmpty())
    doc["time"] = time;
  if (!isnan(amount))
    doc["amount"] = amount;
  if (updateNotes)
    doc["notes"] = notes;
  if (updateTags)
    serialiseTags(tags, doc["tags"]);

  String requestBody = "";
  serializeJson(doc, requestBody);

  String parameters = "/" + String(id) + "/";

  String jsonBuffer;
  jsonBuffer = httpRequest(PUMPING_ENDPOINT, "PATCH", parameters.c_str(), "", requestBody);
  Serial.println(jsonBuffer);

  DynamicJsonDocument result = JsonParser(jsonBuffer);

  outcome.id = result["id"];
  outcome.child = result["child"];
  outcome.time = result["time"].as<String>();
  outcome.amount = result["amount"];
  result["notes"].as<String>().toCharArray(outcome.notes,256);
  deserialiseTags(result["tags"].as<JsonArray>()).toCharArray(outcome.tags, 256);

  return outcome;
}

bool BabyApi::removePumping(int id)
{
  String parameters = "/" + String(id) + "/";
  int responseCode;
  String jsonBuffer;
  jsonBuffer = httpRequest(CHANGES_ENDPOINT, "DELETE", parameters.c_str(), "", "", &responseCode);
  Serial.println(jsonBuffer);

  return responseCode == 204;
}

BabyApi::searchResults<BabyApi::Sleep> BabyApi::findSleepRecords(
    int offset = -1,
    int child = -1,
    String start = String(),
    String start_max = String(),
    String start_min = String(),
    String end = String(),
    String end_max = String(),
    String end_min = String(),
    String tags = String(),
    String ordering = String())
{
  BabyApi::searchResults<BabyApi::Sleep> outcome;
  int count = 0;

  String query = "?limit=" + SEARCH_LIMIT +
                         (offset > -1)
                     ? "&offset=" + String(offset)
                 : "" +
                         (child > -1)
                     ? "&child=" + String(child)
                 : "" +
                         (start.length() > 0)
                     ? "&start=" + start
                 : "" +
                         (start_max.length() > 0)
                     ? "&start_max=" + start_max
                 : "" +
                         (start_min.length() > 0)
                     ? "&date_min=" + start_min
                 : "" +
                         (end.length() > 0)
                     ? "&end=" + end
                 : "" +
                         (end_max.length() > 0)
                     ? "&end_max=" + end_max
                 : "" +
                         (end_min.length() > 0)
                     ? "&end_min=" + end_min
                 : "" +
                         (tags.length() > 0)
                     ? "&tags=" + tags
                 : "" +
                         (ordering.length() > 0)
                     ? "&ordering=" + ordering
                     : "";

  String jsonBuffer;
  jsonBuffer = httpRequest(SLEEP_ENDPOINT, "GET", "", query.c_str());
  Serial.println(jsonBuffer);

  DynamicJsonDocument result = JsonParser(jsonBuffer);

  searchResultParser(result, &outcome.count, &outcome.next, &outcome.previous);

  JsonArray results = result["results"].as<JsonArray>();

  for (JsonObject sleepingRecord : results)
  {
    outcome.results[count].id = sleepingRecord["id"];
    outcome.results[count].child = sleepingRecord["child"];
    outcome.results[count].duration = sleepingRecord["duration"].as<String>();
    outcome.results[count].end = sleepingRecord["end"].as<String>();
    outcome.results[count].nap = sleepingRecord["nap"].as<String>();
    outcome.results[count].start = sleepingRecord["start"].as<String>();
    outcome.results[count].notes = sleepingRecord["notes"].as<String>();
    outcome.results[count].tags = deserialiseTags(sleepingRecord["tags"].as<JsonArray>());

    count++;
  }

  return outcome;
}

BabyApi::Sleep BabyApi::logSleep(
    int child,          // Required unless a Timer value is provided.
    String start = String(), // Required unless a Timer value is provided.
    String end = String(),   // Required unless a Timer value is provided.
    int timer = -1,          // May be used in place of the Start, End, and/or Child values.
    String notes = String(),
    String tags[] = {})
{
  Sleep outcome;

  DynamicJsonDocument doc(JOSN_CAPACITY);

  // if no tmer value present, child start and end are required fields
  if (timer == -1 && (child == -1 || start.isEmpty() || end.isEmpty()))
  {
    Serial.println("Missing child, start and end, these are required if no timer id provided:");
    Serial.print("child: ");
    Serial.println(child);
    Serial.print("start: ");
    Serial.println(start);
    Serial.print("end: ");
    Serial.println(end);
    return outcome;
  }

  if (child > -1)
    doc["child"] = child;
  doc["start"] = start;
  doc["end"] = end;
  if (timer > -1)
    doc["timer"] = timer;
  doc["notes"] = notes;
  serialiseTags(tags, doc["tags"]);

  String requestBody = "";
  serializeJson(doc, requestBody);

  String jsonBuffer;
  jsonBuffer = httpRequest(SLEEP_ENDPOINT, "POST", "", "", requestBody);
  Serial.println(jsonBuffer);

  DynamicJsonDocument result = JsonParser(jsonBuffer);

  outcome.id = result["id"];
  outcome.child = result["child"];
  outcome.duration = result["duration"].as<String>();
  outcome.end = result["end"].as<String>();
  outcome.nap = result["nap"].as<String>();
  outcome.start = result["start"].as<String>();
  result["notes"].as<String>().toCharArray(outcome.notes,256);
  deserialiseTags(result["tags"].as<JsonArray>()).toCharArray(outcome.tags, 256);

  return outcome;
}

BabyApi::Sleep BabyApi::getSleep(int id)
{
  Sleep outcome;

  String parameters = "/" + String(id) + "/";

  String jsonBuffer;
  jsonBuffer = httpRequest(SLEEP_ENDPOINT, "GET", parameters.c_str());
  Serial.println(jsonBuffer);

  DynamicJsonDocument result = JsonParser(jsonBuffer);

  outcome.id = result["id"];
  outcome.child = result["child"];
  outcome.duration = result["duration"].as<String>();
  outcome.end = result["end"].as<String>();
  outcome.nap = result["nap"].as<String>();
  outcome.start = result["start"].as<String>();
  result["notes"].as<String>().toCharArray(outcome.notes,256);
    deserialiseTags(result["tags"].as<JsonArray>()).toCharArray(outcome.tags, 256);

  return outcome;
}

BabyApi::Sleep BabyApi::updateSleep(
    int id,
    int child = -1,
    String start = String(),
    String end = String(),
    bool updateNotes = false,
    String notes = String(),
    bool updateTags = false,
    String tags = String())
{
  Sleep outcome;

  DynamicJsonDocument doc(JOSN_CAPACITY);

  if (child > -1)
    doc["child"] = child;
  if (!start.isEmpty())
    doc["start"] = start;
  if (!end.isEmpty())
    doc["end"] = end;
  if (updateNotes)
    doc["notes"] = notes;
  if (updateTags)
    serialiseTags(tags, doc["tags"]);

  String requestBody = "";
  serializeJson(doc, requestBody);

  String parameters = "/" + String(id) + "/";

  String jsonBuffer;
  jsonBuffer = httpRequest(SLEEP_ENDPOINT, "PATCH", parameters.c_str(), "", requestBody);
  Serial.println(jsonBuffer);

  DynamicJsonDocument result = JsonParser(jsonBuffer);

  outcome.id = result["id"];
  outcome.child = result["child"];
  outcome.duration = result["duration"].as<String>();
  outcome.end = result["end"].as<String>();
  outcome.nap = result["nap"].as<String>();
  outcome.start = result["start"].as<String>();
  result["notes"].as<String>().toCharArray(outcome.notes,256);
    deserialiseTags(result["tags"].as<JsonArray>()).toCharArray(outcome.tags, 256);

  return outcome;
}

bool BabyApi::removeSleep(int id)
{
  String parameters = "/" + String(id) + "/";
  int responseCode;
  String jsonBuffer;
  jsonBuffer = httpRequest(CHANGES_ENDPOINT, "DELETE", parameters.c_str(), "", "", &responseCode);
  Serial.println(jsonBuffer);

  return responseCode == 204;
}

BabyApi::searchResults<BabyApi::Tag> BabyApi::findAllTags(
    int offset = -1,
    String name = String(),
    String last_used = String(),
    String ordering = String())
{
  BabyApi::searchResults<BabyApi::Tag> outcome;
  int count = 0;

  String query = "?limit=" + SEARCH_LIMIT +
                         (offset > -1)
                     ? "&offset=" + String(offset)
                 : "" +
                         (name.length() > 0)
                     ? "&name=" + name
                 : "" +
                         (last_used.length() > 0)
                     ? "&last_used=" + last_used
                 : "" +
                         (ordering.length() > 0)
                     ? "&ordering=" + ordering
                     : "";

  String jsonBuffer;
  jsonBuffer = httpRequest(TAGS_ENDPOINT, "GET", "", query.c_str());
  Serial.println(jsonBuffer);

  DynamicJsonDocument result = JsonParser(jsonBuffer);

  searchResultParser(result, &outcome.count, &outcome.next, &outcome.previous);

  JsonArray results = result["results"].as<JsonArray>();

  for (JsonObject tagRecord : results)
  {
    outcome.results[count].name = tagRecord["name"].as<String>();
    outcome.results[count].last_used = tagRecord["last_used"].as<String>();
    outcome.results[count].color = tagRecord["color"].as<String>();
    outcome.results[count].slug = tagRecord["slug"].as<String>();
    count++;
  }

  return outcome;
}

BabyApi::Tag BabyApi::createTag(
    String name,
    String colour = "")
{
  BabyApi::Tag outcome;

  DynamicJsonDocument doc(JOSN_CAPACITY);

  doc["name"] = name;
  doc["colour"] = colour;

  String requestBody = "";
  serializeJson(doc, requestBody);

  String jsonBuffer;
  jsonBuffer = httpRequest(TAGS_ENDPOINT, "POST", "", "", requestBody);
  Serial.println(jsonBuffer);

  DynamicJsonDocument result = JsonParser(jsonBuffer);

  outcome.name = result["name"].as<String>();
  outcome.last_used = result["last_used"].as<String>();
  outcome.color = result["color"].as<String>();
  outcome.slug = result["slug"].as<String>();

  return outcome;
}

BabyApi::Tag BabyApi::getTag(String slug)
{
  BabyApi::Tag outcome;

  String parameters = "/" + slug + "/";

  String jsonBuffer;
  jsonBuffer = httpRequest(TAGS_ENDPOINT, "GET", parameters.c_str());
  Serial.println(jsonBuffer);

  DynamicJsonDocument result = JsonParser(jsonBuffer);

  outcome.name = result["name"].as<String>();
  outcome.last_used = result["last_used"].as<String>();
  outcome.color = result["color"].as<String>();
  outcome.slug = result["slug"].as<String>();

  return outcome;
}

BabyApi::Tag BabyApi::updateTag(
    String slug,
    bool updateName = false,
    String name = String(),
    bool updateColour = false,
    String colour = String())
{
  BabyApi::Tag outcome;

  DynamicJsonDocument doc(JOSN_CAPACITY);

  if (updateName)
    doc["name"] = name;
  if (updateColour)
    doc["colour"] = colour;

  String requestBody = "";
  serializeJson(doc, requestBody);

  String parameters = "/" + slug + "/";

  String jsonBuffer;
  jsonBuffer = httpRequest(TAGS_ENDPOINT, "PATCH", parameters.c_str(), "", requestBody);
  Serial.println(jsonBuffer);

  DynamicJsonDocument result = JsonParser(jsonBuffer);

  outcome.name = result["name"].as<String>();
  outcome.last_used = result["last_used"].as<String>();
  outcome.color = result["color"].as<String>();
  outcome.slug = result["slug"].as<String>();

  return outcome;
}

bool BabyApi::removeTag(String slug)
{
  String parameters = "/" + slug + "/";
  int responseCode;
  String jsonBuffer;
  jsonBuffer = httpRequest(CHANGES_ENDPOINT, "DELETE", parameters.c_str(), "", "", &responseCode);
  Serial.println(jsonBuffer);

  return responseCode == 204;
}

BabyApi::searchResults<BabyApi::Temperature> BabyApi::findTemperatureRecords(
    int offset = -1,
    int child = -1,
    String date = String(),
    String date_max = String(),
    String date_min = String(),
    String tags = String(),
    String ordering = String())
{
  BabyApi::searchResults<BabyApi::Temperature> outcome;
  int count = 0;

  String query = "?limit=" + SEARCH_LIMIT +
                         (offset > -1)
                     ? "&offset=" + String(offset)
                 : "" +
                         (child > -1)
                     ? "&child=" + String(child)
                 : "" +
                         (date.length() > 0)
                     ? "&date=" + date
                 : "" +
                         (date_max.length() > 0)
                     ? "&date_max=" + date_max
                 : "" +
                         (date_min.length() > 0)
                     ? "&date_min=" + date_min
                 : "" +
                         (tags.length() > 0)
                     ? "&tags=" + tags
                 : "" +
                         (ordering.length() > 0)
                     ? "&ordering=" + ordering
                     : "";

  String jsonBuffer;
  jsonBuffer = httpRequest(TEMPERATURE_ENDPOINT, "GET", "", query.c_str());
  Serial.println(jsonBuffer);

  DynamicJsonDocument result = JsonParser(jsonBuffer);

  searchResultParser(result, &outcome.count, &outcome.next, &outcome.previous);

  JsonArray results = result["results"].as<JsonArray>();

  for (JsonObject temperatureRecord : results)
  {
    outcome.results[count].id = temperatureRecord["id"];
    outcome.results[count].child = temperatureRecord["child"];
    outcome.results[count].time = temperatureRecord["time"].as<String>();
    outcome.results[count].temperature = temperatureRecord["temperature"];
    outcome.results[count].notes = temperatureRecord["notes"].as<String>();
    outcome.results[count].tags = deserialiseTags(temperatureRecord["tags"].as<JsonArray>());

    count++;
  }

  return outcome;
}

BabyApi::Temperature BabyApi::logTemperature(
    int child,
    float temperature,
    String time,
    String notes = String(),
    String tags[] = {})
{
  BabyApi::Temperature outcome;

  DynamicJsonDocument doc(JOSN_CAPACITY);

  doc["child"] = child;
  doc["time"] = time;
  doc["temperature"] = temperature;
  doc["notes"] = notes;
  serialiseTags(tags, doc["tags"]);

  String requestBody = "";
  serializeJson(doc, requestBody);

  String jsonBuffer;
  jsonBuffer = httpRequest(TEMPERATURE_ENDPOINT, "POST", "", "", requestBody);
  Serial.println(jsonBuffer);

  DynamicJsonDocument result = JsonParser(jsonBuffer);

  outcome.id = result["id"];
  outcome.child = result["child"];
  outcome.time = result["time"].as<String>();
  outcome.temperature = result["temperature"];
  result["notes"].as<String>().toCharArray(outcome.notes,256);
    deserialiseTags(result["tags"].as<JsonArray>()).toCharArray(outcome.tags, 256);

  return outcome;
}

BabyApi::Temperature BabyApi::getTemperature(int id)
{
  BabyApi::Temperature outcome;

  String parameters = "/" + String(id) + "/";

  String jsonBuffer;
  jsonBuffer = httpRequest(TEMPERATURE_ENDPOINT, "GET", parameters.c_str());
  Serial.println(jsonBuffer);

  DynamicJsonDocument result = JsonParser(jsonBuffer);

  outcome.id = result["id"];
  outcome.child = result["child"];
  outcome.time = result["time"].as<String>();
  outcome.temperature = result["temperature"];
  result["notes"].as<String>().toCharArray(outcome.notes,256);
    deserialiseTags(result["tags"].as<JsonArray>()).toCharArray(outcome.tags, 256);

  return outcome;
}

BabyApi::Temperature BabyApi::updateTemperature(
    int id = -1,
    int child = -1,
    float temperature = NAN,
    String time = "",
    bool updateNotes = false,
    String notes = String(),
    bool updateTags = false,
    String tags = String())
{
  BabyApi::Temperature outcome;

  DynamicJsonDocument doc(JOSN_CAPACITY);

  if (child > -1)
    doc["child"] = child;
  if (!time.isEmpty())
    doc["time"] = time;
  if (!isnan(temperature))
    doc["temperature"] = temperature;
  if (updateNotes)
    doc["notes"] = notes;
  if (updateTags)
    serialiseTags(tags, doc["tags"]);

  String requestBody = "";
  serializeJson(doc, requestBody);

  String parameters = "/" + String(id) + "/";

  String jsonBuffer;
  jsonBuffer = httpRequest(TEMPERATURE_ENDPOINT, "PATCH", parameters.c_str(), "", requestBody);
  Serial.println(jsonBuffer);

  DynamicJsonDocument result = JsonParser(jsonBuffer);

  outcome.id = result["id"];
  outcome.child = result["child"];
  outcome.time = result["time"].as<String>();
  outcome.temperature = result["temperature"];
  result["notes"].as<String>().toCharArray(outcome.notes,256);
    deserialiseTags(result["tags"].as<JsonArray>()).toCharArray(outcome.tags, 256);

  return outcome;
}

bool BabyApi::removeTemperature(int id)
{
  String parameters = "/" + String(id) + "/";
  int responseCode;
  String jsonBuffer;
  jsonBuffer = httpRequest(CHANGES_ENDPOINT, "DELETE", parameters.c_str(), "", "", &responseCode);
  Serial.println(jsonBuffer);

  return responseCode == 204;
}

BabyApi::searchResults<BabyApi::Timer> BabyApi::findTimers(
    int offset = -1,
    int child = -1,
    String start = String(),
    String start_max = String(),
    String start_min = String(),
    String end = String(),
    String end_max = String(),
    String end_min = String(),
    String active = String(),
    int user = -1,
    String ordering = String())
{
  BabyApi::searchResults<BabyApi::Timer> outcome;
  int count = 0;

  String query = "?limit=" + SEARCH_LIMIT +
                         (offset > -1)
                     ? "&offset=" + String(offset)
                 : "" +
                         (child > -1)
                     ? "&child=" + String(child)
                 : "" +
                         (start.length() > 0)
                     ? "&start=" + start
                 : "" +
                         (start_max.length() > 0)
                     ? "&start_max=" + start_max
                 : "" +
                         (start_min.length() > 0)
                     ? "&date_min=" + start_min
                 : "" +
                         (end.length() > 0)
                     ? "&end=" + end
                 : "" +
                         (end_max.length() > 0)
                     ? "&end_max=" + end_max
                 : "" +
                         (end_min.length() > 0)
                     ? "&end_min=" + end_min
                 : "" +
                         (active.length())
                     ? "&active=" + String(active)
                 : "" +
                         (user > -1)
                     ? "&user=" + String(user)
                 : "" +
                         (ordering.length() > 0)
                     ? "&ordering=" + ordering
                     : "";

  String jsonBuffer;
  jsonBuffer = httpRequest(TIMERS_ENDPOINT, "GET", "", query.c_str());
  Serial.println(jsonBuffer);

  DynamicJsonDocument result = JsonParser(jsonBuffer);

  searchResultParser(result, &outcome.count, &outcome.next, &outcome.previous);

  JsonArray results = result["results"].as<JsonArray>();

  for (JsonObject timerRecord : results)
  {
    outcome.results[count].id = timerRecord["id"];
    outcome.results[count].child = timerRecord["child"];
    outcome.results[count].duration = timerRecord["duration"].as<String>();
    outcome.results[count].end = timerRecord["end"].as<String>();
    outcome.results[count].user = timerRecord["user"];
    outcome.results[count].start = timerRecord["start"].as<String>();
    count++;
  }

  return outcome;
}

int BabyApi::startTimer(int childId, String name = "", int timer = -1)
{
  BabyApi::Timer babyTimer;

  if (timer == -1 && !name.isEmpty())
  {
    // search for existing timer
    BabyApi::searchResults<BabyApi::Timer> results;

    do
    {
      results = findTimers(0, childId);

      for (BabyApi::Timer result : results.results)
      {
        if (result.name == name)
        {
          // timer found
          timer = result.id;
          break;
        }
      }
    } while (results.next > -1 && timer == -1);
  }

  if (babyTimer.id > -1)
  {
    // attempt to restart it
    babyTimer = restartTimer(timer);
  }
  else
  {
    // if we make it here, no valid timer could be started. need to create one
    babyTimer = createTimer(childId, name);
  }

  return babyTimer.id;
}

BabyApi::Timer BabyApi::createTimer(
    int child)
{
  return createTimer(child, String());
}

BabyApi::Timer BabyApi::createTimer(
    int child,
    String name)
{
  return createTimer(
      child,
      name,
      String());
}

BabyApi::Timer BabyApi::createTimer(
    int child,
    String start)
{
  return createTimer(
      child,
      String(),
      start);
}

BabyApi::Timer BabyApi::createTimer(
    int child,
    String name,
    String start)
{
  BabyApi::Timer outcome;

  DynamicJsonDocument doc(JOSN_CAPACITY);

  doc["child"] = child;
  doc["name"] = name;
  doc["start"] = start;

  String requestBody = "";
  serializeJson(doc, requestBody);

  String jsonBuffer;
  jsonBuffer = httpRequest(TIMERS_ENDPOINT, "POST", "", "", requestBody);
  Serial.println(jsonBuffer);

  DynamicJsonDocument result = JsonParser(jsonBuffer);

  outcome.id = result["id"];
  outcome.child = result["child"];
  outcome.duration = result["duration"].as<String>();
  outcome.end = result["end"].as<String>();
  outcome.user = result["user"];
  outcome.start = result["start"].as<String>();

  return outcome;
}

BabyApi::Timer BabyApi::getTimer(int id)
{
  BabyApi::Timer outcome;

  String parameters = "/" + String(id) + "/";

  String jsonBuffer;
  jsonBuffer = httpRequest(TIMERS_ENDPOINT, "GET", parameters.c_str());
  Serial.println(jsonBuffer);

  DynamicJsonDocument result = JsonParser(jsonBuffer);

  outcome.id = result["id"];
  outcome.child = result["child"];
  outcome.duration = result["duration"].as<String>();
  outcome.end = result["end"].as<String>();
  outcome.user = result["user"];
  outcome.start = result["start"].as<String>();

  return outcome;
}

BabyApi::Timer BabyApi::updateTimer(
    int id,
    int child = -1,
    String name = String(),
    String start = String(),
    int user = -1)
{
  BabyApi::Timer outcome;

  DynamicJsonDocument doc(JOSN_CAPACITY);

  if (child > -1)
    doc["child"] = child;
  if (!name.isEmpty())
    doc["name"] = name;
  if (!start.isEmpty())
    doc["start"] = start;
  if (child > -1)
    doc["user"] = user;

  String requestBody = "";
  serializeJson(doc, requestBody);

  String parameters = "/" + String(id) + "/";

  String jsonBuffer;
  jsonBuffer = httpRequest(TIMERS_ENDPOINT, "PATCH", parameters.c_str(), "", requestBody);
  Serial.println(jsonBuffer);

  DynamicJsonDocument result = JsonParser(jsonBuffer);

  outcome.id = result["id"];
  outcome.child = result["child"];
  outcome.duration = result["duration"].as<String>();
  outcome.end = result["end"].as<String>();
  outcome.user = result["user"];
  outcome.start = result["start"].as<String>();

  return outcome;
}

bool BabyApi::removeTimer(int id)
{
  String parameters = "/" + String(id) + "/";
  int responseCode;
  String jsonBuffer;
  jsonBuffer = httpRequest(CHANGES_ENDPOINT, "DELETE", parameters.c_str(), "", "", &responseCode);
  Serial.println(jsonBuffer);

  return responseCode == 204;
}

BabyApi::Timer BabyApi::restartTimer(int id)
{
  BabyApi::Timer outcome;

  DynamicJsonDocument doc(JOSN_CAPACITY);

  String parameters = "/" + String(id) + "/";

  String jsonBuffer;
  jsonBuffer = httpRequest(TIMERS_ENDPOINT, "PATCH", parameters.c_str(), "");
  Serial.println(jsonBuffer);

  DynamicJsonDocument result = JsonParser(jsonBuffer);

  outcome.id = result["id"];
  outcome.child = result["child"];
  outcome.duration = result["duration"].as<String>();
  outcome.end = result["end"].as<String>();
  outcome.user = result["user"];
  outcome.start = result["start"].as<String>();

  return outcome;
}

BabyApi::Timer BabyApi::stopTimer(int id)
{
  BabyApi::Timer outcome;

  DynamicJsonDocument doc(JOSN_CAPACITY);

  String parameters = "/" + String(id) + "/";

  String jsonBuffer;
  jsonBuffer = httpRequest(TIMERS_ENDPOINT, "PATCH", parameters.c_str(), "");
  Serial.println(jsonBuffer);

  DynamicJsonDocument result = JsonParser(jsonBuffer);

  outcome.id = result["id"];
  outcome.child = result["child"];
  outcome.duration = result["duration"].as<String>();
  outcome.end = result["end"].as<String>();
  outcome.user = result["user"];
  outcome.start = result["start"].as<String>();

  return outcome;
}

BabyApi::searchResults<BabyApi::TummyTime> BabyApi::findTummyTimes(
    int offset = -1,
    int child = -1,
    String start = String(),
    String start_max = String(),
    String start_min = String(),
    String end = String(),
    String end_max = String(),
    String end_min = String(),
    String tags = String(),
    String ordering = String())
{
  BabyApi::searchResults<BabyApi::TummyTime> outcome;
  int count = 0;

  String query = "?limit=" + SEARCH_LIMIT +
                         (offset > -1)
                     ? "&offset=" + String(offset)
                 : "" +
                         (child > -1)
                     ? "&child=" + String(child)
                 : "" +
                         (start.length() > 0)
                     ? "&start=" + start
                 : "" +
                         (start_max.length() > 0)
                     ? "&start_max=" + start_max
                 : "" +
                         (start_min.length() > 0)
                     ? "&date_min=" + start_min
                 : "" +
                         (end.length() > 0)
                     ? "&end=" + end
                 : "" +
                         (end_max.length() > 0)
                     ? "&end_max=" + end_max
                 : "" +
                         (end_min.length() > 0)
                     ? "&end_min=" + end_min
                 : "" +
                         (tags.length() > 0)
                     ? "&tags=" + tags
                 : "" +
                         (ordering.length() > 0)
                     ? "&ordering=" + ordering
                     : "";

  String jsonBuffer;
  jsonBuffer = httpRequest(TUMMY_TIMES_ENDPOINT, "GET", "", query.c_str());
  Serial.println(jsonBuffer);

  DynamicJsonDocument result = JsonParser(jsonBuffer);

  searchResultParser(result, &outcome.count, &outcome.next, &outcome.previous);

  JsonArray results = result["results"].as<JsonArray>();

  for (JsonObject tummyTimeRecord : results)
  {
    outcome.results[count].id = tummyTimeRecord["id"];
    outcome.results[count].child = tummyTimeRecord["child"];
    outcome.results[count].duration = tummyTimeRecord["duration"].as<String>();
    outcome.results[count].end = tummyTimeRecord["end"].as<String>();
    outcome.results[count].milestone = tummyTimeRecord["milestone"].as<String>();
    outcome.results[count].start = tummyTimeRecord["start"].as<String>();
    outcome.results[count].tags = deserialiseTags(tummyTimeRecord["tags"].as<JsonArray>());

    count++;
  }

  return outcome;
}

BabyApi::TummyTime BabyApi::logTummyTime(
    int child = -1,          // Required unless a Timer value is provided.
    String start = String(), // Required unless a Timer value is provided.
    String end = String(),   // Required unless a Timer value is provided.
    int timer = -1,          // May be used in place of the Start, End, and/or Child values.
    String milestone = String(),
    String tags[] = {})
{
  BabyApi::TummyTime outcome;

  DynamicJsonDocument doc(JOSN_CAPACITY);

  // if no tmer value present, child start and end are required fields
  if (timer == -1 && (child == -1 || start.isEmpty() || end.isEmpty()))
  {
    Serial.println("Missing child, start and end, these are required if no timer id provided:");
    Serial.print("child: ");
    Serial.println(child);
    Serial.print("start: ");
    Serial.println(start);
    Serial.print("end: ");
    Serial.println(end);
    return outcome;
  }

  if (child > -1)
    doc["child"] = child;
  doc["start"] = start;
  doc["end"] = end;
  if (timer > -1)
    doc["timer"] = timer;
  doc["milestone"] = milestone;
  serialiseTags(tags, doc["tags"]);

  String requestBody = "";
  serializeJson(doc, requestBody);

  String jsonBuffer;
  jsonBuffer = httpRequest(TUMMY_TIMES_ENDPOINT, "POST", "", "", requestBody);
  Serial.println(jsonBuffer);

  DynamicJsonDocument result = JsonParser(jsonBuffer);

  outcome.id = result["id"];
  outcome.child = result["child"];
  outcome.duration = result["duration"].as<String>();
  outcome.end = result["end"].as<String>();
  outcome.milestone = result["milestone"].as<String>();
  outcome.start = result["start"].as<String>();
    deserialiseTags(result["tags"].as<JsonArray>()).toCharArray(outcome.tags, 256);

  return outcome;
}

BabyApi::TummyTime BabyApi::getTummyTime(int id)
{
  BabyApi::TummyTime outcome;

  String parameters = "/" + String(id) + "/";

  String jsonBuffer;
  jsonBuffer = httpRequest(TUMMY_TIMES_ENDPOINT, "GET", parameters.c_str());
  Serial.println(jsonBuffer);

  DynamicJsonDocument result = JsonParser(jsonBuffer);

  outcome.id = result["id"];
  outcome.child = result["child"];
  outcome.duration = result["duration"].as<String>();
  outcome.end = result["end"].as<String>();
  outcome.milestone = result["milestone"].as<String>();
  outcome.start = result["start"].as<String>();
    deserialiseTags(result["tags"].as<JsonArray>()).toCharArray(outcome.tags, 256);

  return outcome;
}

BabyApi::TummyTime BabyApi::updateTummyTime(
    int id,
    int child = -1,
    String start = String(),
    String end = String(),
    bool updateMilestone = false,
    String milestone = String(),
    bool updateTags = false,
    String tags = String())
{
  BabyApi::TummyTime outcome;

  DynamicJsonDocument doc(JOSN_CAPACITY);

  if (child > -1)
    doc["child"] = child;
  if (!start.isEmpty())
    doc["start"] = start;
  if (!start.isEmpty())
    doc["end"] = end;
  if (updateMilestone)
    doc["milestone"] = milestone;
  if (updateTags)
    serialiseTags(tags, doc["tags"]);

  String requestBody = "";
  serializeJson(doc, requestBody);

  String parameters = "/" + String(id) + "/";

  String jsonBuffer;
  jsonBuffer = httpRequest(TUMMY_TIMES_ENDPOINT, "PATCH", parameters.c_str(), "", requestBody);
  Serial.println(jsonBuffer);

  DynamicJsonDocument result = JsonParser(jsonBuffer);

  outcome.id = result["id"];
  outcome.child = result["child"];
  outcome.duration = result["duration"].as<String>();
  outcome.end = result["end"].as<String>();
  outcome.milestone = result["milestone"].as<String>();
  outcome.start = result["start"].as<String>();
    deserialiseTags(result["tags"].as<JsonArray>()).toCharArray(outcome.tags, 256);

  return outcome;
}

bool BabyApi::removeTummyTime(int id)
{
  String parameters = "/" + String(id) + "/";
  int responseCode;
  String jsonBuffer;
  jsonBuffer = httpRequest(CHANGES_ENDPOINT, "DELETE", parameters.c_str(), "", "", &responseCode);
  Serial.println(jsonBuffer);

  return responseCode == 204;
}

BabyApi::searchResults<BabyApi::Weight> BabyApi::findWeightRecords(
    int offset = -1,
    int child = -1,
    String date = String(),
    String ordering = String())
{
  BabyApi::searchResults<BabyApi::Weight> outcome;
  int count = 0;

  String query = "?limit=" + SEARCH_LIMIT +
                         (offset > -1)
                     ? "&offset=" + String(offset)
                 : "" +
                         (child > -1)
                     ? "&child=" + String(child)
                 : "" +
                         (date.length() > 0)
                     ? "&date=" + date
                 : "" +
                         (ordering.length() > 0)
                     ? "&ordering=" + ordering
                     : "";

  String jsonBuffer;
  jsonBuffer = httpRequest(WEIGHT_ENDPOINT, "GET", "", query.c_str());
  Serial.println(jsonBuffer);

  DynamicJsonDocument result = JsonParser(jsonBuffer);

  searchResultParser(result, &outcome.count, &outcome.next, &outcome.previous);

  JsonArray results = result["results"].as<JsonArray>();

  for (JsonObject weightRecord : results)
  {
    outcome.results[count].id = weightRecord["id"];
    outcome.results[count].child = weightRecord["child"];
    outcome.results[count].date = weightRecord["date"].as<String>();
    outcome.results[count].weight = weightRecord["weight"];
    outcome.results[count].notes = weightRecord["notes"].as<String>();
    outcome.results[count].tags = deserialiseTags(weightRecord["tags"].as<JsonArray>());

    count++;
  }

  return outcome;
}

BabyApi::Weight BabyApi::logWeight(
    int child,
    float weight,
    String date,
    String notes = "",
    String tags[] = {})
{
  BabyApi::Weight outcome;

  DynamicJsonDocument doc(JOSN_CAPACITY);

  doc["child"] = child;
  doc["date"] = date;
  doc["weight"] = weight;
  doc["notes"] = notes;
  serialiseTags(tags, doc["tags"]);

  String requestBody = "";
  serializeJson(doc, requestBody);

  String jsonBuffer;
  jsonBuffer = httpRequest(WEIGHT_ENDPOINT, "POST", "", "", requestBody);
  Serial.println(jsonBuffer);

  DynamicJsonDocument result = JsonParser(jsonBuffer);

  outcome.id = result["id"];
  outcome.child = result["child"];
  result["date"].as<String>().toCharArray(outcome.date,33);
  outcome.weight = result["weight"];
  result["notes"].as<String>().toCharArray(outcome.notes,256);
    deserialiseTags(result["tags"].as<JsonArray>()).toCharArray(outcome.tags, 256);

  return outcome;
}

BabyApi::Weight BabyApi::getWeight(int id)
{
  BabyApi::Weight outcome;

  String parameters = "/" + String(id) + "/";

  String jsonBuffer;
  jsonBuffer = httpRequest(WEIGHT_ENDPOINT, "GET", parameters.c_str());
  Serial.println(jsonBuffer);

  DynamicJsonDocument result = JsonParser(jsonBuffer);

  outcome.id = result["id"];
  outcome.child = result["child"];
  result["date"].as<String>().toCharArray(outcome.date,33);
  outcome.weight = result["weight"];
  result["notes"].as<String>().toCharArray(outcome.notes,256);
    deserialiseTags(result["tags"].as<JsonArray>()).toCharArray(outcome.tags, 256);

  return outcome;
}

BabyApi::Weight BabyApi::updateWeight(
    int id,
    int child = -1,
    float weight = NAN,
    String date = String(),
    bool updateNotes = false,
    String notes = String(),
    bool updateTags = false,
    String tags = String())
{
  BabyApi::Weight outcome;

  DynamicJsonDocument doc(JOSN_CAPACITY);

  if (child > -1)
    doc["child"] = child;
  if (!date.isEmpty())
    doc["date"] = date;
  if (!isnan(weight))
    doc["weight"] = weight;
  if (updateNotes)
    doc["notes"] = notes;
  if (updateTags)
    doc["tags"] = serialiseTags(tags);

  String requestBody = "";
  serializeJson(doc, requestBody);

  String parameters = "/" + String(id) + "/";

  String jsonBuffer;
  jsonBuffer = httpRequest(WEIGHT_ENDPOINT, "PATCH", parameters.c_str(), "", requestBody);
  Serial.println(jsonBuffer);

  DynamicJsonDocument result = JsonParser(jsonBuffer);

  outcome.id = result["id"];
  outcome.child = result["child"];
  result["date"].as<String>().toCharArray(outcome.date,33);
  outcome.weight = result["weight"];
  result["notes"].as<String>().toCharArray(outcome.notes,256);
  deserialiseTags(result["tags"].as<JsonArray>()).toCharArray(outcome.tags, 256);

  return outcome;
}

bool BabyApi::removeWeight(int id)
{
  String parameters = "/" + String(id) + "/";
  int responseCode;
  String jsonBuffer;
  jsonBuffer = httpRequest(CHANGES_ENDPOINT, "DELETE", parameters.c_str(), "", "", &responseCode);
  Serial.println(jsonBuffer);

  return responseCode == 204;
}

BabyApi::Profile BabyApi::getProfile()
{
  BabyApi::Profile outcome;

  String jsonBuffer;
  jsonBuffer = httpRequest(PROFILE_ENDPOINT, "GET");
  Serial.println(jsonBuffer);

  DynamicJsonDocument result = JsonParser(jsonBuffer);

  outcome.user.id = result["user"]["id"];
  outcome.user.username = result["user"]["username"].as<String>();
  outcome.user.first_name = result["user"]["first_name"].as<String>();
  outcome.user.last_name = result["user"]["last_name"].as<String>();
  outcome.user.email = result["user"]["email"].as<String>();
  outcome.user.is_staff = result["user"]["is_staff"];
  outcome.language = result["language"].as<String>();
  outcome.timezone = result["timezone"].as<String>();
  outcome.api_key = result["api_key"].as<String>();

  return outcome;
}

uint8_t BabyApi::getAllChildren(BabyApi::Child *children, uint8_t count)
{
  uint8_t retrieved = 0;
  uint8_t i = 0;
  searchResults<Child> results;
  do
  {
    results = findChildren(retrieved);

    for (i = 0; (i < SEARCH_LIMIT) && (retrieved + i < count); i++)
    {
      children[retrieved + i] = results.results[i];
    }

    retrieved += results.count;
  } while (results.next > 0);

  return retrieved;
}

uint8_t BabyApi::recordFeeding(uint16_t timerId, BabyApi::FeedingType feedingType, BabyApi::FeedingMethod feedingMethod, float amount)
{
  BabyApi::Feeding fed;

  fed = babyApi.logFeeding(timerId, feedingType, feedingMethod, amount); 
  
  return fed.id;
}

uint8_t BabyApi::recordSleep(uint16_t timerId)
{
  BabyApi::Sleep slept;

  slept = babyApi.logSleep(
      -1,
      String(),
      String(),
      timerId);

  return slept.id;
}

uint8_t BabyApi::recordPumping(uint16_t timerId, float amount)
{
  BabyApi::Pumping pumped;

  pumped = babyApi.logPumping(timerId, amount);

  return pumped.id;
}

uint8_t BabyApi::recordTummyTime(uint16_t timerId)
{
  BabyApi::TummyTime tummy;

  tummy = babyApi.logTummyTime(
      -1,
      String(),
      String(),
      (int)timerId);

  return tummy.id;
}

uint8_t BabyApi::recordNappyChange(uint16_t child, bool wet, bool solid, uint16_t colour)
{
  BabyApi::DiaperChange changed;

  changed = babyApi.logDiaperChange(child,wet,solid,(BabyApi::StoolColour)colour);

  return changed.id;
}
