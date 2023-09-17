
#include "BabyAPI.h"


BabyApi::BabyApi(const char * baby_api_key)
{
  setServerHost(DEFAULT_SERVER_HOST);
  setServerPort(DEFAULT_SERVER_PORT);
  setApiKey(baby_api_key);
}

BabyApi::BabyApi(const char * server_host, const char * baby_api_key)
{
  setServerHost(server_host);
  setServerPort(DEFAULT_SERVER_PORT);
  setApiKey(baby_api_key);
}

BabyApi::BabyApi(const char * server_host, const char * server_port, const char * baby_api_key)
{
  setServerHost(server_host);
  setServerPort(server_port);
  setApiKey(baby_api_key);
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
  strcpy(serverHost, server_host);
}

void BabyApi::setServerPort(const char * server_port)
{
  strcpy(serverPort, server_port);

}
void BabyApi::setApiKey(const char * apiKey)
{
  strcpy(babyApiKey, "Token ");
  strcat(babyApiKey, apiKey);
}

int BabyApi::httpRequest(
  const char * endpoint, 
  const char * type, 
  const char * parameters = {}, 
  const char * query = {}, 
  const char * requestBody = {})
{
  WiFiClientSecure client;
  HTTPClient https;

  char address[255] = "https://";

  strcat(address, getServerHost());
  strcat(address, ":");
  strcat(address, getServerPort());
  strcat(address, ENDPOINT);
  strcat(address, endpoint);
  strcat(address, "/");
  strcat(address, parameters);
  strcat(address, query);

  https.addHeader("Authorization", babyApiKey);

  https.begin(client, address);

  // Send HTTP POST request
  int httpsResponseCode = https.sendRequest(type, requestBody);

  if (httpsResponseCode > 0)
  {
    Serial.print("HTTP Response code: ");
    Serial.println(httpsResponseCode);
    ResponseParser(https.getString());
  }
  else
  {
    Serial.print("Error code: ");
    Serial.println(httpsResponseCode);
  }
  // Free resources
  https.end();

  return httpsResponseCode;
}

void BabyApi::ResponseParser(String parse)
{
  
  DeserializationError err = deserializeJson(response, parse);
  if (err)
  {
    Serial.print(F("deserializeJson() failed with code "));
    Serial.println(err.f_str());
  }
}


JsonArray serialiseTags(char * tags)
{
  JsonArray tagsArray;

  char * tag;
  char * savePtr;

  if(tags[0] != '\0')
  {
    tag = strtok_r(tags, ",", &savePtr); 

    if(tag == NULL)
    {
      tagsArray.add(tags);
    }

    while (tag != NULL)
    {
      tagsArray.add(tag);

      tag = strtok_r(NULL, ",", &savePtr); 
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

void BabyApi::searchResultParser(long *count, long *next, long *previous)
{
  int offsetLocation;

  *count = response["count"];

  offsetLocation = response["next"].as<String>().indexOf("offset=");

  *next = (offsetLocation > 0) ? response["next"].as<String>().substring(offsetLocation + 7).toInt() : 0;

  offsetLocation = response["previous"].as<String>().indexOf("offset=");

  *previous = (offsetLocation > 0) ? response["previous"].as<String>().substring(offsetLocation + 7).toInt() : 0;
}

BabyApi::searchResults<BabyApi::BMI> BabyApi::findBMIRecords(
    uint16_t offset = 0,
    uint16_t child = 0,
    const char * date = {},
    const char * ordering = {})
{
  BabyApi::searchResults<BabyApi::BMI> outcome;
  uint16_t count = 0;
    
  snprintf(query,256,"limit=%d%s%s%s%s",
    SEARCH_LIMIT,
    (offset > 0) ? ",offset=" + String(offset) : "",
    (child > 0) ? ",child=" + String(child) : "",
    (date[0] != '\0') ? ",date=" + String(date) : "",
    (ordering[0] != '\0') ? ",ordering=" + String(ordering) : ""
  );


  int httpsResponseCode = httpRequest(BMI_ENDPOINT, "GET", "", query);
  Serial.println(httpsResponseCode);

  if(httpsResponseCode <= 0)
  {
    return outcome;
  }

  searchResultParser(&outcome.count, &outcome.next, &outcome.previous);

  JsonArray results = response["results"].as<JsonArray>();

  for (JsonObject bmiRecord : results)
  {
    outcome.results[count].id = bmiRecord["id"];
    outcome.results[count].child = bmiRecord["child"];
    bmiRecord["date"].as<String>().toCharArray(outcome.results[count].date,26);
    outcome.results[count].bmi = bmiRecord["bmi"];
    bmiRecord["notes"].as<String>().toCharArray(outcome.results[count].notes,256);
    deserialiseTags(bmiRecord["tags"].as<JsonArray>()).toCharArray(outcome.results[count].tags, 256);

    count++;
  }

  return outcome;
}

BabyApi::BMI BabyApi::getBMI(uint16_t id)
{
  BabyApi::BMI outcome;

  snprintf(parameters, 256,"/%d/",id);

  int httpsResponseCode = httpRequest(BMI_ENDPOINT, "GET", parameters);
  Serial.println(httpsResponseCode);

  if(httpsResponseCode <= 0)
  {
    return outcome;
  }

  outcome.id = response["id"];
  outcome.child = response["child"];
  response["date"].as<String>().toCharArray(outcome.date,33);
  outcome.bmi = response["bmi"];
  response["notes"].as<String>().toCharArray(outcome.notes,256);
  deserialiseTags(response["tags"].as<JsonArray>()).toCharArray(outcome.tags, 256);

  return outcome;
}


BabyApi::BMI BabyApi::logBMI(
    uint16_t child,
    float bmi,
    char * date,
    char * notes = {},
    char * tags = {})
{
  
  BabyApi::BMI outcome;
  
  doc.clear();

  doc["child"] = child;
  doc["date"] = date;
  doc["bmi"] = bmi;
  doc["notes"] = notes;
  doc["tags"] = serialiseTags(tags);
  
  serializeJson(doc, requestBody);

  int httpsResponseCode = httpRequest(BMI_ENDPOINT, "POST", "", "", requestBody);
  Serial.println(httpsResponseCode);

  if(httpsResponseCode <= 0)
  {
    return outcome;
  }

  outcome.id = response["id"];
  outcome.child = response["child"];
  response["date"].as<String>().toCharArray(outcome.date,33);
  outcome.bmi = response["bmi"];
  response["notes"].as<String>().toCharArray(outcome.notes,256);
  deserialiseTags(response["tags"].as<JsonArray>()).toCharArray(outcome.tags, 256);

  return outcome;
}

BabyApi::BMI BabyApi::updateBMI(
    uint16_t id,
    uint16_t child = 0,
    float bmi = NAN,
    char * date = {},
    bool updateNotes = false,
    char * notes = {},
    bool updateTags = false,
    char * tags = {})
{
  
  BabyApi::BMI outcome;

  doc.clear();

  if (child > 0)
    doc["child"] = child;
  if (date != '\0')
    doc["date"] = date;
  if (!isnan(bmi))
    doc["bmi"] = bmi;
  if (updateNotes)
    doc["notes"] = notes;
  if (updateTags)
    doc["tags"] = serialiseTags(tags);

  String parameters = "/" + String(id) + "/";


  serializeJson(doc, requestBody);

  int httpsResponseCode = httpRequest(BMI_ENDPOINT, "PATCH", parameters.c_str(), "", requestBody);
  Serial.println(httpsResponseCode);

  if(httpsResponseCode <= 0)
  {
    return outcome;
  }

  JsonArray results = response["results"].as<JsonArray>();

  outcome.id = response["id"];
  outcome.child = response["child"];
  response["date"].as<String>().toCharArray(outcome.date,33);
  outcome.bmi = response["bmi"];
  response["notes"].as<String>().toCharArray(outcome.notes,256);
  deserialiseTags(response["tags"].as<JsonArray>()).toCharArray(outcome.tags, 256);

  return outcome;
}

bool BabyApi::deleteBMI(uint16_t id)
{
  String parameters = "/" + String(id) + "/";

  int responseCode = httpRequest(BMI_ENDPOINT, "DELETE", parameters.c_str(), "", "");
  Serial.println(responseCode);

  return responseCode == 204;
}

BabyApi::searchResults<BabyApi::DiaperChange> BabyApi::findDiaperChanges(
    uint16_t offset = 0,
    uint16_t child = 0,
     char * colour = {},
     char * date = {},
     char * date_max = {},
     char * date_min = {},
     char * solid = {},
     char * wet = {},
     char * tags = {},
     char * ordering = {})
{
  BabyApi::searchResults<BabyApi::DiaperChange> outcome;
  uint16_t count = 0;
  
  snprintf(query,256,"limit=%d%s%s%s%s%s%s%s%s%s%s",
    SEARCH_LIMIT,
    offset > 0 ? ",offset=" + String(offset) : "",
    child > 0 ? ",child=" + String(child) : "",
    date[0] != '\0' ? ",date=" + String(date) : "",
    date_max[0] != '\0' ? "&date_max=" + String(date_max) : "",
    date_min[0] != '\0' ? "&date_min=" + String(date_min) : "",
    colour[0] != '\0' ? "&colour=" + String(colour) : "",
    solid[0] != '\0' ? "&solid=" + String(solid) : "",
    wet[0] != '\0' ? "&wet=" + String(wet) : "",
    tags[0] != '\0' ? "&tags=" + String(tags) : "",
    ordering[0] != '\0' ? ",ordering=" + String(ordering) : ""
  );

  int httpsResponseCode = httpRequest(CHANGES_ENDPOINT, "GET", "", query);
  Serial.println(httpsResponseCode);

  if(httpsResponseCode <= 0)
  {
    return outcome;
  }

  searchResultParser(&outcome.count, &outcome.next, &outcome.previous);

  JsonArray results = response["results"].as<JsonArray>();

  for (JsonObject diaperChangeRecord : results)
  {
    outcome.results[count].id = diaperChangeRecord["id"];
    outcome.results[count].child = diaperChangeRecord["child"];
    outcome.results[count].amount = diaperChangeRecord["amount"];
    diaperChangeRecord["color"].as<String>().toCharArray(outcome.results[count].color, 8);
    outcome.results[count].solid = diaperChangeRecord["solid"];
    outcome.results[count].wet = diaperChangeRecord["wet"];
    diaperChangeRecord["time"].as<String>().toCharArray(outcome.results[count].time, 33);
    diaperChangeRecord["notes"].as<String>().toCharArray(outcome.results[count].notes, 256);
    deserialiseTags(diaperChangeRecord["tags"].as<JsonArray>()).toCharArray(outcome.results[count].tags, 256);

    count++;
  }

  return outcome;
}

BabyApi::DiaperChange BabyApi::logDiaperChange(
    uint16_t child,
    bool wet,
    bool solid,
    char * color = {},
    float amount = NAN,
    char * notes = {},
    char * tags = {})
{
  return logDiaperChange(child, "", wet, solid, color, amount, notes, tags);
}

BabyApi::DiaperChange BabyApi::logDiaperChange(
    uint16_t child,
    char * time,
    bool wet = false,
    bool solid = false,
    char * color = {},
    float amount = NAN,
    char * notes = {},
    char * tags = {})
{
  
  BabyApi::DiaperChange outcome;

  doc.clear();

  doc["child"] = child;
  doc["time"] = time;
  doc["wet"] = wet;
  doc["solid"] = solid;
  if (color[0] != '\0')
    doc["color"] = color;
  if (!isnan(amount))
    doc["amount"] = amount;
  doc["notes"] = notes;
  doc["tags"] = serialiseTags(tags);

  serializeJson(doc, requestBody);

  int httpsResponseCode = httpRequest(CHANGES_ENDPOINT, "POST", "", "", requestBody);
  Serial.println(httpsResponseCode);

  if(httpsResponseCode <= 0)
  {
    return outcome;
  }

  outcome.id = response["id"];
  outcome.child = response["child"];
  outcome.amount = response["amount"];
  response["color"].as<String>().toCharArray(outcome.color,8);
  outcome.solid = response["solid"];
  outcome.wet = response["wet"];
  response["time"].as<String>().toCharArray(outcome.time,33);
  response["notes"].as<String>().toCharArray(outcome.notes,256);

  deserialiseTags(response["tags"].as<JsonArray>()).toCharArray(outcome.tags, 256);

  return outcome;
}

BabyApi::DiaperChange BabyApi::getDiaperChange(uint16_t id)
{
  BabyApi::DiaperChange outcome;

  String parameters = "/" + String(id) + "/";

  int httpsResponseCode = httpRequest(CHANGES_ENDPOINT, "GET", parameters.c_str());
  Serial.println(httpsResponseCode);

  if(httpsResponseCode <= 0)
  {
    return outcome;
  }

  outcome.id = response["id"];
  outcome.child = response["child"];
  outcome.amount = response["amount"];
  response["color"].as<String>().toCharArray(outcome.color,8);
  outcome.solid = response["solid"];
  outcome.wet = response["wet"];
  response["time"].as<String>().toCharArray(outcome.time,33);
  response["notes"].as<String>().toCharArray(outcome.notes,256);
  deserialiseTags(response["tags"].as<JsonArray>()).toCharArray(outcome.tags, 256);

  return outcome;
}

BabyApi::DiaperChange BabyApi::updateDiaperChange(
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
    char * tags = {})
{
  
  BabyApi::DiaperChange outcome;

  doc.clear();

  if (child > 0)
    doc["child"] = child;
  if (time[0] != '\0')
    doc["time"] = time;
  if (wet[0] != '\0')
    doc["wet"] = wet;
  if (solid[0] != '\0')
    doc["solid"] = solid;
  if (color[0] != '\0')
    doc["color"] = color;
  if (!isnan(amount))
    doc["amount"] = amount;
  if (updateNotes)
    doc["notes"] = notes;
  if (updateTags)
    doc["tags"] = serialiseTags(tags);

  String parameters = "/" + String(id) + "/";

  serializeJson(doc, requestBody);

  int httpsResponseCode = httpRequest(CHANGES_ENDPOINT, "PATCH", parameters.c_str(), "", requestBody);
  Serial.println(httpsResponseCode);

  if(httpsResponseCode <= 0)
  {
    return outcome;
  }

  outcome.id = response["id"];
  outcome.child = response["child"];
  outcome.amount = response["amount"];
  response["color"].as<String>().toCharArray(outcome.color,8);
  outcome.solid = response["solid"];
  outcome.wet = response["wet"];
  response["time"].as<String>().toCharArray(outcome.time,33);
  response["notes"].as<String>().toCharArray(outcome.notes,256);
  deserialiseTags(response["tags"].as<JsonArray>()).toCharArray(outcome.tags, 256);

  return outcome;
}

bool BabyApi::removeDiaperChange(uint16_t id)
{
  String parameters = "/" + String(id) + "/";

  int httpsResponseCode = httpRequest(CHANGES_ENDPOINT, "DELETE", parameters.c_str());
  Serial.println(httpsResponseCode);

  return httpsResponseCode == 204;
}

BabyApi::searchResults<BabyApi::Child> BabyApi::findChildren(
    uint16_t offset = 0,
    char * first_name = {},
    char * last_name = {},
    char * birth_date = {},
    char * slug = {},
    char * ordering = {})
{
  BabyApi::searchResults<BabyApi::Child> outcome;
  uint16_t count = 0;

  snprintf(query,256,"limit=%d%s%s%s%s%s%s%s%s%s%s",
    SEARCH_LIMIT,
    offset > 0 ? ",offset=" + String(offset) : "",
    first_name[0] != '\0' ? ",first_name=" + String(first_name) : "",
    last_name[0] != '\0' ? "&last_name=" + String(last_name) : "",
    birth_date[0] != '\0' ? "&birth_date=" + String(birth_date) : "",
    slug[0] != '\0' ? "&slug=" + String(slug) : "",
    ordering[0] != '\0' ? ",ordering=" + String(ordering) : ""
  );

  int httpsResponseCode = httpRequest(CHILDREN_ENDPOINT, "GET", "", query);
  Serial.println(httpsResponseCode);

  if(httpsResponseCode <= 0)
  {
    return outcome;
  }

  searchResultParser(&outcome.count, &outcome.next, &outcome.previous);

  JsonArray results = response["results"].as<JsonArray>();

  for (JsonObject childeRecord : results)
  {
    outcome.results[count].id = childeRecord["id"];
    childeRecord["first_name"].as<String>().toCharArray(outcome.results[count].first_name,256);
    childeRecord["last_name"].as<String>().toCharArray(outcome.results[count].last_name,256);
    childeRecord["birth_date"].as<String>().toCharArray(outcome.results[count].birth_date,11);
    childeRecord["picture"].as<String>().toCharArray(outcome.results[count].picture,256);
    count++;
  }

  return outcome;
}

BabyApi::Child BabyApi::newChild(
    char * first_name,
    char * last_name,
    char * birth_date,
    char * picture = {})
{
  BabyApi::Child outcome;

  doc.clear();

  doc["first_name"] = first_name;
  doc["last_name"] = last_name;
  doc["birth_date"] = birth_date;
  doc["picture"] = picture;

  serializeJson(doc, requestBody);

  int httpsResponseCode = httpRequest(CHILDREN_ENDPOINT, "POST", "", "", requestBody);
  Serial.println(httpsResponseCode);

  if(httpsResponseCode <= 0)
  {
    return outcome;
  }

  outcome.id = response["id"];
  response["first_name"].as<String>().toCharArray(outcome.first_name,256);
  response["last_name"].as<String>().toCharArray(outcome.last_name,256);
  response["birth_date"].as<String>().toCharArray(outcome.birth_date,11);
  response["picture"].as<String>().toCharArray(outcome.picture,256);
  response["slug"].as<String>().toCharArray(outcome.slug,101);

  return outcome;
}

BabyApi::Child BabyApi::getChild(char * slug)
{

  BabyApi::Child outcome;

  String parameters = "/" + String(slug) + "/";

  int httpsResponseCode = httpRequest(CHILDREN_ENDPOINT, "GET", parameters.c_str());
  Serial.println(httpsResponseCode);

  if(httpsResponseCode <= 0)
  {
    return outcome;
  }

  outcome.id = response["id"];
  response["first_name"].as<String>().toCharArray(outcome.first_name,256);
  response["last_name"].as<String>().toCharArray(outcome.last_name,256);
  response["birth_date"].as<String>().toCharArray(outcome.birth_date,11);
  response["picture"].as<String>().toCharArray(outcome.picture,256);
  response["slug"].as<String>().toCharArray(outcome.slug,101);

  return outcome;
}

BabyApi::Child BabyApi::updateChild(
    char * slug,
    char * first_name = {},
    char * last_name = {},
    char * birth_date = {},
    bool updatePicture = false,
    char * picture = {})
{
  
  BabyApi::Child outcome;

  doc.clear();

  if (first_name[0] != '\0')
    doc["first_name"] = first_name;
  if (last_name[0] != '\0')
    doc["last_name"] = last_name;
  if (birth_date[0] != '\0')
    doc["birth_date"] = birth_date;
  if (updatePicture)
    doc["picture"] = picture;

  serializeJson(doc, requestBody);

  String parameters = "/" + String(slug) + "/";

  int httpsResponseCode = httpRequest(CHILDREN_ENDPOINT, "PATCH", parameters.c_str(), "", babyApi.requestBody);
  Serial.println(httpsResponseCode);

  if(httpsResponseCode <= 0)
  {
    return outcome;
  }

  outcome.id = response["id"];
  response["first_name"].as<String>().toCharArray(outcome.first_name,256);
  response["last_name"].as<String>().toCharArray(outcome.last_name,256);
  response["birth_date"].as<String>().toCharArray(outcome.birth_date,11);
  response["picture"].as<String>().toCharArray(outcome.picture,256);
  response["slug"].as<String>().toCharArray(outcome.slug,101);

  return outcome;
}
bool BabyApi::removeChild(char *slug)
{
  String parameters = "/" + String(slug) + "/";

  int httpsResponseCode = httpRequest(CHILDREN_ENDPOINT, "DELETE", parameters.c_str());
  Serial.println(httpsResponseCode);

  return httpsResponseCode == 204;
}

BabyApi::searchResults<BabyApi::Feeding> BabyApi::findFeedingRecords(
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
    char * ordering = {})
{
  BabyApi::searchResults<BabyApi::Feeding> outcome;
  uint16_t count = 0;

  snprintf(query,256,"limit=%d%s%s%s%s%s%s%s%s%s%s%s%s",
    SEARCH_LIMIT,
    offset > 0 ? ",offset=" + String(offset) : "",
    child > 0 ? ",child=" + String(child) : "",
    start[0] != '\0' ? ",start=" + String(start) : "",
    start_max[0] != '\0' ? "&start_max=" + String(start_max) : "",
    start_min[0] != '\0' ? "&start_min=" + String(start_min) : "",
    end[0] != '\0' ? ",end=" + String(start) : "",
    end_max[0] != '\0' ? "&end_max=" + String(start_max) : "",
    end_min[0] != '\0' ? "&end_min=" + String(start_min) : "",
    type[0] != '\0' ? "&type=" + String(type) : "",
    method[0] != '\0' ? "&method=" + String(method) : "",
    tags[0] != '\0' ? "&tags=" + String(tags) : "",
    ordering[0] != '\0' ? ",ordering=" + String(ordering) : ""
  );

  String jsonBuffer;
  jsonBuffer = httpRequest(FEEDINGS_ENDPOINT, "GET", "", query);
  Serial.println(jsonBuffer);

  ResponseParser(jsonBuffer);

  searchResultParser(&outcome.count, &outcome.next, &outcome.previous);

  JsonArray results = response["results"].as<JsonArray>();

  for (JsonObject feedingRecord : results)
  {
    outcome.results[count].id = feedingRecord["id"];
    outcome.results[count].child = feedingRecord["child"].as<int>();
    outcome.results[count].amount = feedingRecord["amount"].as<float>();
    feedingRecord["duration"].as<String>().toCharArray(outcome.results[count].duration,22);
    feedingRecord["start"].as<String>().toCharArray(outcome.results[count].start,33);
    feedingRecord["end"].as<String>().toCharArray(outcome.results[count].end,33);
    feedingRecord["method"].as<String>().toCharArray(outcome.results[count].method,13);
    feedingRecord["type"].as<String>().toCharArray(outcome.results[count].type,22);
    feedingRecord["notes"].as<String>().toCharArray(outcome.results[count].notes,256);
    deserialiseTags(feedingRecord["tags"].as<JsonArray>()).toCharArray(outcome.results[count].tags, 256);

    count++;
  }

  return outcome;
}

BabyApi::Feeding BabyApi::logFeeding(
    uint16_t timer,
    char * type,
    char * method,
    float amount,
    char * notes = {},
    char * tags = {})
{
  return logFeeding(
      0,
      "",
      "",
      timer,
      type,
      method,
      amount,
      notes,
      tags);
}

BabyApi::Feeding BabyApi::logFeeding(
    uint16_t child,    // Required unless a Timer value is provided.
    char * start, // Required unless a Timer value is provided.
    char * end,   // Required unless a Timer value is provided.
    char * type,
    char * method,
    float amount = NAN,
    char * notes = {},
    char * tags = {})
{
  return logFeeding(
      child,
      start,
      end,
      0,
      type,
      method,
      amount,
      notes,
      tags);
}

BabyApi::Feeding BabyApi::logFeeding(
    uint16_t child = 0,          // Required unless a Timer value is provided.
    char * start = {}, // Required unless a Timer value is provided.
    char * end = {},   // Required unless a Timer value is provided.
    uint16_t timer = 0,          // May be used in place of the Start, End, and/or Child values.
    char * type = {},
    char * method = {},
    float amount = NAN,
    char * notes = {},
    char * tags = {})
{
  BabyApi::Feeding outcome;
  doc.clear();

  // if no tmer value present, child start and end are required fields
  if (timer == 0 && (child == 0 || start[0] != '\0'  || end[0] != '\0' ))
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

  if (child > 0)
    doc["child"] = child;
  doc["start"] = start;
  doc["end"] = end;
  if (timer > 0)
    doc["timer"] = timer;
  doc["type"] = type;
  doc["method"] = method;
  if (!isnan(amount))
    doc["amount"] = amount;
  doc["notes"] = notes;
  doc["tags"] = serialiseTags(tags);

  serializeJson(doc, requestBody);

  String jsonBuffer;
  jsonBuffer = httpRequest(FEEDINGS_ENDPOINT, "POST", "", "", requestBody);
  Serial.println(jsonBuffer);

  ResponseParser(jsonBuffer);

  outcome.id = response["id"];
  outcome.child = response["child"].as<int>();
  outcome.amount = response["amount"].as<float>();
  response["duration"].as<String>().toCharArray(outcome.duration,22);
  response["start"].as<String>().toCharArray(outcome.start,33);
  response["end"].as<String>().toCharArray(outcome.end,33);
  response["method"].as<String>().toCharArray(outcome.method,13);
  response["type"].as<String>().toCharArray(outcome.type,22);
  response["notes"].as<String>().toCharArray(outcome.notes,256);
  deserialiseTags(response["tags"].as<JsonArray>()).toCharArray(outcome.tags, 256);

  return outcome;
}

BabyApi::Feeding BabyApi::getFeeding(uint16_t id)
{
  BabyApi::Feeding outcome;

  String parameters = "/" + String(id) + "/";

  String jsonBuffer;
  jsonBuffer = httpRequest(FEEDINGS_ENDPOINT, "GET", parameters.c_str());
  Serial.println(jsonBuffer);

  ResponseParser(jsonBuffer);

  outcome.id = response["id"];
  outcome.child = response["child"].as<int>();
  outcome.amount = response["amount"].as<float>();
  response["duration"].as<String>().toCharArray(outcome.duration,22);
  response["start"].as<String>().toCharArray(outcome.start,33);
  response["end"].as<String>().toCharArray(outcome.end,33);
  response["method"].as<String>().toCharArray(outcome.method,13);
  response["type"].as<String>().toCharArray(outcome.type,22);
  response["notes"].as<String>().toCharArray(outcome.notes,256);
  deserialiseTags(response["tags"].as<JsonArray>()).toCharArray(outcome.tags, 256);

  return outcome;
}

BabyApi::Feeding BabyApi::updateFeeding(
    uint16_t id,
    uint16_t child = 0,          // Required unless a Timer value is provided.
    char * start = {}, // Required unless a Timer value is provided.
    char * end = {},   // Required unless a Timer value is provided.
    char * method = {},
    char * type = {},
    float amount = NAN,
    bool updateNotes = false,
    char * notes = {},
    bool updateTags = false,
    char * tags = {})
{
  BabyApi::Feeding outcome;
  
  doc.clear();

  if (child > 0)
    doc["child"] = child;
  if (start[0] != '\0' )
    doc["start"] = start;
  if (start[0] != '\0' )
    doc["end"] = end;
  if (type[0] != '\0')
    doc["type"] = type;
  if (method[0] != '\0')
    doc["method"] = method;
  if (!isnan(amount))
    doc["amount"] = amount;
  if (updateNotes)
    doc["notes"] = notes;
  if (updateTags)
    doc["tags"] = serialiseTags(tags);

  serializeJson(doc, requestBody);

  String parameters = "/" + String(id) + "/";

  String jsonBuffer;
  jsonBuffer = httpRequest(FEEDINGS_ENDPOINT, "PATCH", parameters.c_str(), "", requestBody);
  Serial.println(jsonBuffer);

  ResponseParser(jsonBuffer);

  outcome.id = response["id"];
  outcome.child = response["child"].as<int>();
  outcome.amount = response["amount"].as<float>();
  response["duration"].as<String>().toCharArray(outcome.duration,22);
  response["start"].as<String>().toCharArray(outcome.start,33);
  response["end"].as<String>().toCharArray(outcome.end,33);
  response["method"].as<String>().toCharArray(outcome.method,13);
  response["type"].as<String>().toCharArray(outcome.type,22);
  response["notes"].as<String>().toCharArray(outcome.notes,256);
  deserialiseTags(response["tags"].as<JsonArray>()).toCharArray(outcome.tags, 256);

  return outcome;
}

bool BabyApi::removeFeeding(uint16_t id)
{
  String parameters = "/" + String(id) + "/";
  int responseCode;
  String jsonBuffer;
  jsonBuffer = httpRequest(CHANGES_ENDPOINT, "DELETE", parameters.c_str(), "", "", &responseCode);
  Serial.println(jsonBuffer);

  return responseCode == 204;
}

BabyApi::searchResults<BabyApi::HeadCircumference> BabyApi::findHeadCircumferenceRecords(
    uint16_t offset = 0,
    uint16_t child = 0,
    char * date = {},
    char * ordering = {})
{
  BabyApi::searchResults<BabyApi::HeadCircumference> outcome;
  uint16_t count = 0;

  snprintf(query,256,"limit=%d%s%s%s%s",
    SEARCH_LIMIT,
    offset > 0 ? ",offset=" + String(offset) : "",
    child > 0 ? ",child=" + String(child) : "",
    date[0] != '\0' ? ",date=" + String(date) : "",
    ordering[0] != '\0' ? ",ordering=" + String(ordering) : ""
  );

  String jsonBuffer;
  jsonBuffer = httpRequest(HEAD_CIRCUMFERENCE_ENDPOINT, "GET", "", query);
  Serial.println(jsonBuffer);

  ResponseParser(jsonBuffer);

  searchResultParser(&outcome.count, &outcome.next, &outcome.previous);

  JsonArray results = response["results"].as<JsonArray>();

  for (JsonObject headCircumferenceRecord : results)
  {
    outcome.results[count].id = headCircumferenceRecord["id"];
    outcome.results[count].child = headCircumferenceRecord["child"];
    response["date"].as<String>().toCharArray(outcome.results[count].date,33);
    outcome.results[count].head_circumference = headCircumferenceRecord["head_circumference"];
    headCircumferenceRecord["notes"].as<String>().toCharArray(outcome.results[count].notes,256);
    deserialiseTags(headCircumferenceRecord["tags"].as<JsonArray>()).toCharArray(outcome.results[count].tags, 256);

    count++;
  }

  return outcome;
}

BabyApi::HeadCircumference BabyApi::logHeadCircumference(
    uint16_t child,
    float head_circumference,
    char * date,
    char * notes = {},
    char * tags = {})
{
  BabyApi::HeadCircumference outcome;

  doc.clear();

  doc["child"] = child;
  doc["date"] = date;
  doc["head_circumference"] = head_circumference;
  doc["notes"] = notes;
  doc["tags"] = serialiseTags(tags);

  serializeJson(doc, requestBody);

  String jsonBuffer;
  jsonBuffer = httpRequest(HEAD_CIRCUMFERENCE_ENDPOINT, "POST", "", "", requestBody);
  Serial.println(jsonBuffer);

  ResponseParser(jsonBuffer);

  outcome.id = response["id"];
  outcome.child = response["child"];
  response["date"].as<String>().toCharArray(outcome.date,33);
  outcome.head_circumference = response["head_circumference"];
  response["notes"].as<String>().toCharArray(outcome.notes,256);
  deserialiseTags(response["tags"].as<JsonArray>()).toCharArray(outcome.tags, 256);

  return outcome;
}

BabyApi::HeadCircumference BabyApi::getHeadCircumference(uint16_t id)
{
  BabyApi::HeadCircumference outcome;

  String parameters = "/" + String(id) + "/";

  String jsonBuffer;
  jsonBuffer = httpRequest(HEAD_CIRCUMFERENCE_ENDPOINT, "GET", parameters.c_str());
  Serial.println(jsonBuffer);

  ResponseParser(jsonBuffer);

  outcome.id = response["id"];
  outcome.child = response["child"];
  response["date"].as<String>().toCharArray(outcome.date,33);
  outcome.head_circumference = response["head_circumference"];
  response["notes"].as<String>().toCharArray(outcome.notes,256);
  deserialiseTags(response["tags"].as<JsonArray>()).toCharArray(outcome.tags, 256);

  return outcome;
}

BabyApi::HeadCircumference BabyApi::updateHeadCircumference(
    uint16_t id,
    uint16_t child = 0,
    float head_circumference = NAN,
    char * date = {},
    bool updateNotes = false,
    char * notes = {},
    bool updateTags = false,
    char * tags = {})
{
  BabyApi::HeadCircumference outcome;

  doc.clear();

  if (child > 0)
    doc["child"] = child;
  if (date[0] != '\0')
    doc["date"] = date;
  if (!isnan(head_circumference))
    doc["head_circumference"] = head_circumference;
  if (updateNotes)
    doc["notes"] = notes;
  if (updateTags)
    doc["tags"] = serialiseTags(tags);

  serializeJson(doc, requestBody);

  String parameters = "/" + String(id) + "/";

  String jsonBuffer;
  jsonBuffer = httpRequest(HEAD_CIRCUMFERENCE_ENDPOINT, "PATCH", parameters.c_str(), "", requestBody);
  Serial.println(jsonBuffer);

  ResponseParser(jsonBuffer);

  outcome.id = response["id"];
  outcome.child = response["child"];
  response["date"].as<String>().toCharArray(outcome.date,33);
  outcome.head_circumference = response["head_circumference"];
  response["notes"].as<String>().toCharArray(outcome.notes,256);
  deserialiseTags(response["tags"].as<JsonArray>()).toCharArray(outcome.tags, 256);

  return outcome;
}

bool BabyApi::removeHeadCircumference(uint16_t id)
{
  String parameters = "/" + String(id) + "/";
  int responseCode;
  String jsonBuffer;
  jsonBuffer = httpRequest(CHANGES_ENDPOINT, "DELETE", parameters.c_str(), "", "", &responseCode);
  Serial.println(jsonBuffer);

  return responseCode == 204;
}

BabyApi::searchResults<BabyApi::Height> BabyApi::findHeightRecords(
    uint16_t offset = 0,
    uint16_t child = 0,
    char * date = {},
    char * ordering = {})
{
  BabyApi::searchResults<BabyApi::Height> outcome;
  uint16_t count = 0;

  snprintf(query,256,"limit=%d%s%s%s%s",
    SEARCH_LIMIT,
    offset > 0 ? ",offset=" + String(offset) : "",
    child > 0 ? ",child=" + String(child) : "",
    date[0] != '\0' ? ",date=" + String(date) : "",
    ordering[0] != '\0' ? ",ordering=" + String(ordering) : ""
  );

  String jsonBuffer;
  jsonBuffer = httpRequest(HEIGHT_ENDPOINT, "GET", "", query);
  Serial.println(jsonBuffer);

  ResponseParser(jsonBuffer);

  searchResultParser(&outcome.count, &outcome.next, &outcome.previous);

  JsonArray results = response["results"].as<JsonArray>();

  for (JsonObject heightRecord : results)
  {
    outcome.results[count].id = heightRecord["id"];
    outcome.results[count].child = heightRecord["child"];
    heightRecord["date"].as<String>().toCharArray(outcome.results[count].date,33);
    outcome.results[count].height = heightRecord["height"];
    heightRecord["notes"].as<String>().toCharArray(outcome.results[count].notes,256);
    deserialiseTags(heightRecord["tags"].as<JsonArray>()).toCharArray(outcome.results[count].tags, 256);

    count++;
  }

  return outcome;
}

BabyApi::Height BabyApi::logHeight(
    uint16_t child,
    float height,
    char * date,
    char * notes = {},
    char * tags = {})
{
  BabyApi::Height outcome;

  doc.clear();

  doc["child"] = child;
  doc["date"] = date;
  doc["height"] = height;
  doc["notes"] = notes;
  doc["tags"] = serialiseTags(tags);

  serializeJson(doc, requestBody);

  String jsonBuffer;
  jsonBuffer = httpRequest(HEIGHT_ENDPOINT, "POST", "", "", requestBody);
  Serial.println(jsonBuffer);

  ResponseParser(jsonBuffer);

  outcome.id = response["id"];
  outcome.child = response["child"];
  response["date"].as<String>().toCharArray(outcome.date,33);
  outcome.height = response["height"];
  response["notes"].as<String>().toCharArray(outcome.notes,256);
  deserialiseTags(response["tags"].as<JsonArray>()).toCharArray(outcome.tags, 256);

  return outcome;
}

BabyApi::Height BabyApi::getHeight(uint16_t id)
{
  BabyApi::Height outcome;

  String parameters = "/" + String(id) + "/";

  String jsonBuffer;
  jsonBuffer = httpRequest(HEIGHT_ENDPOINT, "GET", parameters.c_str());
  Serial.println(jsonBuffer);

  ResponseParser(jsonBuffer);

  outcome.id = response["id"];
  outcome.child = response["child"];
  response["date"].as<String>().toCharArray(outcome.date,33);
  outcome.height = response["height"];
  response["notes"].as<String>().toCharArray(outcome.notes,256);
  deserialiseTags(response["tags"].as<JsonArray>()).toCharArray(outcome.tags, 256);

  return outcome;
}

BabyApi::Height BabyApi::updateHeight(
    uint16_t id,
    uint16_t child = 0,
    float height = NAN,
    char * date = {},
    bool updateNotes = false,
    char * notes = {},
    bool updateTags = false,
    char * tags = {})
{
  Height outcome;

  doc.clear();

  if (child > 0)
    doc["child"] = child;
  if (date != '\0')
    doc["date"] = date;
  if (!isnan(height))
    doc["height"] = height;
  if (updateNotes)
    doc["notes"] = notes;
  if (updateTags)
    doc["tags"] = serialiseTags(tags);

  serializeJson(doc, requestBody);

  String parameters = "/" + String(id) + "/";

  String jsonBuffer;
  jsonBuffer = httpRequest(HEIGHT_ENDPOINT, "PATCH", parameters.c_str(), "", requestBody);
  Serial.println(jsonBuffer);

  ResponseParser(jsonBuffer);

  outcome.id = response["id"];
  outcome.child = response["child"];
  outcome.height = response["height"];
  response["date"].as<String>().toCharArray(outcome.date,33);
  response["notes"].as<String>().toCharArray(outcome.notes,256);
  deserialiseTags(response["tags"].as<JsonArray>()).toCharArray(outcome.tags, 256);

  return outcome;
}

bool BabyApi::removeHeight(uint16_t id)
{
  String parameters = "/" + String(id) + "/";
  int responseCode;
  String jsonBuffer;
  jsonBuffer = httpRequest(CHANGES_ENDPOINT, "DELETE", parameters.c_str(), "", "", &responseCode);
  Serial.println(jsonBuffer);

  return responseCode == 204;
}

BabyApi::searchResults<BabyApi::Note> BabyApi::findNotes(
    uint16_t offset = 0,
    uint16_t child = 0,
    char * date = {},
    char * date_max = {},
    char * date_min = {},
    char * tags = {},
    char * ordering = {})
{
  BabyApi::searchResults<BabyApi::Note> outcome;
  uint16_t count = 0;

  snprintf(query,256,"limit=%d%s%s%s%s%s%s%s",
    SEARCH_LIMIT,
    offset > 0 ? ",offset=" + String(offset) : "",
    child > 0 ? ",child=" + String(child) : "",
    date[0] != '\0' ? ",date=" + String(date) : "",
    date_max[0] != '\0' ? "&date_max=" + String(date_max) : "",
    date_min[0] != '\0' ? "&date_min=" + String(date_min) : "",
    tags[0] != '\0' ? "&tags=" + String(tags) : "",
    ordering[0] != '\0' ? ",ordering=" + String(ordering) : ""
  );

  String jsonBuffer;
  jsonBuffer = httpRequest(NOTES_ENDPOINT, "GET", "", query);
  Serial.println(jsonBuffer);

  ResponseParser(jsonBuffer);

  searchResultParser(&outcome.count, &outcome.next, &outcome.previous);

  JsonArray results = response["results"].as<JsonArray>();

  for (JsonObject noteRecord : results)
  {
    outcome.results[count].id = noteRecord["id"];
    outcome.results[count].child = noteRecord["child"];
    noteRecord["date"].as<String>().toCharArray(outcome.results[count].date,33);
    noteRecord["note"].as<String>().toCharArray(outcome.results[count].note,256);
    deserialiseTags(noteRecord["tags"].as<JsonArray>()).toCharArray(outcome.results[count].tags, 256);

    count++;
  }

  return outcome;
}

BabyApi::Note BabyApi::createNote(
    uint16_t child,
    char * note,
    char * date,
    char * tags = {})
{
  BabyApi::Note outcome;

  doc.clear();

  doc["child"] = child;
  doc["date"] = date;
  doc["note"] = note;
  doc["tags"] = serialiseTags(tags);

  serializeJson(doc, requestBody);

  String jsonBuffer;
  jsonBuffer = httpRequest(NOTES_ENDPOINT, "POST", "", "", requestBody);
  Serial.println(jsonBuffer);

  ResponseParser(jsonBuffer);

  outcome.id = response["id"];
  outcome.child = response["child"];
  response["date"].as<String>().toCharArray(outcome.date,33);
  response["note"].as<String>().toCharArray(outcome.note,256);
  deserialiseTags(response["tags"].as<JsonArray>()).toCharArray(outcome.tags, 256);

  return outcome;
}

BabyApi::Note BabyApi::getNote(uint16_t id)
{
  BabyApi::Note outcome;

  String parameters = "/" + String(id) + "/";

  String jsonBuffer;
  jsonBuffer = httpRequest(NOTES_ENDPOINT, "GET", parameters.c_str());
  Serial.println(jsonBuffer);

  ResponseParser(jsonBuffer);

  outcome.id = response["id"];
  outcome.child = response["child"];
  response["date"].as<String>().toCharArray(outcome.date,33);
  response["note"].as<String>().toCharArray(outcome.note,256);
  deserialiseTags(response["tags"].as<JsonArray>()).toCharArray(outcome.tags, 256);

  return outcome;
}

BabyApi::Note BabyApi::updateNote(
    uint16_t id,
    uint16_t child = 0,
    char * date = {},
    bool updateNote = false,
    char * note = {},
    bool updateTags = false,
    char * tags = {})
{
  BabyApi::Note outcome;

  doc.clear();

  if (child > 0)
    doc["child"] = child;
  if (date != '\0')
    doc["date"] = date;
  if (updateNote)
    doc["note"] = note;
  if (updateTags)
    doc["tags"] = serialiseTags(tags);

  serializeJson(doc, requestBody);

  String parameters = "/" + String(id) + "/";

  String jsonBuffer;
  jsonBuffer = httpRequest(NOTES_ENDPOINT, "PATCH", parameters.c_str(), "", requestBody);
  Serial.println(jsonBuffer);

  ResponseParser(jsonBuffer);

  outcome.id = response["id"];
  outcome.child = response["child"];
  response["date"].as<String>().toCharArray(outcome.date,33);
  response["note"].as<String>().toCharArray(outcome.note,256);
  deserialiseTags(response["tags"].as<JsonArray>()).toCharArray(outcome.tags, 256);

  return outcome;
}

bool BabyApi::removeNote(uint16_t id)
{
  String parameters = "/" + String(id) + "/";
  int responseCode;
  String jsonBuffer;
  jsonBuffer = httpRequest(CHANGES_ENDPOINT, "DELETE", parameters.c_str(), "", "", &responseCode);
  Serial.println(jsonBuffer);

  return responseCode == 204;
}

BabyApi::searchResults<BabyApi::Pumping> BabyApi::findPumpingRecords(
    uint16_t offset = 0,
    uint16_t child = 0,
    char * date = {},
    char * date_max = {},
    char * date_min = {},
    char * ordering = {})
{
  BabyApi::searchResults<BabyApi::Pumping> outcome;
  uint16_t count = 0;

  snprintf(query,256,"limit=%d%s%s%s%s%s%s",
    SEARCH_LIMIT,
    offset > 0 ? ",offset=" + String(offset) : "",
    child > 0 ? ",child=" + String(child) : "",
    date[0] != '\0' ? ",date=" + String(date) : "",
    date_max[0] != '\0' ? "&date_max=" + String(date_max) : "",
    date_min[0] != '\0' ? "&date_min=" + String(date_min) : "",
    ordering[0] != '\0' ? ",ordering=" + String(ordering) : ""
  );

  String jsonBuffer;
  jsonBuffer = httpRequest(PUMPING_ENDPOINT, "GET", "", query);
  Serial.println(jsonBuffer);

  ResponseParser(jsonBuffer);

  searchResultParser(&outcome.count, &outcome.next, &outcome.previous);

  JsonArray results = response["results"].as<JsonArray>();

  for (JsonObject pumpingRecord : results)
  {
    outcome.results[count].id = pumpingRecord["id"];
    outcome.results[count].child = pumpingRecord["child"];
    pumpingRecord["time"].as<String>().toCharArray(outcome.results[count].time,33);
    outcome.results[count].amount = pumpingRecord["amount"];
    pumpingRecord["notes"].as<String>().toCharArray(outcome.results[count].notes , 256);
    deserialiseTags(pumpingRecord["tags"].as<JsonArray>()).toCharArray(outcome.results[count].tags, 256);

    count++;
  }

  return outcome;
}

BabyApi::Pumping BabyApi::logPumping(
    uint16_t child,
    float amount,
    char * time = {},
    char * notes = {},
    char * tags = {})
{
  BabyApi::Pumping outcome;

  doc.clear();

  doc["child"] = child;
  doc["time"] = time;
  doc["amount"] = amount;
  doc["notes"] = notes;
  doc["tags"] = serialiseTags(tags);

  serializeJson(doc, requestBody);

  String jsonBuffer;
  jsonBuffer = httpRequest(PUMPING_ENDPOINT, "POST", "", "", requestBody);
  Serial.println(jsonBuffer);

  ResponseParser(jsonBuffer);

  outcome.id = response["id"];
  outcome.child = response["child"];
  response["time"].as<String>().toCharArray(outcome.time,33);
  outcome.amount = response["amount"];
  response["notes"].as<String>().toCharArray(outcome.notes,256);
  deserialiseTags(response["tags"].as<JsonArray>()).toCharArray(outcome.tags, 256);

  return outcome;
}

BabyApi::Pumping BabyApi::getPumping(uint16_t id)
{
  BabyApi::Pumping outcome;

  String parameters = "/" + String(id) + "/";

  String jsonBuffer;
  jsonBuffer = httpRequest(PUMPING_ENDPOINT, "GET", parameters.c_str());
  Serial.println(jsonBuffer);

  ResponseParser(jsonBuffer);

  outcome.id = response["id"];
  outcome.child = response["child"];
  response["time"].as<String>().toCharArray(outcome.time,33);
  outcome.amount = response["amount"];
  response["notes"].as<String>().toCharArray(outcome.notes,256);
  deserialiseTags(response["tags"].as<JsonArray>()).toCharArray(outcome.tags, 256);

  return outcome;
}

BabyApi::Pumping BabyApi::updatePumping(
    uint16_t id,
    uint16_t child = 0,
    float amount = NAN,
    char * time = {},
    bool updateNotes = false,
    char * notes = {},
    bool updateTags = false,
    char * tags = {})
{
  BabyApi::Pumping outcome;

  doc.clear();

  if (child > 0)
    doc["child"] = child;
  if (time != '\0')
    doc["time"] = time;
  if (!isnan(amount))
    doc["amount"] = amount;
  if (updateNotes)
    doc["notes"] = notes;
  if (updateTags)
    doc["tags"] = serialiseTags(tags);

  serializeJson(doc, requestBody);

  snprintf(parameters, 256, "/%d/", id);

  String jsonBuffer;
  jsonBuffer = httpRequest(PUMPING_ENDPOINT, "PATCH", parameters, "", requestBody);
  Serial.println(jsonBuffer);

  ResponseParser(jsonBuffer);

  outcome.id = response["id"];
  outcome.child = response["child"];
  response["time"].as<String>().toCharArray(outcome.time,33);
  outcome.amount = response["amount"];
  response["notes"].as<String>().toCharArray(outcome.notes,256);
  deserialiseTags(response["tags"].as<JsonArray>()).toCharArray(outcome.tags, 256);

  return outcome;
}

bool BabyApi::removePumping(uint16_t id)
{
  String parameters = "/" + String(id) + "/";
  int responseCode;
  String jsonBuffer;
  jsonBuffer = httpRequest(CHANGES_ENDPOINT, "DELETE", parameters.c_str(), "", "", &responseCode);
  Serial.println(jsonBuffer);

  return responseCode == 204;
}

BabyApi::searchResults<BabyApi::Sleep> BabyApi::findSleepRecords(
    uint16_t offset = 0,
    uint16_t child = 0,
    char * start = {},
    char * start_max = {},
    char * start_min = {},
    char * end = {},
    char * end_max = {},
    char * end_min = {},
    char * tags = {},
    char * ordering = {})
{
  BabyApi::searchResults<BabyApi::Sleep> outcome;
  uint16_t count = 0;

  snprintf(query,256,"limit=%d%s%s%s%s%s%s%s%s%s%s",
    SEARCH_LIMIT,
    offset > 0 ? ",offset=" + String(offset) : "",
    child > 0 ? ",child=" + String(child) : "",
    start[0] != '\0' ? ",start=" + String(start) : "",
    start_max[0] != '\0' ? "&start_max=" + String(start_max) : "",
    start_min[0] != '\0' ? "&start_min=" + String(start_min) : "",
    end[0] != '\0' ? ",end=" + String(start) : "",
    end_max[0] != '\0' ? "&end_max=" + String(start_max) : "",
    end_min[0] != '\0' ? "&end_min=" + String(start_min) : "",
    tags[0] != '\0' ? "&tags=" + String(tags) : "",
    ordering[0] != '\0' ? ",ordering=" + String(ordering) : ""
  );

  String jsonBuffer;
  jsonBuffer = httpRequest(SLEEP_ENDPOINT, "GET", "", query);
  Serial.println(jsonBuffer);

  ResponseParser(jsonBuffer);

  searchResultParser(&outcome.count, &outcome.next, &outcome.previous);

  JsonArray results = response["results"].as<JsonArray>();

  for (JsonObject sleepingRecord : results)
  {
    outcome.results[count].id = sleepingRecord["id"];
    outcome.results[count].child = sleepingRecord["child"];
    outcome.results[count].nap = sleepingRecord["nap"].as<String>() == "True";
    sleepingRecord["start"].as<String>().toCharArray(outcome.results[count].start,3);
    sleepingRecord["end"].as<String>().toCharArray(outcome.results[count].end,33);
    sleepingRecord["duration"].as<String>().toCharArray(outcome.results[count].duration,17);
    sleepingRecord["notes"].as<String>().toCharArray(outcome.results[count].notes,256);
    deserialiseTags(sleepingRecord["tags"].as<JsonArray>()).toCharArray(outcome.results[count].tags, 256);

    count++;
  }

  return outcome;
}

BabyApi::Sleep BabyApi::logSleep(
    uint16_t child,          // Required unless a Timer value is provided.
    char * start = {}, // Required unless a Timer value is provided.
    char * end = {},   // Required unless a Timer value is provided.
    uint16_t timer = 0,          // May be used in place of the Start, End, and/or Child values.
    char * notes = {},
    char * tags = {})
{
  Sleep outcome;

  doc.clear();

  // if no tmer value present, child start and end are required fields
  if (timer == 0 && (child == 0 || start != '\0' || end != '\0'))
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

  if (child > 0)
    doc["child"] = child;
  doc["start"] = start;
  doc["end"] = end;
  if (timer > 0)
    doc["timer"] = timer;
  doc["notes"] = notes;
  doc["tags"] = serialiseTags(tags);

  serializeJson(doc, requestBody);

  String jsonBuffer;
  jsonBuffer = httpRequest(SLEEP_ENDPOINT, "POST", "", "", requestBody);
  Serial.println(jsonBuffer);

  ResponseParser(jsonBuffer);

  outcome.id = response["id"];
  outcome.child = response["child"];
  outcome.nap = response["nap"].as<String>() == "True";
  response["start"].as<String>().toCharArray(outcome.start,3);
  response["end"].as<String>().toCharArray(outcome.end,33);
  response["notes"].as<String>().toCharArray(outcome.notes,256);
  deserialiseTags(response["tags"].as<JsonArray>()).toCharArray(outcome.tags, 256);

  return outcome;
}

BabyApi::Sleep BabyApi::getSleep(uint16_t id)
{
  Sleep outcome;

  String parameters = "/" + String(id) + "/";

  String jsonBuffer;
  jsonBuffer = httpRequest(SLEEP_ENDPOINT, "GET", parameters.c_str());
  Serial.println(jsonBuffer);

  ResponseParser(jsonBuffer);

  outcome.id = response["id"];
  outcome.child = response["child"];
  outcome.nap = response["nap"].as<String>() == "True";
  response["start"].as<String>().toCharArray(outcome.start,3);
  response["end"].as<String>().toCharArray(outcome.end,33);
  response["duration"].as<String>().toCharArray(outcome.duration,17);
  response["notes"].as<String>().toCharArray(outcome.notes,256);
  deserialiseTags(response["tags"].as<JsonArray>()).toCharArray(outcome.tags, 256);

  return outcome;
}

BabyApi::Sleep BabyApi::updateSleep(
    uint16_t id,
    uint16_t child = 0,
    char * start = {},
    char * end = {},
    bool updateNotes = false,
    char * notes = {},
    bool updateTags = false,
    char * tags = {})
{
  Sleep outcome;

  doc.clear();

  if (child > 0)
    doc["child"] = child;
  if (start != '\0')
    doc["start"] = start;
  if (end != '\0')
    doc["end"] = end;
  if (updateNotes)
    doc["notes"] = notes;
  if (updateTags)
    doc["tags"] = serialiseTags(tags);

  serializeJson(doc, requestBody);

  String parameters = "/" + String(id) + "/";

  String jsonBuffer;
  jsonBuffer = httpRequest(SLEEP_ENDPOINT, "PATCH", parameters.c_str(), "", requestBody);
  Serial.println(jsonBuffer);

  ResponseParser(jsonBuffer);

  outcome.id = response["id"];
  outcome.child = response["child"];
  outcome.nap = response["nap"].as<String>() == "True";
  response["start"].as<String>().toCharArray(outcome.start,3);
  response["end"].as<String>().toCharArray(outcome.end,33);
  response["duration"].as<String>().toCharArray(outcome.duration,17);
  response["notes"].as<String>().toCharArray(outcome.notes,256);
  deserialiseTags(response["tags"].as<JsonArray>()).toCharArray(outcome.tags, 256);

  return outcome;
}

bool BabyApi::removeSleep(uint16_t id)
{
  String parameters = "/" + String(id) + "/";
  int responseCode;
  String jsonBuffer;
  jsonBuffer = httpRequest(CHANGES_ENDPOINT, "DELETE", parameters.c_str(), "", "", &responseCode);
  Serial.println(jsonBuffer);

  return responseCode == 204;
}

BabyApi::searchResults<BabyApi::Tag> BabyApi::findAllTags(
    uint16_t offset = 0,
    char * name = {},
    char * last_used = {},
    char * ordering = {})
{
  BabyApi::searchResults<BabyApi::Tag> outcome;
  uint16_t count = 0;

  snprintf(query,256,"limit=%d%s%s%s%s",
    SEARCH_LIMIT,
    offset > 0 ? ",offset=" + String(offset) : "",
    name > 0 ? ",name=" + String(name) : "",
    last_used[0] != '\0' ? ",last_used=" + String(last_used) : "",
    ordering[0] != '\0' ? ",ordering=" + String(ordering) : ""
  );

  String jsonBuffer;
  jsonBuffer = httpRequest(TAGS_ENDPOINT, "GET", "", query);
  Serial.println(jsonBuffer);

  ResponseParser(jsonBuffer);

  searchResultParser(&outcome.count, &outcome.next, &outcome.previous);

  JsonArray results = response["results"].as<JsonArray>();

  for (JsonObject tagRecord : results)
  {
    tagRecord["name"].as<String>().toCharArray(outcome.results[count].name, 256);
    tagRecord["last_used"].as<String>().toCharArray(outcome.results[count].last_used, 33);
    tagRecord["color"].as<String>().toCharArray(outcome.results[count].color, 8);
    tagRecord["slug"].as<String>().toCharArray(outcome.results[count].slug, 101);

    count++;
  }

  return outcome;
}

BabyApi::Tag BabyApi::createTag(
    char * name,
    char * colour = {})
{
  BabyApi::Tag outcome;

  doc.clear();

  doc["name"] = name;
  doc["colour"] = colour;

  serializeJson(doc, requestBody);

  String jsonBuffer;
  jsonBuffer = httpRequest(TAGS_ENDPOINT, "POST", "", "", requestBody);
  Serial.println(jsonBuffer);

  ResponseParser(jsonBuffer);

  response["name"].as<String>().toCharArray(outcome.name, 256);
  response["last_used"].as<String>().toCharArray(outcome.last_used, 33);
  response["color"].as<String>().toCharArray(outcome.color, 8);
  response["slug"].as<String>().toCharArray(outcome.slug, 101);

  return outcome;
}

BabyApi::Tag BabyApi::getTag(char * slug)
{
  BabyApi::Tag outcome;

  String parameters = "/" + String(slug) + "/";

  String jsonBuffer;
  jsonBuffer = httpRequest(TAGS_ENDPOINT, "GET", parameters.c_str());
  Serial.println(jsonBuffer);

  ResponseParser(jsonBuffer);

  response["name"].as<String>().toCharArray(outcome.name, 256);
  response["last_used"].as<String>().toCharArray(outcome.last_used, 33);
  response["color"].as<String>().toCharArray(outcome.color, 8);
  response["slug"].as<String>().toCharArray(outcome.slug, 101);

  return outcome;
}

BabyApi::Tag BabyApi::updateTag(
    char * slug,
    bool updateName = false,
    char * name = {},
    bool updateColour = false,
    char * colour = {})
{
  BabyApi::Tag outcome;

  doc.clear();

  if (updateName)
    doc["name"] = name;
  if (updateColour)
    doc["colour"] = colour;

  serializeJson(doc, requestBody);

  String parameters = "/" + String(slug) + "/";

  String jsonBuffer;
  jsonBuffer = httpRequest(TAGS_ENDPOINT, "PATCH", parameters.c_str(), "", requestBody);
  Serial.println(jsonBuffer);

  ResponseParser(jsonBuffer);

  response["name"].as<String>().toCharArray(outcome.name, 256);
  response["last_used"].as<String>().toCharArray(outcome.last_used, 33);
  response["color"].as<String>().toCharArray(outcome.color, 8);
  response["slug"].as<String>().toCharArray(outcome.slug, 101);

  return outcome;
}

bool BabyApi::removeTag(char * slug)
{
  String parameters = "/" + String(slug) + "/";
  int responseCode;
  String jsonBuffer;
  jsonBuffer = httpRequest(CHANGES_ENDPOINT, "DELETE", parameters.c_str(), "", "", &responseCode);
  Serial.println(jsonBuffer);

  return responseCode == 204;
}

BabyApi::searchResults<BabyApi::Temperature> BabyApi::findTemperatureRecords(
    uint16_t offset = 0,
    uint16_t child = 0,
    char * date = {},
    char * date_max = {},
    char * date_min = {},
    char * tags = {},
    char * ordering = {})
{
  BabyApi::searchResults<BabyApi::Temperature> outcome;
  uint16_t count = 0;

  snprintf(query,256,"limit=%d%s%s%s%s%s%s%s",
    SEARCH_LIMIT,
    offset > 0 ? ",offset=" + String(offset) : "",
    child > 0 ? ",child=" + String(child) : "",
    date[0] != '\0' ? ",date=" + String(date) : "",
    date_max[0] != '\0' ? "&date_max=" + String(date_max) : "",
    date_min[0] != '\0' ? "&date_min=" + String(date_min) : "",
    tags[0] != '\0' ? "&tags=" + String(tags) : "",
    ordering[0] != '\0' ? ",ordering=" + String(ordering) : ""
  );

  String jsonBuffer;
  jsonBuffer = httpRequest(TEMPERATURE_ENDPOINT, "GET", "", query);
  Serial.println(jsonBuffer);

  ResponseParser(jsonBuffer);

  searchResultParser(&outcome.count, &outcome.next, &outcome.previous);

  JsonArray results = response["results"].as<JsonArray>();

  for (JsonObject temperatureRecord : results)
  {
    outcome.results[count].id = temperatureRecord["id"];
    outcome.results[count].child = temperatureRecord["child"];
    temperatureRecord["time"].as<String>().toCharArray(outcome.results[count].time,33);
    outcome.results[count].temperature = temperatureRecord["temperature"];
    temperatureRecord["notes"].as<String>().toCharArray(outcome.results[count].notes,256);
    deserialiseTags(temperatureRecord["tags"].as<JsonArray>()).toCharArray(outcome.results[count].tags, 256);

    count++;
  }

  return outcome;
}

BabyApi::Temperature BabyApi::logTemperature(
    uint16_t child,
    float temperature,
    char * time,
    char * notes = {},
    char * tags = {})
{
  BabyApi::Temperature outcome;

  doc.clear();

  doc["child"] = child;
  doc["time"] = time;
  doc["temperature"] = temperature;
  doc["notes"] = notes;
  doc["tags"] = serialiseTags(tags);

  serializeJson(doc, requestBody);

  String jsonBuffer;
  jsonBuffer = httpRequest(TEMPERATURE_ENDPOINT, "POST", "", "", requestBody);
  Serial.println(jsonBuffer);

  ResponseParser(jsonBuffer);

  outcome.id = response["id"];
  outcome.child = response["child"];
  response["time"].as<String>().toCharArray(outcome.time,33);
  outcome.temperature = response["temperature"];
  response["notes"].as<String>().toCharArray(outcome.notes,256);
    deserialiseTags(response["tags"].as<JsonArray>()).toCharArray(outcome.tags, 256);

  return outcome;
}

BabyApi::Temperature BabyApi::getTemperature(uint16_t id)
{
  BabyApi::Temperature outcome;

  String parameters = "/" + String(id) + "/";

  String jsonBuffer;
  jsonBuffer = httpRequest(TEMPERATURE_ENDPOINT, "GET", parameters.c_str());
  Serial.println(jsonBuffer);

  ResponseParser(jsonBuffer);

  outcome.id = response["id"];
  outcome.child = response["child"];
  response["time"].as<String>().toCharArray(outcome.time,33);
  outcome.temperature = response["temperature"];
  response["notes"].as<String>().toCharArray(outcome.notes,256);
    deserialiseTags(response["tags"].as<JsonArray>()).toCharArray(outcome.tags, 256);

  return outcome;
}

BabyApi::Temperature BabyApi::updateTemperature(
    uint16_t id = 0,
    uint16_t child = 0,
    float temperature = NAN,
    char * time = {},
    bool updateNotes = false,
    char * notes = {},
    bool updateTags = false,
    char * tags = {})
{
  BabyApi::Temperature outcome;

  doc.clear();

  if (child > 0)
    doc["child"] = child;
  if (time != '\0')
    doc["time"] = time;
  if (!isnan(temperature))
    doc["temperature"] = temperature;
  if (updateNotes)
    doc["notes"] = notes;
  if (updateTags)
    doc["tags"] = serialiseTags(tags);

  serializeJson(doc, requestBody);

  String parameters = "/" + String(id) + "/";

  String jsonBuffer;
  jsonBuffer = httpRequest(TEMPERATURE_ENDPOINT, "PATCH", parameters.c_str(), "", requestBody);
  Serial.println(jsonBuffer);

  ResponseParser(jsonBuffer);

  outcome.id = response["id"];
  outcome.child = response["child"];
  response["time"].as<String>().toCharArray(outcome.time,33);
  outcome.temperature = response["temperature"];
  response["notes"].as<String>().toCharArray(outcome.notes,256);
    deserialiseTags(response["tags"].as<JsonArray>()).toCharArray(outcome.tags, 256);

  return outcome;
}

bool BabyApi::removeTemperature(uint16_t id)
{
  String parameters = "/" + String(id) + "/";
  int responseCode;
  String jsonBuffer;
  jsonBuffer = httpRequest(CHANGES_ENDPOINT, "DELETE", parameters.c_str(), "", "", &responseCode);
  Serial.println(jsonBuffer);

  return responseCode == 204;
}

BabyApi::searchResults<BabyApi::Timer> BabyApi::findTimers(
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
    const char * ordering = {})
{
  BabyApi::searchResults<BabyApi::Timer> outcome;
  uint16_t count = 0;


                (active[0] != '\0' ? "&active=" + String(active) : "") +
                (user > 0 ? "&user=" + String(user) : "") +

  snprintf(query,256,"limit=%d%s%s%s%s%s%s%s%s%s%s",
    SEARCH_LIMIT,
    offset > 0 ? ",offset=" + String(offset) : "",
    child > 0 ? ",child=" + String(child) : "",
    start[0] != '\0' ? ",start=" + String(start) : "",
    start_max[0] != '\0' ? "&start_max=" + String(start_max) : "",
    start_min[0] != '\0' ? "&start_min=" + String(start_min) : "",
    end[0] != '\0' ? ",end=" + String(start) : "",
    end_max[0] != '\0' ? "&end_max=" + String(start_max) : "",
    end_min[0] != '\0' ? "&end_min=" + String(start_min) : "",
    active[0] != '\0' ? "&active=" + String(active) : "",
    user > 0 ? "&user=" + String(user) : "",
    ordering[0] != '\0' ? ",ordering=" + String(ordering) : ""
  );

  String jsonBuffer;
  jsonBuffer = httpRequest(TIMERS_ENDPOINT, "GET", "", query);
  Serial.println(jsonBuffer);

  ResponseParser(jsonBuffer);

  searchResultParser(&outcome.count, &outcome.next, &outcome.previous);

  JsonArray results = response["results"].as<JsonArray>();

  for (JsonObject timerRecord : results)
  {
    outcome.results[count].id = timerRecord["id"];
    outcome.results[count].child = timerRecord["child"];
    timerRecord["duration"].as<String>().toCharArray(outcome.results[count].duration,17);
    timerRecord["end"].as<String>().toCharArray(outcome.results[count].end,33);
    outcome.results[count].user = timerRecord["user"];
    timerRecord["start"].as<String>().toCharArray(outcome.results[count].start,33);

    count++;
  }

  return outcome;
}

uint16_t BabyApi::startTimer(uint16_t childId, String name = {}, uint16_t timer = 0)
{
  BabyApi::Timer babyTimer;

  if (timer == 0 && name[0] != '\0')
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
    } while (results.next > 0 && timer == 0);
  }

  if (babyTimer.id > 0)
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
    uint16_t child)
{
  return createTimer(child, "");
}

BabyApi::Timer BabyApi::createTimer(
    uint16_t child,
    char * name)
{
  return createTimer(
      child,
      name,
      "");
}

BabyApi::Timer BabyApi::createTimer(
    uint16_t child,
    char * start)
{
  return createTimer(
      child,
      "",
      start);
}

BabyApi::Timer BabyApi::createTimer(
    uint16_t child,
    char * name,
    char * start)
{
  BabyApi::Timer outcome;

  doc.clear();

  doc["child"] = child;
  doc["name"] = name;
  doc["start"] = start;

  serializeJson(doc, requestBody);

  String jsonBuffer;
  jsonBuffer = httpRequest(TIMERS_ENDPOINT, "POST", "", "", requestBody);
  Serial.println(jsonBuffer);

  ResponseParser(jsonBuffer);

  outcome.id = response["id"];
  outcome.child = response["child"];
  response["duration"].as<String>().toCharArray(outcome.duration,17);
  response["end"].as<String>().toCharArray(outcome.end,33);
  outcome.user = response["user"];
  response["start"].as<String>().toCharArray(outcome.start,33);

  return outcome;
}

BabyApi::Timer BabyApi::getTimer(uint16_t id)
{
  BabyApi::Timer outcome;

  String parameters = "/" + String(id) + "/";

  String jsonBuffer;
  jsonBuffer = httpRequest(TIMERS_ENDPOINT, "GET", parameters.c_str());
  Serial.println(jsonBuffer);

  ResponseParser(jsonBuffer);

  outcome.id = response["id"];
  outcome.child = response["child"];
  response["duration"].as<String>().toCharArray(outcome.duration,17);
  response["end"].as<String>().toCharArray(outcome.end,33);
  outcome.user = response["user"];
  response["start"].as<String>().toCharArray(outcome.start,33);

  return outcome;
}

BabyApi::Timer BabyApi::updateTimer(
    uint16_t id,
    uint16_t child = 0,
    char * name = {},
    char * start = {},
    uint16_t user = 0)
{
  BabyApi::Timer outcome;

  doc.clear();

  if (child > 0)
    doc["child"] = child;
  if (!name != '\0')
    doc["name"] = name;
  if (!start != '\0')
    doc["start"] = start;
  if (user > 0)
    doc["user"] = user;

  serializeJson(doc, requestBody);

  String parameters = "/" + String(id) + "/";

  String jsonBuffer;
  jsonBuffer = httpRequest(TIMERS_ENDPOINT, "PATCH", parameters.c_str(), "", requestBody);
  Serial.println(jsonBuffer);

  ResponseParser(jsonBuffer);

  outcome.id = response["id"];
  outcome.child = response["child"];
  response["duration"].as<String>().toCharArray(outcome.duration,17);
  response["end"].as<String>().toCharArray(outcome.end,33);
  outcome.user = response["user"];
  response["start"].as<String>().toCharArray(outcome.start,33);

  return outcome;
}

bool BabyApi::removeTimer(uint16_t id)
{
  String parameters = "/" + String(id) + "/";
  int responseCode;
  String jsonBuffer;
  jsonBuffer = httpRequest(CHANGES_ENDPOINT, "DELETE", parameters.c_str(), "", "", &responseCode);
  Serial.println(jsonBuffer);

  return responseCode == 204;
}

BabyApi::Timer BabyApi::restartTimer(uint16_t id)
{
  BabyApi::Timer outcome;

  

  String parameters = "/" + String(id) + "/";

  String jsonBuffer;
  jsonBuffer = httpRequest(TIMERS_ENDPOINT, "PATCH", parameters.c_str(), "");
  Serial.println(jsonBuffer);

  ResponseParser(jsonBuffer);

  outcome.id = response["id"];
  outcome.child = response["child"];
  response["duration"].as<String>().toCharArray(outcome.duration,17);
  response["end"].as<String>().toCharArray(outcome.end,33);
  outcome.user = response["user"];
  response["start"].as<String>().toCharArray(outcome.start,33);

  return outcome;
}

BabyApi::Timer BabyApi::stopTimer(uint16_t id)
{
  BabyApi::Timer outcome;

  

  String parameters = "/" + String(id) + "/";

  String jsonBuffer;
  jsonBuffer = httpRequest(TIMERS_ENDPOINT, "PATCH", parameters.c_str(), "");
  Serial.println(jsonBuffer);

  ResponseParser(jsonBuffer);

  outcome.id = response["id"];
  outcome.child = response["child"];
  response["duration"].as<String>().toCharArray(outcome.duration,17);
  response["end"].as<String>().toCharArray(outcome.end,33);
  outcome.user = response["user"];
  response["start"].as<String>().toCharArray(outcome.start,33);

  return outcome;
}

BabyApi::searchResults<BabyApi::TummyTime> BabyApi::findTummyTimes(
    uint16_t offset = 0,
    uint16_t child = 0,
    char * start = {},
    char * start_max = {},
    char * start_min = {},
    char * end = {},
    char * end_max = {},
    char * end_min = {},
    char * tags = {},
    char * ordering = {})
{
  BabyApi::searchResults<BabyApi::TummyTime> outcome;
  uint16_t count = 0;

  snprintf(query,256,"limit=%d%s%s%s%s%s%s%s%s%s%s",
    SEARCH_LIMIT,
    offset > 0 ? ",offset=" + String(offset) : "",
    child > 0 ? ",child=" + String(child) : "",
    start[0] != '\0' ? ",start=" + String(start) : "",
    start_max[0] != '\0' ? "&start_max=" + String(start_max) : "",
    start_min[0] != '\0' ? "&start_min=" + String(start_min) : "",
    end[0] != '\0' ? ",end=" + String(start) : "",
    end_max[0] != '\0' ? "&end_max=" + String(start_max) : "",
    end_min[0] != '\0' ? "&end_min=" + String(start_min) : "",
    tags[0] != '\0' ? "&tags=" + String(tags) : "",
    ordering[0] != '\0' ? ",ordering=" + String(ordering) : ""
  );

  String jsonBuffer;
  jsonBuffer = httpRequest(TUMMY_TIMES_ENDPOINT, "GET", "", query);
  Serial.println(jsonBuffer);

  ResponseParser(jsonBuffer);

  searchResultParser( &outcome.count, &outcome.next, &outcome.previous);

  JsonArray results = response["results"].as<JsonArray>();

  for (JsonObject tummyTimeRecord : results)
  {
    outcome.results[count].id = tummyTimeRecord["id"];
    outcome.results[count].child = tummyTimeRecord["child"];
    tummyTimeRecord["duration"].as<String>().toCharArray(outcome.results[count].duration,17);
    tummyTimeRecord["end"].as<String>().toCharArray(outcome.results[count].end,33);
    tummyTimeRecord["milestone"].as<String>().toCharArray(outcome.results[count].milestone,256);
    tummyTimeRecord["start"].as<String>().toCharArray(outcome.results[count].start,33);
    deserialiseTags(tummyTimeRecord["tags"].as<JsonArray>()).toCharArray(outcome.results[count].tags, 256);

    count++;
  }

  return outcome;
}

BabyApi::TummyTime BabyApi::logTummyTime(
    uint16_t child = 0,          // Required unless a Timer value is provided.
    char * start = {}, // Required unless a Timer value is provided.
    char * end = {},   // Required unless a Timer value is provided.
    uint16_t timer = 0,          // May be used in place of the Start, End, and/or Child values.
    char * milestone = {},
    char * tags = {})
{
  BabyApi::TummyTime outcome;

  doc.clear();

  // if no tmer value present, child start and end are required fields
  if (timer == 0 && (child == 0 || start[0] != '\0' || end[0] != '\0'))
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

  if (child > 0)
    doc["child"] = child;
  doc["start"] = start;
  doc["end"] = end;
  if (timer > 0)
    doc["timer"] = timer;
  doc["milestone"] = milestone;
  doc["tags"] = serialiseTags(tags);

  serializeJson(doc, requestBody);

  String jsonBuffer;
  jsonBuffer = httpRequest(TUMMY_TIMES_ENDPOINT, "POST", "", "", requestBody);
  Serial.println(jsonBuffer);

  ResponseParser(jsonBuffer);

  outcome.id = response["id"];
  outcome.child = response["child"];
  response["duration"].as<String>().toCharArray(outcome.duration,17);
  response["end"].as<String>().toCharArray(outcome.end,33);
  response["milestone"].as<String>().toCharArray(outcome.milestone,256);
  response["start"].as<String>().toCharArray(outcome.start,33);
  deserialiseTags(response["tags"].as<JsonArray>()).toCharArray(outcome.tags, 256);

  return outcome;
}

BabyApi::TummyTime BabyApi::getTummyTime(uint16_t id)
{
  BabyApi::TummyTime outcome;

  String parameters = "/" + String(id) + "/";

  String jsonBuffer;
  jsonBuffer = httpRequest(TUMMY_TIMES_ENDPOINT, "GET", parameters.c_str());
  Serial.println(jsonBuffer);

  ResponseParser(jsonBuffer);

  outcome.id = response["id"];
  outcome.child = response["child"];
  response["duration"].as<String>().toCharArray(outcome.duration,17);
  response["end"].as<String>().toCharArray(outcome.end,33);
  response["milestone"].as<String>().toCharArray(outcome.milestone,256);
  response["start"].as<String>().toCharArray(outcome.start,33);
  deserialiseTags(response["tags"].as<JsonArray>()).toCharArray(outcome.tags, 256);

  return outcome;
}

BabyApi::TummyTime BabyApi::updateTummyTime(
    uint16_t id,
    uint16_t child = 0,
    char * start = {},
    char * end = {},
    bool updateMilestone = false,
    char * milestone = {},
    bool updateTags = false,
    char * tags = {})
{
  BabyApi::TummyTime outcome;

  doc.clear();

  if (child > 0)
    doc["child"] = child;
  if (start != '\0')
    doc["start"] = start;
  if (start != '\0')
    doc["end"] = end;
  if (updateMilestone)
    doc["milestone"] = milestone;
  if (updateTags)
    doc["tags"] = serialiseTags(tags);

  serializeJson(doc, requestBody);

  String parameters = "/" + String(id) + "/";

  String jsonBuffer;
  jsonBuffer = httpRequest(TUMMY_TIMES_ENDPOINT, "PATCH", parameters.c_str(), "", requestBody);
  Serial.println(jsonBuffer);

  ResponseParser(jsonBuffer);

  outcome.id = response["id"];
  outcome.child = response["child"];
  response["duration"].as<String>().toCharArray(outcome.duration,17);
  response["end"].as<String>().toCharArray(outcome.end,33);
  response["milestone"].as<String>().toCharArray(outcome.milestone,256);
  response["start"].as<String>().toCharArray(outcome.start,33);
  deserialiseTags(response["tags"].as<JsonArray>()).toCharArray(outcome.tags, 256);

  return outcome;
}

bool BabyApi::removeTummyTime(uint16_t id)
{
  String parameters = "/" + String(id) + "/";
  int responseCode;
  String jsonBuffer;
  jsonBuffer = httpRequest(CHANGES_ENDPOINT, "DELETE", parameters.c_str(), "", "", &responseCode);
  Serial.println(jsonBuffer);

  return responseCode == 204;
}

BabyApi::searchResults<BabyApi::Weight> BabyApi::findWeightRecords(
    uint16_t offset = 0,
    uint16_t child = 0,
    char * date = {},
    char * ordering = {})
{
  BabyApi::searchResults<BabyApi::Weight> outcome;
  uint16_t count = 0;

  snprintf(query,256,"limit=%d%s%s%s%s%s%s%s%s%s%s",
    SEARCH_LIMIT,
    offset > 0 ? ",offset=" + String(offset) : "",
    child > 0 ? ",child=" + String(child) : "",
    date[0] != '\0' ? ",date=" + String(date) : "",
    ordering[0] != '\0' ? ",ordering=" + String(ordering) : ""
  );

  String jsonBuffer;
  jsonBuffer = httpRequest(WEIGHT_ENDPOINT, "GET", "", query);
  Serial.println(jsonBuffer);

  ResponseParser(jsonBuffer);

  searchResultParser(&outcome.count, &outcome.next, &outcome.previous);

  JsonArray results = response["results"].as<JsonArray>();

  for (JsonObject weightRecord : results)
  {
    outcome.results[count].id = weightRecord["id"];
    outcome.results[count].child = weightRecord["child"];
    outcome.results[count].weight = weightRecord["weight"];
    weightRecord["date"].as<String>().toCharArray(outcome.results[count].date,33);
    weightRecord["notes"].as<String>().toCharArray(outcome.results[count].notes,256);
    deserialiseTags(weightRecord["tags"].as<JsonArray>()).toCharArray(outcome.results[count].tags, 256);

    count++;
  }

  return outcome;
}

BabyApi::Weight BabyApi::logWeight(
    uint16_t child,
    float weight,
    char * date,
    char * notes = {},
    char * tags = {})
{
  BabyApi::Weight outcome;

  doc.clear();

  doc["child"] = child;
  doc["date"] = date;
  doc["weight"] = weight;
  doc["notes"] = notes;
  doc["tags"] = serialiseTags(tags);

  serializeJson(doc, requestBody);

  String jsonBuffer;
  jsonBuffer = httpRequest(WEIGHT_ENDPOINT, "POST", "", "", requestBody);
  Serial.println(jsonBuffer);

  ResponseParser(jsonBuffer);

  outcome.id = response["id"];
  outcome.child = response["child"];
  response["date"].as<String>().toCharArray(outcome.date,33);
  outcome.weight = response["weight"];
  response["notes"].as<String>().toCharArray(outcome.notes,256);
    deserialiseTags(response["tags"].as<JsonArray>()).toCharArray(outcome.tags, 256);

  return outcome;
}

BabyApi::Weight BabyApi::getWeight(uint16_t id)
{
  BabyApi::Weight outcome;

  String parameters = "/" + String(id) + "/";

  String jsonBuffer;
  jsonBuffer = httpRequest(WEIGHT_ENDPOINT, "GET", parameters.c_str());
  Serial.println(jsonBuffer);

  ResponseParser(jsonBuffer);

  outcome.id = response["id"];
  outcome.child = response["child"];
  response["date"].as<String>().toCharArray(outcome.date,33);
  outcome.weight = response["weight"];
  response["notes"].as<String>().toCharArray(outcome.notes,256);
    deserialiseTags(response["tags"].as<JsonArray>()).toCharArray(outcome.tags, 256);

  return outcome;
}

BabyApi::Weight BabyApi::updateWeight(
    uint16_t id,
    uint16_t child = 0,
    float weight = NAN,
    char * date = {},
    bool updateNotes = false,
    char * notes = {},
    bool updateTags = false,
    char * tags = {})
{
  BabyApi::Weight outcome;

  doc.clear();

  if (child > 0)
    doc["child"] = child;
  if (date != '\0')
    doc["date"] = date;
  if (!isnan(weight))
    doc["weight"] = weight;
  if (updateNotes)
    doc["notes"] = notes;
  if (updateTags)
    doc["tags"] = serialiseTags(tags);

  serializeJson(doc, requestBody);

  String parameters = "/" + String(id) + "/";

  String jsonBuffer;
  jsonBuffer = httpRequest(WEIGHT_ENDPOINT, "PATCH", parameters.c_str(), "", requestBody);
  Serial.println(jsonBuffer);

  ResponseParser(jsonBuffer);

  outcome.id = response["id"];
  outcome.child = response["child"];
  response["date"].as<String>().toCharArray(outcome.date,33);
  outcome.weight = response["weight"];
  response["notes"].as<String>().toCharArray(outcome.notes,256);
  deserialiseTags(response["tags"].as<JsonArray>()).toCharArray(outcome.tags, 256);

  return outcome;
}

bool BabyApi::removeWeight(uint16_t id)
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

  ResponseParser(jsonBuffer);

  outcome.user.id = response["user"]["id"];
  response["user"]["username"].as<String>().toCharArray(outcome.user.username,151);
  response["user"]["first_name"].as<String>().toCharArray(outcome.user.first_name,151);
  response["user"]["last_name"].as<String>().toCharArray(outcome.user.last_name,151);
  response["user"]["email"].as<String>().toCharArray(outcome.user.email,151);
  outcome.user.is_staff = response["user"]["is_staff"];
  response["language"].as<String>().toCharArray(outcome.language,256);
  response["timezone"].as<String>().toCharArray(outcome.timezone,101);
  response["api_key"].as<String>().toCharArray(outcome.api_key,129);

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

uint8_t BabyApi::recordFeeding(uint16_t timerId, uint8_t feedingType, uint8_t feedingMethod, float amount)
{
  BabyApi::Feeding fed;

  fed = babyApi.logFeeding(timerId, feedingTypes[feedingType], feedingMethods[feedingMethod], amount); 
  
  return fed.id;
}

uint8_t BabyApi::recordSleep(uint16_t timerId)
{
  BabyApi::Sleep slept;

  slept = babyApi.logSleep(
      0,
      "",
      "",
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
      0,
      "",
      "",
      (int)timerId);

  return tummy.id;
}

uint8_t BabyApi::recordNappyChange(uint16_t child, bool wet, bool solid, uint16_t colour)
{
  BabyApi::DiaperChange changed;

  changed = babyApi.logDiaperChange(child,wet,solid,stoolColours[colour]);

  return changed.id;
}
