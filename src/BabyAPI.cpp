
#include "BabyAPI.h"

#include <sstream>

bool convertToJson(const tmElements_t& src, JsonVariant dst) {
  return dst.set(dateTime(makeTime(src.Hour,src.Minute,src.Second,src.Day,src.Month,src.Year),BB_DATE_FORMAT));
}

void convertFromJson(JsonVariantConst src, tm& dst) {
  strptime(src.as<const char*>(), BB_DATE_FORMAT, &dst);
}


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
  snprintf(babyApiKey,129, "Token %s", apiKey);
}

int BabyApi::httpRequest(
  const char * endpoint, 
  const char * type, 
  const char * parameters = {}, 
  const char * query = {}, 
  bool includeRequest = false)
{
  WiFiClientSecure client;
  HTTPClient https;

  char address[256];

  if(includeRequest)
  {
    serializeJson(doc, requestBody);
  }
  else
  {
    requestBody[0] = '\0';
  }

  snprintf(address, 256, "https://%s:%s%s%s/%s%s", getServerHost(), getServerPort(), ENDPOINT, endpoint, parameters, query);

  https.addHeader("Authorization", getApiKey());

  https.begin(client, address);

  // Send HTTP POST request
  int httpsResponseCode = https.sendRequest(type, requestBody);

  if (httpsResponseCode > 0)
  {
    Serial.print("HTTP Response code: ");
    Serial.println(httpsResponseCode);
    ResponseParser(https.getString().c_str());
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

void BabyApi::ResponseParser(const char * parse)
{
  
  DeserializationError err = deserializeJson(response, parse);
  if (err)
  {
    Serial.print(F("deserializeJson() failed with code "));
    Serial.println(err.f_str());
  }
}

void BabyApi::searchResultParser(long *count, long *next, long *previous)
{
  char * offsetLocation;


  *count = response["count"];

  offsetLocation = strstr(response["next"],"offset=");

  *next = (offsetLocation != NULL) ? strtol(response["next"][offsetLocation + 7],NULL,0) : 0;

  offsetLocation = strstr(response["previous"],"offset=");

  *next = (offsetLocation != NULL) ? strtol(response["previous"][offsetLocation + 7],NULL,0) : 0;
}

BabyApi::searchResults<BabyApi::BMI> BabyApi::findBMIRecords(
    uint16_t offset = 0,
    uint16_t child = 0,
    time_t date = 0,
    const char * ordering = {})
{
  BabyApi::searchResults<BabyApi::BMI> outcome;
  uint16_t count = 0;
  std::ostringstream stream;
    
  stream << "limit=" << SEARCH_LIMIT;
  if(offset > 0) stream << ",offset=%d" << offset;
  if(child > 0) stream << ",child=" << child;
  if(date > 0 ) stream << ",date=" << dateTime(date,BB_DATE_FORMAT);
  if(ordering[0] != '\0') stream << ",ordering=" << ordering;

  int httpsResponseCode = httpRequest(BMI_ENDPOINT, "GET", "", stream.str().c_str());
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
    outcome.results[count].date = bmiRecord["date"];
    outcome.results[count].bmi = bmiRecord["bmi"];
    outcome.results[count].notes = bmiRecord["notes"];
    copyArray(bmiRecord["tags"].as<JsonArray>(),outcome.results[count].tags);
 
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
  outcome.date = response["date"];
  outcome.bmi = response["bmi"];
  outcome.notes = response["notes"];
  copyArray(response["tags"].as<JsonArray>(),outcome.tags);

  return outcome;
}


BabyApi::BMI BabyApi::logBMI(
    uint16_t child,
    float bmi,
    time_t date,
    const char * notes = {},
    const char * tags[MAX_TAGS] = {})
{
  
  BabyApi::BMI outcome;
  
  doc.clear();

  doc["child"] = child;
  doc["date"] = dateTime(date, BB_DATE_FORMAT);
  doc["bmi"] = bmi;
  doc["notes"] = notes;
  copyArray(tags, doc["tags"]);

  int httpsResponseCode = httpRequest(BMI_ENDPOINT, "POST", "", "", true);
  Serial.println(httpsResponseCode);

  if(httpsResponseCode <= 0)
  {
    return outcome;
  }

  outcome.id = response["id"];
  outcome.child = response["child"];
  outcome.date = response["date"];
  outcome.bmi = response["bmi"];
  outcome.notes = response["notes"];
  copyArray(response["tags"].as<JsonArray>(), outcome.tags);

  return outcome;
}

BabyApi::BMI BabyApi::updateBMI(
    uint16_t id,
    uint16_t child = 0,
    float bmi = NAN,
    time_t date = 0,
    bool updateNotes = false,
    const char * notes = {},
    bool updateTags = false,
    const char * tags[MAX_TAGS] = {})
{
  
  BabyApi::BMI outcome;

  doc.clear();

  if (child > 0)
    doc["child"] = child;
  if (date > 0)
    doc["date"] = dateTime(date, BB_DATE_FORMAT);
  if (!isnan(bmi))
    doc["bmi"] = bmi;
  if (updateNotes)
    doc["notes"] = notes;
  if (updateTags)
    copyArray(tags, doc["tags"]);

  snprintf(parameters, 256,"/%d/",id);

  int httpsResponseCode = httpRequest(BMI_ENDPOINT, "PATCH", parameters, "", true);
  Serial.println(httpsResponseCode);

  if(httpsResponseCode <= 0)
  {
    return outcome;
  }

  outcome.id = response["id"];
  outcome.child = response["child"];
  outcome.date = response["date"];
  outcome.bmi = response["bmi"];
  outcome.notes = response["notes"];
  copyArray(response["tags"].as<JsonArray>(), outcome.tags);

  return outcome;
}

bool BabyApi::deleteBMI(uint16_t id)
{
  snprintf(parameters, 256,"/%d/",id);

  int responseCode = httpRequest(BMI_ENDPOINT, "DELETE", parameters);
  Serial.println(responseCode);

  return responseCode == 204;
}

BabyApi::searchResults<BabyApi::DiaperChange> BabyApi::findDiaperChanges(
    uint16_t offset = 0,
    uint16_t child = 0,
    const char * colour = {},
    time_t date = {},
    time_t date_max = {},
    time_t date_min = {},
    bool solid = {},
    bool wet = {},
    const char * tags[MAX_TAGS] = {},
    const char * ordering = {})
{
  BabyApi::searchResults<BabyApi::DiaperChange> outcome;
  uint16_t count = 0;
  
  std::ostringstream stream;

  stream << "limit=" << SEARCH_LIMIT;

  if(offset > 0)  stream << ",offset=" << offset;
  if(child > 0 ) stream << ",child=" << child;
  if(date > 0 ) stream << ",date=" << dateTime(date, BB_DATE_FORMAT);
  if(date_max > 0 ) stream << ",date_max=" << dateTime(date_max, BB_DATE_FORMAT);
  if(date_min > 0 ) stream << ",date_min=" << dateTime(date_min, BB_DATE_FORMAT);
  if(colour[0] != '\0' ) stream << ",colour=" << colour;
  if(solid ) stream << ",solid=true";
  if(wet ) stream << ",wet=true";
  if(tags[0][0] != '\0' )
  {
    stream << ",tags=" << tags;
    for(int i = 0; i < MAX_TAGS; i++)
    {
      if(i > 0) stream << ",";
      stream << tags[i];
    }
  } 
  if(ordering[0] != '\0' ) stream << ",ordering=" << ordering;
  

  int httpsResponseCode = httpRequest(CHANGES_ENDPOINT, "GET", "", stream.str().c_str());
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
    outcome.results[count].color = diaperChangeRecord["color"];
    outcome.results[count].solid = diaperChangeRecord["solid"];
    outcome.results[count].wet = diaperChangeRecord["wet"];
    outcome.results[count].time = diaperChangeRecord["time"];
    outcome.results[count].notes = diaperChangeRecord["notes"];
    copyArray(diaperChangeRecord["tags"].as<JsonArray>(),outcome.results[count].tags);

    count++;
  }

  return outcome;
}

BabyApi::DiaperChange BabyApi::logDiaperChange(
    uint16_t child,
    bool wet,
    bool solid,
    const char * color = {},
    float amount = NAN,
    const char * notes = {},
    const char * tags[MAX_TAGS] = {})
{
  return logDiaperChange(child, 0, wet, solid, color, amount, notes, tags);
}

BabyApi::DiaperChange BabyApi::logDiaperChange(
    uint16_t child,
    time_t time,
    bool wet = false,
    bool solid = false,
    const char * color = {},
    float amount = NAN,
    const char * notes = {},
    const char * tags[MAX_TAGS] = {})
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
  copyArray(tags, doc["tags"]);

  int httpsResponseCode = httpRequest(CHANGES_ENDPOINT, "POST", "", "", true);
  Serial.println(httpsResponseCode);

  if(httpsResponseCode <= 0)
  {
    return outcome;
  }

  outcome.id = response["id"];
  outcome.child = response["child"];
  outcome.amount = response["amount"];
  outcome.color = response["color"];
  outcome.solid = response["solid"];
  outcome.wet = response["wet"];
  outcome.time = response["time"];
  outcome.notes = response["notes"];

  copyArray(response["tags"].as<JsonArray>(),outcome.tags);

  return outcome;
}

BabyApi::DiaperChange BabyApi::getDiaperChange(uint16_t id)
{
  BabyApi::DiaperChange outcome;

  snprintf(parameters, 256,"/%d/",id);

  int httpsResponseCode = httpRequest(CHANGES_ENDPOINT, "GET", parameters);
  Serial.println(httpsResponseCode);

  if(httpsResponseCode <= 0)
  {
    return outcome;
  }

  outcome.id = response["id"];
  outcome.child = response["child"];
  outcome.amount = response["amount"];
  outcome.color = response["color"];
  outcome.solid = response["solid"];
  outcome.wet = response["wet"];
  outcome.time = response["time"];
  outcome.notes = response["notes"];

  copyArray(response["tags"].as<JsonArray>(),outcome.tags);

  return outcome;
}

BabyApi::DiaperChange BabyApi::updateDiaperChange(
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
    const char * tags[MAX_TAGS] = {})
{
  
  BabyApi::DiaperChange outcome;

  doc.clear();

  if (child > 0)
    doc["child"] = child;
  if (time > 0)
    doc["time"] = time;
  if (wet)
    doc["wet"] = wet;
  if (solid)
    doc["solid"] = solid;
  if (color[0] != '\0')
    doc["color"] = color;
  if (!isnan(amount))
    doc["amount"] = amount;
  if (updateNotes)
    doc["notes"] = notes;
  if (updateTags)
    copyArray(tags, doc["tags"]);

  snprintf(parameters, 256,"/%d/",id);

  int httpsResponseCode = httpRequest(CHANGES_ENDPOINT, "PATCH", parameters, "", true);
  Serial.println(httpsResponseCode);

  if(httpsResponseCode <= 0)
  {
    return outcome;
  }

  outcome.id = response["id"];
  outcome.child = response["child"];
  outcome.amount = response["amount"];
  outcome.color = response["color"];
  outcome.solid = response["solid"];
  outcome.wet = response["wet"];
  outcome.time = response["time"];
  outcome.notes = response["notes"];

  copyArray(response["tags"].as<JsonArray>(),outcome.tags);

  return outcome;
}

bool BabyApi::removeDiaperChange(uint16_t id)
{
  snprintf(parameters, 256,"/%d/",id);

  int httpsResponseCode = httpRequest(CHANGES_ENDPOINT, "DELETE", parameters);
  Serial.println(httpsResponseCode);

  return httpsResponseCode == 204;
}

BabyApi::searchResults<BabyApi::Child> BabyApi::findChildren(
    uint16_t offset = 0,
    const char * first_name = {},
    const char * last_name = {},
    time_t birth_date = 0,
    const char * slug = {},
    const char * ordering = {})
{
  BabyApi::searchResults<BabyApi::Child> outcome;
  uint16_t count = 0;
  std::ostringstream stream;

  stream << "limit=" << SEARCH_LIMIT;

  if( offset > 0 ) stream << ",offset=" << offset;
  if( first_name[0] != '\0' ) stream << ",first_name=" << first_name;
  if( last_name[0] != '\0' ) stream << ",last_name=" << last_name;
  if( birth_date > 0 ) stream << ",birth_date=" << dateTime(birth_date, BB_DATE_FORMAT);
  if( slug[0] != '\0' ) stream << ",slug=" << slug;
  if( ordering[0] != '\0' ) stream << ",ordering=" << ordering;

  int httpsResponseCode = httpRequest(CHILDREN_ENDPOINT, "GET", "", stream.str().c_str());
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
    outcome.results[count].first_name = childeRecord["first_name"];
    outcome.results[count].last_name = childeRecord["last_name"];
    outcome.results[count].birth_date = childeRecord["birth_date"];
    outcome.results[count].picture = childeRecord["picture"];
    outcome.results[count].slug = childeRecord["slug"];
    count++;
  }

  return outcome;
}

BabyApi::Child BabyApi::newChild(
    const char * first_name,
    const char * last_name,
    time_t birth_date,
    const char * picture = {})
{
  BabyApi::Child outcome;

  doc.clear();

  doc["first_name"] = first_name;
  doc["last_name"] = last_name;
  doc["birth_date"] = dateTime(birth_date, BB_DATE_FORMAT);
  doc["picture"] = picture;

  int httpsResponseCode = httpRequest(CHILDREN_ENDPOINT, "POST", "", "", true);
  Serial.println(httpsResponseCode);

  if(httpsResponseCode <= 0)
  {
    return outcome;
  }

  outcome.id = response["id"];
  outcome.first_name = response["first_name"];
  outcome.last_name = response["last_name"];
  outcome.birth_date = response["birth_date"];
  outcome.picture = response["picture"];
  outcome.slug = response["slug"];

  return outcome;
}

BabyApi::Child BabyApi::getChild(const char * slug)
{

  BabyApi::Child outcome;

  snprintf(parameters, 256,"/%s/",slug);

  int httpsResponseCode = httpRequest(CHILDREN_ENDPOINT, "GET", parameters);
  Serial.println(httpsResponseCode);

  if(httpsResponseCode <= 0)
  {
    return outcome;
  }

  outcome.id = response["id"];
  outcome.first_name = response["first_name"];
  outcome.last_name = response["last_name"];
  outcome.birth_date = response["birth_date"];
  outcome.picture = response["picture"];
  outcome.slug = response["slug"];

  return outcome;
}

BabyApi::Child BabyApi::updateChild(
    const char * slug,
    const char * first_name = {},
    const char * last_name = {},
    time_t birth_date = {},
    bool updatePicture = false,
    const char * picture = {})
{
  
  BabyApi::Child outcome;

  doc.clear();

  if (first_name[0] != '\0')
    doc["first_name"] = first_name;
  if (last_name[0] != '\0')
    doc["last_name"] = last_name;
  if (birth_date > 0)
    doc["birth_date"] = dateTime(birth_date, BB_DATE_FORMAT);
  if (updatePicture)
    doc["picture"] = picture;

  snprintf(parameters, 256,"/%s/",slug);

  int httpsResponseCode = httpRequest(CHILDREN_ENDPOINT, "PATCH", parameters, "", babyApi.requestBody);
  Serial.println(httpsResponseCode);

  if(httpsResponseCode <= 0)
  {
    return outcome;
  }

  outcome.id = response["id"];
  outcome.first_name = response["first_name"];
  outcome.last_name = response["last_name"];
  outcome.birth_date = response["birth_date"];
  outcome.picture = response["picture"];
  outcome.slug = response["slug"];

  return outcome;
}

bool BabyApi::removeChild(const char * slug)
{
  snprintf(parameters, 256,"/%s/",slug);

  int httpsResponseCode = httpRequest(CHILDREN_ENDPOINT, "DELETE", parameters);
  Serial.println(httpsResponseCode);

  return httpsResponseCode == 204;
}

BabyApi::searchResults<BabyApi::Feeding> BabyApi::findFeedingRecords(
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
    const char * ordering = {})
{
  BabyApi::searchResults<BabyApi::Feeding> outcome;
  uint16_t count = 0;
  std::ostringstream stream;

  stream << "limit=" << SEARCH_LIMIT;

  if( offset > 0 ) stream << ",offset=" << offset;
  if( child > 0 ) stream << ",child=" << child;
  if( start > 0 ) stream << ",start="<< dateTime(start,BB_DATE_FORMAT);
  if( start_max > 0 ) stream << ",start_max=" << dateTime(start_max,BB_DATE_FORMAT);
  if( start_min > 0 ) stream << ",start_min=" << dateTime(start_min,BB_DATE_FORMAT);
  if( end > 0 ) stream << ",end=" << dateTime(end,BB_DATE_FORMAT);
  if( end_max > 0 ) stream << ",end_max=" << dateTime(end_max,BB_DATE_FORMAT);
  if( end_min > 0 ) stream << ",end_min=" << dateTime(end_min,BB_DATE_FORMAT);
  if( type[0] != '\0' ) stream << ",type=" << type;
  if( method[0] != '\0' ) stream << ",method=" << method;
  if( tags[0] != '\0' ) stream << ",tags=" << tags;
  if( ordering[0] != '\0' ) stream << ",ordering=" << ordering;

  int httpsResponseCode = httpRequest(FEEDINGS_ENDPOINT, "GET", "", query);
  Serial.println(httpsResponseCode);

  if(httpsResponseCode <= 0)
  {
    return outcome;
  }

  searchResultParser(&outcome.count, &outcome.next, &outcome.previous);

  JsonArray results = response["results"].as<JsonArray>();

  for (JsonObject feedingRecord : results)
  {
    outcome.results[count].id = feedingRecord["id"];
    outcome.results[count].child = feedingRecord["child"].as<int>();
    outcome.results[count].amount = feedingRecord["amount"].as<float>();
    outcome.results[count].duration = feedingRecord["duration"];
    outcome.results[count].start = feedingRecord["start"];
    outcome.results[count].end = feedingRecord["end"];
    outcome.results[count].method = feedingRecord["method"];
    outcome.results[count].type = feedingRecord["type"];
    outcome.results[count].notes = feedingRecord["notes"];
    copyArray(feedingRecord["tags"].as<JsonArray>(),outcome.results[count].tags);

    count++;
  }

  return outcome;
}

BabyApi::Feeding BabyApi::logFeeding(
    uint16_t timer,
    const char * type,
    const char * method,
    float amount,
    const char * notes = {},
    const char * tags[MAX_TAGS] = {})
{
  return logFeeding(
      0,
      0,
      0,
      timer,
      type,
      method,
      amount,
      notes,
      tags);
}

BabyApi::Feeding BabyApi::logFeeding(
    uint16_t child,    // Required unless a Timer value is provided.
    time_t start, // Required unless a Timer value is provided.
    time_t end,   // Required unless a Timer value is provided.
    const char * type,
    const char * method,
    float amount = NAN,
    const char * notes = {},
    const char * tags[MAX_TAGS] = {})
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
    time_t start = {}, // Required unless a Timer value is provided.
    time_t end = {},   // Required unless a Timer value is provided.
    uint16_t timer = 0,          // May be used in place of the Start, End, and/or Child values.
    const char * type = {},
    const char * method = {},
    float amount = NAN,
    const char * notes = {},
    const char * tags[MAX_TAGS] = {})
{
  BabyApi::Feeding outcome;
  doc.clear();

  // if no tmer value present, child start and end are required fields
  if (timer == 0 && (child == 0 || start == 0  || end == 0 ))
  {
    Serial.println("Missing child, start and end, these are required if no timer id provided:");
    Serial.print("child: ");
    Serial.println(child);
    Serial.print("start: ");
    Serial.println(dateTime(start,BB_DATE_FORMAT));
    Serial.print("end: ");
    Serial.println(dateTime(end,BB_DATE_FORMAT));
    return outcome;
  }

  if (child > 0)
    doc["child"] = child;
  if (child > 0)
    doc["start"] = dateTime(start,BB_DATE_FORMAT);
  if (child > 0)
    doc["end"] = dateTime(end,BB_DATE_FORMAT);
  if (timer > 0)
    doc["timer"] = timer;
  doc["type"] = type;
  doc["method"] = method;
  if (!isnan(amount))
    doc["amount"] = amount;
  doc["notes"] = notes;
  copyArray(tags, doc["tags"]);

  int httpsResponseCode = httpRequest(FEEDINGS_ENDPOINT, "POST", "", "", true);
  Serial.println(httpsResponseCode);

  if(httpsResponseCode <= 0)
  {
    return outcome;
  }

  outcome.id = response["id"];
  outcome.child = response["child"].as<int>();
  outcome.amount = response["amount"].as<float>();
  outcome.duration = response["duration"];
  outcome.start = response["start"];
  outcome.end = response["end"];
  outcome.method = response["method"];
  outcome.type = response["type"];
  outcome.notes = response["notes"];
  copyArray(response["tags"].as<JsonArray>(),outcome.tags);

  return outcome;
}

BabyApi::Feeding BabyApi::getFeeding(uint16_t id)
{
  BabyApi::Feeding outcome;

  snprintf(parameters, 256,"/%d/",id);

  int httpsResponseCode = httpRequest(FEEDINGS_ENDPOINT, "GET", parameters);
  Serial.println(httpsResponseCode);

  if(httpsResponseCode <= 0)
  {
    return outcome;
  }

  outcome.id = response["id"];
  outcome.child = response["child"].as<int>();
  outcome.amount = response["amount"].as<float>();
  outcome.duration = response["duration"];
  outcome.start = response["start"];
  outcome.end = response["end"];
  outcome.method = response["method"];
  outcome.type = response["type"];
  outcome.notes = response["notes"];
  copyArray(response["tags"].as<JsonArray>(),outcome.tags);

  return outcome;
}

BabyApi::Feeding BabyApi::updateFeeding(
    uint16_t id,
    uint16_t child = 0,          // Required unless a Timer value is provided.
    time_t start = {}, // Required unless a Timer value is provided.
    time_t end = {},   // Required unless a Timer value is provided.
    const char * method = {},
    const char * type = {},
    float amount = NAN,
    bool updateNotes = false,
    const char * notes = {},
    bool updateTags = false,
    const char * tags[MAX_TAGS] = {})
{
  BabyApi::Feeding outcome;
  
  doc.clear();

  if (child > 0)
    doc["child"] = child;
  if (start > 0)
    doc["start"] = dateTime(start,BB_DATE_FORMAT);
  if (end > 0)
    doc["end"] = dateTime(end,BB_DATE_FORMAT);
  if (type[0] != '\0')
    doc["type"] = type;
  if (method[0] != '\0')
    doc["method"] = method;
  if (!isnan(amount))
    doc["amount"] = amount;
  if (updateNotes)
    doc["notes"] = notes;
  if (updateTags)
    copyArray(tags, doc["tags"]);

  

  snprintf(parameters, 256,"/%d/",id);

  int httpsResponseCode = httpRequest(FEEDINGS_ENDPOINT, "PATCH", parameters, "", true);
  Serial.println(httpsResponseCode);

  if(httpsResponseCode <= 0)
  {
    return outcome;
  }

  outcome.id = response["id"];
  outcome.child = response["child"].as<int>();
  outcome.amount = response["amount"].as<float>();
  outcome.duration = response["duration"];
  outcome.start = response["start"];
  outcome.end = response["end"];
  outcome.method = response["method"];
  outcome.type = response["type"];
  outcome.notes = response["notes"];
  copyArray(response["tags"].as<JsonArray>(),outcome.tags);

  return outcome;
}

bool BabyApi::removeFeeding(uint16_t id)
{
  snprintf(parameters, 256,"/%d/",id);

  int httpsResponseCode = httpRequest(CHANGES_ENDPOINT, "DELETE", parameters);
  Serial.println(httpsResponseCode);

  return httpsResponseCode == 204;
}

BabyApi::searchResults<BabyApi::HeadCircumference> BabyApi::findHeadCircumferenceRecords(
    uint16_t offset = 0,
    uint16_t child = 0,
    time_t date = {},
    const char * ordering = {})
{
  BabyApi::searchResults<BabyApi::HeadCircumference> outcome;
  uint16_t count = 0;
  std::ostringstream stream;

  stream << "limit=" << SEARCH_LIMIT;

  if( offset > 0 ) stream << ",offset=" << offset;
  if( child > 0 ) stream << ",child=" << child;
  if( date > 0 ) stream << ",date=" + dateTime(date, BB_DATE_FORMAT);
  if( ordering[0] != '\0' ) stream << ",ordering=" << ordering;

  int httpsResponseCode = httpRequest(HEAD_CIRCUMFERENCE_ENDPOINT, "GET", "", query);
  Serial.println(httpsResponseCode);

  if(httpsResponseCode <= 0)
  {
    return outcome;
  }

  searchResultParser(&outcome.count, &outcome.next, &outcome.previous);

  JsonArray results = response["results"].as<JsonArray>();

  for (JsonObject headCircumferenceRecord : results)
  {
    outcome.results[count].id = headCircumferenceRecord["id"];
    outcome.results[count].child = headCircumferenceRecord["child"];
    outcome.results[count].date = response["date"];
    outcome.results[count].head_circumference = headCircumferenceRecord["head_circumference"];
    outcome.results[count].notes = headCircumferenceRecord["notes"];
    copyArray(headCircumferenceRecord["tags"].as<JsonArray>(),outcome.results[count].tags);

    count++;
  }

  return outcome;
}

BabyApi::HeadCircumference BabyApi::logHeadCircumference(
    uint16_t child,
    float head_circumference,
    time_t date,
    const char * notes = {},
    const char * tags[MAX_TAGS] = {})
{
  BabyApi::HeadCircumference outcome;

  doc.clear();

  doc["child"] = child;
  doc["date"] = dateTime(date, BB_DATE_FORMAT);
  doc["head_circumference"] = head_circumference;
  doc["notes"] = notes;
  copyArray(tags, doc["tags"]);

  int httpsResponseCode = httpRequest(HEAD_CIRCUMFERENCE_ENDPOINT, "POST", "", "", true);
  Serial.println(httpsResponseCode);

  if(httpsResponseCode <= 0)
  {
    return outcome;
  }

  outcome.id = response["id"];
  outcome.child = response["child"];
  outcome.date = response["date"];
  outcome.head_circumference = response["head_circumference"];
  outcome.notes = response["notes"];
  copyArray(response["tags"].as<JsonArray>(),outcome.tags);

  return outcome;
}

BabyApi::HeadCircumference BabyApi::getHeadCircumference(uint16_t id)
{
  BabyApi::HeadCircumference outcome;

  snprintf(parameters, 256,"/%d/",id);

  int httpsResponseCode = httpRequest(HEAD_CIRCUMFERENCE_ENDPOINT, "GET", parameters);
  Serial.println(httpsResponseCode);

  if(httpsResponseCode <= 0)
  {
    return outcome;
  }

  outcome.id = response["id"];
  outcome.child = response["child"];
  outcome.date = response["date"];
  outcome.head_circumference = response["head_circumference"];
  outcome.notes = response["notes"];
  copyArray(response["tags"].as<JsonArray>(),outcome.tags);

  return outcome;
}

BabyApi::HeadCircumference BabyApi::updateHeadCircumference(
    uint16_t id,
    uint16_t child = 0,
    float head_circumference = NAN,
    time_t date = {},
    bool updateNotes = false,
    const char * notes = {},
    bool updateTags = false,
    const char * tags[MAX_TAGS] = {})
{
  BabyApi::HeadCircumference outcome;

  doc.clear();

  if (child > 0)
    doc["child"] = child;
  if (date > 0)
    doc["date"] = dateTime(date, BB_DATE_FORMAT);
  if (!isnan(head_circumference))
    doc["head_circumference"] = head_circumference;
  if (updateNotes)
    doc["notes"] = notes;
  if (updateTags)
    copyArray(tags, doc["tags"]);

  

  snprintf(parameters, 256,"/%d/",id);

  int httpsResponseCode = httpRequest(HEAD_CIRCUMFERENCE_ENDPOINT, "PATCH", parameters, "", true);
  Serial.println(httpsResponseCode);

  if(httpsResponseCode <= 0)
  {
    return outcome;
  }

  outcome.id = response["id"];
  outcome.child = response["child"];
  outcome.date = response["date"];
  outcome.head_circumference = response["head_circumference"];
  outcome.notes = response["notes"];
  copyArray(response["tags"].as<JsonArray>(), outcome.tags);

  return outcome;
}

bool BabyApi::removeHeadCircumference(uint16_t id)
{
  snprintf(parameters, 256,"/%d/",id);
  
  int httpsResponseCode = httpRequest(HEAD_CIRCUMFERENCE_ENDPOINT, "DELETE", parameters);
  Serial.println(httpsResponseCode);

  return httpsResponseCode == 204;
}

BabyApi::searchResults<BabyApi::Height> BabyApi::findHeightRecords(
    uint16_t offset = 0,
    uint16_t child = 0,
    time_t date = {},
    const char * ordering = {})
{
  BabyApi::searchResults<BabyApi::Height> outcome;
  uint16_t count = 0;
  std::ostringstream stream;

  stream << "limit=" << SEARCH_LIMIT;
  if( offset > 0 ) stream << ",offset=" << offset;
  if( child > 0 ) stream << ",child=" << child;
  if( date > 0 ) stream << ",date=" << dateTime(date, BB_DATE_FORMAT);
  if( ordering[0] != '\0' ) stream << ",ordering=" << ordering;

  int httpsResponseCode = httpRequest(HEIGHT_ENDPOINT, "GET", "", stream.str().c_str());
  Serial.println(httpsResponseCode);

  if(httpsResponseCode <= 0)
  {
    return outcome;
  }

  searchResultParser(&outcome.count, &outcome.next, &outcome.previous);

  JsonArray results = response["results"].as<JsonArray>();

  for (JsonObject heightRecord : results)
  {
    outcome.results[count].id = heightRecord["id"];
    outcome.results[count].child = heightRecord["child"];
    outcome.results[count].date = heightRecord["date"];
    outcome.results[count].height = heightRecord["height"];
    outcome.results[count].notes = heightRecord["notes"];
    copyArray(heightRecord["tags"].as<JsonArray>(),outcome.results[count].tags);

    count++;
  }

  return outcome;
}

BabyApi::Height BabyApi::logHeight(
    uint16_t child,
    float height,
    time_t date,
    const char * notes = {},
    const char * tags[MAX_TAGS] = {})
{
  BabyApi::Height outcome;

  doc.clear();

  doc["child"] = child;
  doc["date"] = dateTime(date,BB_DATE_FORMAT);
  doc["height"] = height;
  doc["notes"] = notes;
  copyArray(tags, doc["tags"]);

  

  int httpsResponseCode = httpRequest(HEIGHT_ENDPOINT, "POST", "", "", true);
  Serial.println(httpsResponseCode);

  if(httpsResponseCode <= 0)
  {
    return outcome;
  }

  outcome.id = response["id"];
  outcome.child = response["child"];
  outcome.date = response["date"];
  outcome.height = response["height"];
  outcome.notes = response["notes"];
  copyArray(response["tags"].as<JsonArray>(),outcome.tags);

  return outcome;
}

BabyApi::Height BabyApi::getHeight(uint16_t id)
{
  BabyApi::Height outcome;

  snprintf(parameters, 256,"/%d/",id);

  int httpsResponseCode = httpRequest(HEIGHT_ENDPOINT, "GET", parameters);
  Serial.println(httpsResponseCode);

  if(httpsResponseCode <= 0)
  {
    return outcome;
  }

  outcome.id = response["id"];
  outcome.child = response["child"];
  outcome.date = response["date"];
  outcome.height = response["height"];
  outcome.notes = response["notes"];
  copyArray(response["tags"].as<JsonArray>(),outcome.tags);

  return outcome;
}

BabyApi::Height BabyApi::updateHeight(
    uint16_t id,
    uint16_t child = 0,
    float height = NAN,
    time_t date = {},
    bool updateNotes = false,
    const char * notes = {},
    bool updateTags = false,
    const char * tags[MAX_TAGS] = {})
{
  Height outcome;

  doc.clear();

  if (child > 0)
    doc["child"] = child;
  if (date > 0)
    doc["date"] = dateTime(date, BB_DATE_FORMAT);
  if (!isnan(height))
    doc["height"] = height;
  if (updateNotes)
    doc["notes"] = notes;
  if (updateTags)
    copyArray(tags, doc["tags"]);

  

  snprintf(parameters, 256,"/%d/",id);

  int httpsResponseCode = httpRequest(HEIGHT_ENDPOINT, "PATCH", parameters, "", true);
  Serial.println(httpsResponseCode);

  if(httpsResponseCode <= 0)
  {
    return outcome;
  }

  outcome.id = response["id"];
  outcome.child = response["child"];
  outcome.date = response["date"];
  outcome.height = response["height"];
  outcome.notes = response["notes"];
  copyArray(response["tags"].as<JsonArray>(),outcome.tags);

  return outcome;
}

bool BabyApi::removeHeight(uint16_t id)
{
  snprintf(parameters, 256,"/%d/",id);

  int httpsResponseCode = httpRequest(HEIGHT_ENDPOINT, "DELETE", parameters);
  Serial.println(httpsResponseCode);

  return httpsResponseCode == 204;
}

BabyApi::searchResults<BabyApi::Note> BabyApi::findNotes(
    uint16_t offset = 0,
    uint16_t child = 0,
    time_t date = {},
    time_t date_max = {},
    time_t date_min = {},
    const char * tags[MAX_TAGS] = {},
    const char * ordering = {})
{
  BabyApi::searchResults<BabyApi::Note> outcome;
  uint16_t count = 0;
  std::ostringstream stream;

  stream << "limit=" << SEARCH_LIMIT;
  if( offset > 0 ) stream << ",offset=" << offset;
  if( child > 0 ) stream << ",child=" << child;
  if( date > 0  ) stream << ",date=" << dateTime(date,BB_DATE_FORMAT);
  if( date_max > 0  ) stream << ",date_max=" << dateTime(date_max,BB_DATE_FORMAT);
  if( date_min > 0  ) stream << ",date_min=" << dateTime(date_min,BB_DATE_FORMAT);
  if( tags[0] != '\0' ) stream << ",tags=" << tags;
  if( ordering[0] != '\0' ) stream << ",ordering=" << ordering;

  int httpsResponseCode = httpRequest(NOTES_ENDPOINT, "GET", "", query);
  Serial.println(httpsResponseCode);

  if(httpsResponseCode <= 0)
  {
    return outcome;
  }

  searchResultParser(&outcome.count, &outcome.next, &outcome.previous);

  JsonArray results = response["results"].as<JsonArray>();

  for (JsonObject noteRecord : results)
  {
    outcome.results[count].id = noteRecord["id"];
    outcome.results[count].child = noteRecord["child"];
    outcome.results[count].date = noteRecord["date"];
    outcome.results[count].note = noteRecord["note"];
    copyArray(noteRecord["tags"].as<JsonArray>(),outcome.results[count].tags);

    count++;
  }

  return outcome;
}

BabyApi::Note BabyApi::createNote(
    uint16_t child,
    const char * note,
    time_t date,
    const char * tags[MAX_TAGS] = {})
{
  BabyApi::Note outcome;

  doc.clear();

  doc["child"] = child;
  doc["date"] = dateTime(date,BB_DATE_FORMAT);
  doc["note"] = note;
  copyArray(tags, doc["tags"]);

  int httpsResponseCode = httpRequest(NOTES_ENDPOINT, "POST", "", "", true);
  Serial.println(httpsResponseCode);

  if(httpsResponseCode <= 0)
  {
    return outcome;
  }

  outcome.id = response["id"];
  outcome.child = response["child"];
  outcome.date = response["date"];
  outcome.note = response["note"];
  copyArray(response["tags"].as<JsonArray>(),outcome.tags);

  return outcome;
}

BabyApi::Note BabyApi::getNote(uint16_t id)
{
  BabyApi::Note outcome;

  snprintf(parameters, 256,"/%d/",id);

  int httpsResponseCode = httpRequest(NOTES_ENDPOINT, "GET", parameters);
  Serial.println(httpsResponseCode);

  if(httpsResponseCode <= 0)
  {
    return outcome;
  }

  outcome.id = response["id"];
  outcome.child = response["child"];
  outcome.date = response["date"];
  outcome.note = response["note"];
  copyArray(response["tags"].as<JsonArray>(),outcome.tags);

  return outcome;
}

BabyApi::Note BabyApi::updateNote(
    uint16_t id,
    uint16_t child = 0,
    time_t date = {},
    bool updateNote = false,
    const char * note = {},
    bool updateTags = false,
    const char * tags[MAX_TAGS] = {})
{
  BabyApi::Note outcome;

  doc.clear();

  if (child > 0)
    doc["child"] = child;
  if (date > 0)
    doc["date"] = dateTime(date,BB_DATE_FORMAT);
  if (updateNote)
    doc["note"] = note;
  if (updateTags)
    copyArray(tags, doc["tags"]);

  snprintf(parameters, 256,"/%d/",id);

  int httpsResponseCode = httpRequest(NOTES_ENDPOINT, "PATCH", parameters, "", true);
  Serial.println(httpsResponseCode);

  if(httpsResponseCode <= 0)
  {
    return outcome;
  }

  outcome.id = response["id"];
  outcome.child = response["child"];
  outcome.date = response["date"];
  outcome.note = response["note"];
  copyArray(response["tags"].as<JsonArray>(),outcome.tags);

  return outcome;
}

bool BabyApi::removeNote(uint16_t id)
{
  snprintf(parameters, 256,"/%d/",id);

  int httpsResponseCode = httpRequest(NOTES_ENDPOINT, "DELETE", parameters);
  Serial.println(httpsResponseCode);

  return httpsResponseCode == 204;
}

BabyApi::searchResults<BabyApi::Pumping> BabyApi::findPumpingRecords(
    uint16_t offset = 0,
    uint16_t child = 0,
    time_t date = {},
    time_t date_max = {},
    time_t date_min = {},
    const char * ordering = {})
{
  BabyApi::searchResults<BabyApi::Pumping> outcome;
  uint16_t count = 0;
  std::ostringstream stream;

  stream << "limit=" << SEARCH_LIMIT;
  if( offset > 0 ) stream << ",offset=" << offset;
  if( child > 0 ) stream << ",child=" << child;
  if( date > 0  ) stream << ",date=" << dateTime(date,BB_DATE_FORMAT);
  if( date_max > 0  ) stream << ",date_max=" << dateTime(date_max,BB_DATE_FORMAT);
  if( date_min > 0  ) stream << ",date_min=" << dateTime(date_min,BB_DATE_FORMAT);
  if( ordering[0] != '\0' ) stream << ",ordering=" << ordering;

  int httpsResponseCode = httpRequest(PUMPING_ENDPOINT, "GET", "", query);
  Serial.println(httpsResponseCode);

  if(httpsResponseCode <= 0)
  {
    return outcome;
  }

  searchResultParser(&outcome.count, &outcome.next, &outcome.previous);

  JsonArray results = response["results"].as<JsonArray>();

  for (JsonObject pumpingRecord : results)
  {
    outcome.results[count].id = pumpingRecord["id"];
    outcome.results[count].child = pumpingRecord["child"];
    outcome.results[count].time = pumpingRecord["time"];
    outcome.results[count].amount = pumpingRecord["amount"];
    outcome.results[count].notes = pumpingRecord["notes"];
    copyArray(pumpingRecord["tags"].as<JsonArray>(),outcome.results[count].tags);

    count++;
  }

  return outcome;
}

BabyApi::Pumping BabyApi::logPumping(
    uint16_t child,
    float amount,
    time_t time = {},
    const char * notes = {},
    const char * tags[MAX_TAGS] = {})
{
  BabyApi::Pumping outcome;

  doc.clear();

  doc["child"] = child;
  doc["time"] = dateTime(time,BB_DATE_FORMAT);
  doc["amount"] = amount;
  doc["notes"] = notes;
  copyArray(tags, doc["tags"]);

  

  int httpsResponseCode = httpRequest(PUMPING_ENDPOINT, "POST", "", "", true);
  Serial.println(httpsResponseCode);

  if(httpsResponseCode <= 0)
  {
    return outcome;
  }

  outcome.id = response["id"];
  outcome.child = response["child"];
  outcome.time = response["time"];
  outcome.amount = response["amount"];
  outcome.notes = response["notes"];
  copyArray(response["tags"].as<JsonArray>(),outcome.tags);

  return outcome;
}

BabyApi::Pumping BabyApi::getPumping(uint16_t id)
{
  BabyApi::Pumping outcome;

  snprintf(parameters, 256,"/%d/",id);

  int httpsResponseCode = httpRequest(PUMPING_ENDPOINT, "GET", parameters);
  Serial.println(httpsResponseCode);

  if(httpsResponseCode <= 0)
  {
    return outcome;
  }

  outcome.id = response["id"];
  outcome.child = response["child"];
  outcome.time = response["time"];
  outcome.amount = response["amount"];
  outcome.notes = response["notes"];
  copyArray(response["tags"].as<JsonArray>(),outcome.tags);

  return outcome;
}

BabyApi::Pumping BabyApi::updatePumping(
    uint16_t id,
    uint16_t child = 0,
    float amount = NAN,
    time_t time = {},
    bool updateNotes = false,
    const char * notes = {},
    bool updateTags = false,
    const char * tags[MAX_TAGS] = {})
{
  BabyApi::Pumping outcome;

  doc.clear();

  if (child > 0)
    doc["child"] = child;
  if (time > 0)
    doc["time"] = dateTime(time,BB_DATE_FORMAT);;
  if (!isnan(amount))
    doc["amount"] = amount;
  if (updateNotes)
    doc["notes"] = notes;
  if (updateTags)
    copyArray(tags, doc["tags"]);

  snprintf(parameters, 256, "/%d/", id);

  int httpsResponseCode = httpRequest(PUMPING_ENDPOINT, "PATCH", parameters, "", true);
  Serial.println(httpsResponseCode);

  if(httpsResponseCode <= 0)
  {
    return outcome;
  }

  outcome.id = response["id"];
  outcome.child = response["child"];
  outcome.time = response["time"];
  outcome.amount = response["amount"];
  outcome.notes = response["notes"];
  copyArray(response["tags"].as<JsonArray>(),outcome.tags);

  return outcome;
}

bool BabyApi::removePumping(uint16_t id)
{
  snprintf(parameters, 256,"/%d/",id);

  int httpsResponseCode = httpRequest(PUMPING_ENDPOINT, "DELETE", parameters);
  Serial.println(httpsResponseCode);

  return httpsResponseCode == 204;
}

BabyApi::searchResults<BabyApi::Sleep> BabyApi::findSleepRecords(
    uint16_t offset = 0,
    uint16_t child = 0,
    time_t start = {},
    time_t start_max = {},
    time_t start_min = {},
    time_t end = {},
    time_t end_max = {},
    time_t end_min = {},
    const char * tags[MAX_TAGS] = {},
    const char * ordering = {})
{
  BabyApi::searchResults<BabyApi::Sleep> outcome;
  uint16_t count = 0;
  std::ostringstream stream;

  stream << "limit=" << SEARCH_LIMIT;

  if( offset > 0 ) stream << ",offset=" << offset;
  if( child > 0 ) stream << ",child=" << child;
  if( start > 0 ) stream << ",start="<< dateTime(start,BB_DATE_FORMAT);
  if( start_max > 0 ) stream << ",start_max=" << dateTime(start_max,BB_DATE_FORMAT);
  if( start_min > 0 ) stream << ",start_min=" << dateTime(start_min,BB_DATE_FORMAT);
  if( end > 0 ) stream << ",end=" << dateTime(end,BB_DATE_FORMAT);
  if( end_max > 0 ) stream << ",end_max=" << dateTime(end_max,BB_DATE_FORMAT);
  if( end_min > 0 ) stream << ",end_min=" << dateTime(end_min,BB_DATE_FORMAT);
  if( tags[0] != '\0' ) stream << ",tags=" << tags;
  if( ordering[0] != '\0' ) stream << ",ordering=" << ordering;

  int httpsResponseCode = httpRequest(SLEEP_ENDPOINT, "GET", "", query);
  Serial.println(httpsResponseCode);

  if(httpsResponseCode <= 0)
  {
    return outcome;
  }

  searchResultParser(&outcome.count, &outcome.next, &outcome.previous);

  JsonArray results = response["results"].as<JsonArray>();

  for (JsonObject sleepingRecord : results)
  {
    outcome.results[count].id = sleepingRecord["id"];
    outcome.results[count].child = sleepingRecord["child"];
    outcome.results[count].nap = sleepingRecord["nap"].as<String>() == "True";
    outcome.results[count].start = sleepingRecord["start"];
    outcome.results[count].end = sleepingRecord["end"];
    outcome.results[count].duration = sleepingRecord["duration"];
    outcome.results[count].notes = sleepingRecord["notes"];
    copyArray(sleepingRecord["tags"].as<JsonArray>(),outcome.results[count].tags);

    count++;
  }

  return outcome;
}

BabyApi::Sleep BabyApi::logSleep(
    uint16_t child,          // Required unless a Timer value is provided.
    time_t start = {}, // Required unless a Timer value is provided.
    time_t end = {},   // Required unless a Timer value is provided.
    uint16_t timer = 0,          // May be used in place of the Start, End, and/or Child values.
    const char * notes = {},
    const char * tags[MAX_TAGS] = {})
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
    Serial.println(dateTime(start,BB_DATE_FORMAT));
    Serial.print("end: ");
    Serial.println(dateTime(end,BB_DATE_FORMAT));
    return outcome;
  }

  if (child > 0)
    doc["child"] = child;
  doc["start"] = dateTime(start,BB_DATE_FORMAT);
  doc["end"] = dateTime(end,BB_DATE_FORMAT);
  if (timer > 0)
    doc["timer"] = timer;
  doc["notes"] = notes;
  copyArray(tags, doc["tags"]);

  int httpsResponseCode = httpRequest(SLEEP_ENDPOINT, "POST", "", "", true);
  Serial.println(httpsResponseCode);

  if(httpsResponseCode <= 0)
  {
    return outcome;
  }

  outcome.id = response["id"];
  outcome.child = response["child"];
  outcome.nap = response["nap"].as<String>() == "True";
  outcome.start = response["start"];
  outcome.end = response["end"];
  outcome.notes = response["notes"];
  copyArray(response["tags"].as<JsonArray>(),outcome.tags);

  return outcome;
}

BabyApi::Sleep BabyApi::getSleep(uint16_t id)
{
  Sleep outcome;

  snprintf(parameters, 256,"/%d/",id);

  int httpsResponseCode = httpRequest(SLEEP_ENDPOINT, "GET", parameters);
  Serial.println(httpsResponseCode);

  if(httpsResponseCode <= 0)
  {
    return outcome;
  }

  outcome.id = response["id"];
  outcome.child = response["child"];
  outcome.nap = response["nap"].as<String>() == "True";
  outcome.start = response["start"];
  outcome.end = response["end"];
  outcome.duration = response["duration"];
  outcome.notes = response["notes"];
  copyArray(response["tags"].as<JsonArray>(),outcome.tags);

  return outcome;
}

BabyApi::Sleep BabyApi::updateSleep(
    uint16_t id,
    uint16_t child = 0,
    time_t start = {},
    time_t end = {},
    bool updateNotes = false,
    const char * notes = {},
    bool updateTags = false,
    const char * tags[MAX_TAGS] = {})
{
  Sleep outcome;

  doc.clear();

  if (child > 0)
    doc["child"] = child;
  if (start != '\0')
    doc["start"] = dateTime(start, BB_DATE_FORMAT);
  if (end != '\0')
    doc["end"] = dateTime(end, BB_DATE_FORMAT);
  if (updateNotes)
    doc["notes"] = notes;
  if (updateTags)
    copyArray(tags, doc["tags"]);

  snprintf(parameters, 256,"/%d/",id);

  int httpsResponseCode = httpRequest(SLEEP_ENDPOINT, "PATCH", parameters, "", true);
  Serial.println(httpsResponseCode);

  if(httpsResponseCode <= 0)
  {
    return outcome;
  }

  outcome.id = response["id"];
  outcome.child = response["child"];
  outcome.nap = response["nap"];
  outcome.start = response["start"];
  outcome.end = response["end"];
  outcome.duration = response["duration"];
  outcome.notes = response["notes"];
  copyArray(response["tags"].as<JsonArray>(),outcome.tags);

  return outcome;
}

bool BabyApi::removeSleep(uint16_t id)
{
  snprintf(parameters, 256,"/%d/",id);

  int httpsResponseCode = httpRequest(SLEEP_ENDPOINT, "DELETE", parameters);
  Serial.println(httpsResponseCode);

  return httpsResponseCode == 204;
}

BabyApi::searchResults<BabyApi::Tag> BabyApi::findAllTags(
    uint16_t offset = 0,
    const char * name = {},
    time_t last_used = {},
    const char * ordering = {})
{
  BabyApi::searchResults<BabyApi::Tag> outcome;
  uint16_t count = 0;
  std::ostringstream stream;

  stream << "limit=" << SEARCH_LIMIT;

  if( offset > 0 ) stream << ",offset=" << offset;
  if( name[0] != '\0'  ) stream << ",name=" << name;
  if( last_used > 0 ) stream << ",last_used="<< dateTime(last_used,BB_DATE_FORMAT);
  if( ordering[0] != '\0' ) stream << ",ordering=" << ordering;

  int httpsResponseCode = httpRequest(TAGS_ENDPOINT, "GET", "", query);
  Serial.println(httpsResponseCode);

  if(httpsResponseCode <= 0)
  {
    return outcome;
  }

  searchResultParser(&outcome.count, &outcome.next, &outcome.previous);

  JsonArray results = response["results"].as<JsonArray>();

  for (JsonObject tagRecord : results)
  {
    outcome.results[count].name = tagRecord["name"];
    outcome.results[count].last_used = tagRecord["last_used"];
    outcome.results[count].color = tagRecord["color"];
    outcome.results[count].slug = tagRecord["slug"];

    count++;
  }

  return outcome;
}

BabyApi::Tag BabyApi::createTag(
    const char * name,
    const char * colour = {})
{
  BabyApi::Tag outcome;

  doc.clear();

  doc["name"] = name;
  doc["colour"] = colour;

  

  int httpsResponseCode = httpRequest(TAGS_ENDPOINT, "POST", "", "", true);
  Serial.println(httpsResponseCode);

  if(httpsResponseCode <= 0)
  {
    return outcome;
  }

  outcome.name = response["name"];
  outcome.last_used = response["last_used"];
  outcome.color = response["color"];
  outcome.slug = response["slug"];

  return outcome;
}

BabyApi::Tag BabyApi::getTag(const char * slug)
{
  BabyApi::Tag outcome;

  snprintf(parameters, 256,"/%s/",slug);

  int httpsResponseCode = httpRequest(TAGS_ENDPOINT, "GET", parameters);
  Serial.println(httpsResponseCode);

  if(httpsResponseCode <= 0)
  {
    return outcome;
  }

  outcome.name = response["name"];
  outcome.last_used = response["last_used"];
  outcome.color = response["color"];
  outcome.slug = response["slug"];

  return outcome;
}

BabyApi::Tag BabyApi::updateTag(
    const char * slug,
    bool updateName = false,
    const char * name = {},
    bool updateColour = false,
    const char * colour = {})
{
  BabyApi::Tag outcome;

  doc.clear();

  if (updateName)
    doc["name"] = name;
  if (updateColour)
    doc["colour"] = colour;

  snprintf(parameters, 256,"/%s/",slug);

  int httpsResponseCode = httpRequest(TAGS_ENDPOINT, "PATCH", parameters, "", true);
  Serial.println(httpsResponseCode);

  if(httpsResponseCode <= 0)
  {
    return outcome;
  }

  outcome.name = response["name"];
  outcome.last_used = response["last_used"];
  outcome.color = response["color"];
  outcome.slug = response["slug"];

  return outcome;
}

bool BabyApi::removeTag(const char * slug)
{
  snprintf(parameters, 256,"/%s/",slug);

  int httpsResponseCode = httpRequest(TAGS_ENDPOINT, "DELETE", parameters);
  Serial.println(httpsResponseCode);

  return httpsResponseCode == 204;
}

BabyApi::searchResults<BabyApi::Temperature> BabyApi::findTemperatureRecords(
    uint16_t offset = 0,
    uint16_t child = 0,
    time_t date = {},
    time_t date_max = {},
    time_t date_min = {},
    const char * tags[MAX_TAGS] = {},
    const char * ordering = {})
{
  BabyApi::searchResults<BabyApi::Temperature> outcome;
  uint16_t count = 0;
  std::ostringstream stream;

  stream << "limit=" << SEARCH_LIMIT;
  if( offset > 0 ) stream << ",offset=" << offset;
  if( child > 0 ) stream << ",child=" << child;
  if( date > 0  ) stream << ",date=" << dateTime(date,BB_DATE_FORMAT);
  if( date_max > 0  ) stream << ",date_max=" << dateTime(date_max,BB_DATE_FORMAT);
  if( date_min > 0  ) stream << ",date_min=" << dateTime(date_min,BB_DATE_FORMAT);
  if( tags[0] != '\0' ) stream << ",tags=" << tags;
  if( ordering[0] != '\0' ) stream << ",ordering=" << ordering;

  int httpsResponseCode = httpRequest(TEMPERATURE_ENDPOINT, "GET", "", query);
  Serial.println(httpsResponseCode);

  if(httpsResponseCode <= 0)
  {
    return outcome;
  }

  searchResultParser(&outcome.count, &outcome.next, &outcome.previous);

  JsonArray results = response["results"].as<JsonArray>();

  for (JsonObject temperatureRecord : results)
  {
    outcome.results[count].id = temperatureRecord["id"];
    outcome.results[count].child = temperatureRecord["child"];
    outcome.results[count].time = temperatureRecord["time"];
    outcome.results[count].temperature = temperatureRecord["temperature"];
    outcome.results[count].notes = temperatureRecord["notes"];
    copyArray(temperatureRecord["tags"].as<JsonArray>(),outcome.results[count].tags);

    count++;
  }

  return outcome;
}

BabyApi::Temperature BabyApi::logTemperature(
    uint16_t child,
    float temperature,
    time_t time,
    const char * notes = {},
    const char * tags[MAX_TAGS] = {})
{
  BabyApi::Temperature outcome;

  doc.clear();

  doc["child"] = child;
  doc["time"] = dateTime(time,BB_DATE_FORMAT);;
  doc["temperature"] = temperature;
  doc["notes"] = notes;
  copyArray(tags, doc["tags"]);

  int httpsResponseCode = httpRequest(TEMPERATURE_ENDPOINT, "POST", "", "", true);
  Serial.println(httpsResponseCode);

  if(httpsResponseCode <= 0)
  {
    return outcome;
  }

  outcome.id = response["id"];
  outcome.child = response["child"];
  outcome.time = response["time"];
  outcome.temperature = response["temperature"];
  outcome.notes = response["notes"];
  copyArray(response["tags"].as<JsonArray>(),outcome.tags);

  return outcome;
}

BabyApi::Temperature BabyApi::getTemperature(uint16_t id)
{
  BabyApi::Temperature outcome;

  snprintf(parameters, 256,"/%d/",id);

  int httpsResponseCode = httpRequest(TEMPERATURE_ENDPOINT, "GET", parameters);
  Serial.println(httpsResponseCode);

  if(httpsResponseCode <= 0)
  {
    return outcome;
  }

  outcome.id = response["id"];
  outcome.child = response["child"];
  outcome.time = response["time"];
  outcome.temperature = response["temperature"];
  outcome.notes = response["notes"];
  copyArray(response["tags"].as<JsonArray>(),outcome.tags);

  return outcome;
}

BabyApi::Temperature BabyApi::updateTemperature(
    uint16_t id = 0,
    uint16_t child = 0,
    float temperature = NAN,
    time_t time = {},
    bool updateNotes = false,
    const char * notes = {},
    bool updateTags = false,
    const char * tags[MAX_TAGS] = {})
{
  BabyApi::Temperature outcome;

  doc.clear();

  if (child > 0)
    doc["child"] = child;
  if (time > 0)
    doc["time"] = dateTime(time,BB_DATE_FORMAT);;
  if (!isnan(temperature))
    doc["temperature"] = temperature;
  if (updateNotes)
    doc["notes"] = notes;
  if (updateTags)
    copyArray(tags, doc["tags"]);

  

  snprintf(parameters, 256,"/%d/",id);

  int httpsResponseCode = httpRequest(TEMPERATURE_ENDPOINT, "PATCH", parameters, "", true);
  Serial.println(httpsResponseCode);

  if(httpsResponseCode <= 0)
  {
    return outcome;
  }

  outcome.id = response["id"];
  outcome.child = response["child"];
  outcome.time = response["time"];
  outcome.temperature = response["temperature"];
  outcome.notes = response["notes"];
  copyArray(response["tags"].as<JsonArray>(),outcome.tags);

  return outcome;
}

bool BabyApi::removeTemperature(uint16_t id)
{
  snprintf(parameters, 256,"/%d/",id);

  int httpsResponseCode = httpRequest(TEMPERATURE_ENDPOINT, "DELETE", parameters);
  Serial.println(httpsResponseCode);

  return httpsResponseCode == 204;
}

BabyApi::searchResults<BabyApi::Timer> BabyApi::findTimers(
    uint16_t offset = 0,
    uint16_t child = 0,
     time_t start = {},
     time_t start_max = {},
     time_t start_min = {},
     time_t end = {},
     time_t end_max = {},
     time_t end_min = {},
     TimerState active = UNKNOWN,
    uint16_t user = 0,
     const char * ordering = {})
{
  BabyApi::searchResults<BabyApi::Timer> outcome;
  uint16_t count = 0;
  std::ostringstream stream;

  stream << "limit=" << SEARCH_LIMIT;

  if( offset > 0 ) stream << ",offset=" << offset;
  if( child > 0 ) stream << ",child=" << child;
  if( start > 0 ) stream << ",start="<< dateTime(start,BB_DATE_FORMAT);
  if( start_max > 0 ) stream << ",start_max=" << dateTime(start_max,BB_DATE_FORMAT);
  if( start_min > 0 ) stream << ",start_min=" << dateTime(start_min,BB_DATE_FORMAT);
  if( end > 0 ) stream << ",end=" << dateTime(end,BB_DATE_FORMAT);
  if( end_max > 0 ) stream << ",end_max=" << dateTime(end_max,BB_DATE_FORMAT);
  if( end_min > 0 ) stream << ",end_min=" << dateTime(end_min,BB_DATE_FORMAT);
  if( active != UNKNOWN ) stream << ",active=" << (active == ACTIVE) ? "true":"false";
  if( user > 0 ) stream << ",user=" << user;
  if( ordering[0] != '\0' ) stream << ",ordering=" << ordering;

  int httpsResponseCode = httpRequest(TIMERS_ENDPOINT, "GET", "", query);
  Serial.println(httpsResponseCode);

  if(httpsResponseCode <= 0)
  {
    return outcome;
  }

  searchResultParser(&outcome.count, &outcome.next, &outcome.previous);

  JsonArray results = response["results"].as<JsonArray>();

  for (JsonObject timerRecord : results)
  {
    outcome.results[count].id = timerRecord["id"];
    outcome.results[count].child = timerRecord["child"];
    outcome.results[count].start = timerRecord["start"];
    outcome.results[count].end = timerRecord["end"];
    outcome.results[count].duration = timerRecord["duration"];
    outcome.results[count].user = timerRecord["user"];

    count++;
  }

  return outcome;
}

uint16_t BabyApi::startTimer(
      uint16_t childId = 0, 
      const char * name = {}, 
      uint16_t timer = 0)
{
  BabyApi::Timer babyTimer;

  if(timer == 0 && childId == 0)
  {
    Serial.println("Must provide an ID of a timer or the ID of a child that the timer is for.");
  }

  if (timer == 0 && name[0] != '\0')
  {
    // search for existing timer usin the child id and name
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

  if (timer > 0)
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
    const char * name)
{
  return createTimer(
      child,
      name,
      0);
}

BabyApi::Timer BabyApi::createTimer(
    uint16_t child,
    time_t start)
{
  return createTimer(
      child,
      "",
      start);
}

BabyApi::Timer BabyApi::createTimer(
    uint16_t child,
    const char * name,
    time_t start)
{
  BabyApi::Timer outcome;

  doc.clear();

  doc["child"] = child;
  doc["name"] = name;
  doc["start"] = dateTime(start, BB_DATE_FORMAT);

  int httpsResponseCode = httpRequest(TIMERS_ENDPOINT, "POST", "", "", true);
  Serial.println(httpsResponseCode);

  if(httpsResponseCode <= 0)
  {
    return outcome;
  }

  outcome.id = response["id"];
  outcome.child = response["child"];
  outcome.start = response["start"];
  outcome.end = response["end"];
  outcome.duration = response["duration"];
  outcome.user = response["user"];

  return outcome;
}

BabyApi::Timer BabyApi::getTimer(uint16_t id)
{
  BabyApi::Timer outcome;

  snprintf(parameters, 256,"/%d/",id);

  int httpsResponseCode = httpRequest(TIMERS_ENDPOINT, "GET", parameters);
  Serial.println(httpsResponseCode);

  if(httpsResponseCode <= 0)
  {
    return outcome;
  }

  outcome.id = response["id"];
  outcome.child = response["child"];
  outcome.start = response["start"];
  outcome.end = response["end"];
  outcome.duration = response["duration"];
  outcome.user = response["user"];

  return outcome;
}

BabyApi::Timer BabyApi::updateTimer(
    uint16_t id,
    uint16_t child = 0,
    const char * name = {},
    time_t start = {},
    uint16_t user = 0)
{
  BabyApi::Timer outcome;

  doc.clear();

  if (child > 0)
    doc["child"] = child;
  if (!name != '\0')
    doc["name"] = name;
  if (!start > 0)
    doc["start"] = dateTime(start, BB_DATE_FORMAT);
  if (user > 0)
    doc["user"] = user;

  

  snprintf(parameters, 256,"/%d/",id);

  int httpsResponseCode = httpRequest(TIMERS_ENDPOINT, "GET", parameters, "", true);
  Serial.println(httpsResponseCode);

  if(httpsResponseCode <= 0)
  {
    return outcome;
  }

  outcome.id = response["id"];
  outcome.child = response["child"];
  outcome.start = response["start"];
  outcome.end = response["end"];
  outcome.duration = response["duration"];
  outcome.user = response["user"];

  return outcome;
}

bool BabyApi::removeTimer(uint16_t id)
{
  snprintf(parameters, 256,"/%d/",id);

  int httpsResponseCode = httpRequest(TIMERS_ENDPOINT, "DELETE", parameters);
  Serial.println(httpsResponseCode);

  return httpsResponseCode == 204;
}

BabyApi::Timer BabyApi::restartTimer(uint16_t id)
{
  BabyApi::Timer outcome;

  snprintf(parameters, 256,"/%d/",id);

  int httpsResponseCode = httpRequest(TIMERS_ENDPOINT, "PATCH", parameters);
  Serial.println(httpsResponseCode);

  if(httpsResponseCode <= 0)
  {
    return outcome;
  }

  outcome.id = response["id"];
  outcome.child = response["child"];
  outcome.start = response["start"];
  outcome.end = response["end"];
  outcome.duration = response["duration"];
  outcome.user = response["user"];

  return outcome;
}

BabyApi::Timer BabyApi::stopTimer(uint16_t id)
{
  BabyApi::Timer outcome;

  snprintf(parameters, 256,"/%d/",id);

  int httpsResponseCode = httpRequest(TIMERS_ENDPOINT, "PATCH", parameters);
  Serial.println(httpsResponseCode);

  if(httpsResponseCode <= 0)
  {
    return outcome;
  }

  outcome.id = response["id"];
  outcome.child = response["child"];
  outcome.start = response["start"];
  outcome.end = response["end"];
  outcome.duration = response["duration"];
  outcome.user = response["user"];

  return outcome;
}

BabyApi::searchResults<BabyApi::TummyTime> BabyApi::findTummyTimes(
    uint16_t offset = 0,
    uint16_t child = 0,
    time_t start = {},
    time_t start_max = {},
    time_t start_min = {},
    time_t end = {},
    time_t end_max = {},
    time_t end_min = {},
    const char * tags[MAX_TAGS] = {},
    const char * ordering = {})
{
  BabyApi::searchResults<BabyApi::TummyTime> outcome;
  uint16_t count = 0;
  std::ostringstream stream;

  stream << "limit=" << SEARCH_LIMIT;

  if( offset > 0 ) stream << ",offset=" << offset;
  if( child > 0 ) stream << ",child=" << child;
  if( start > 0 ) stream << ",start="<< dateTime(start,BB_DATE_FORMAT);
  if( start_max > 0 ) stream << ",start_max=" << dateTime(start_max,BB_DATE_FORMAT);
  if( start_min > 0 ) stream << ",start_min=" << dateTime(start_min,BB_DATE_FORMAT);
  if( end > 0 ) stream << ",end=" << dateTime(end,BB_DATE_FORMAT);
  if( end_max > 0 ) stream << ",end_max=" << dateTime(end_max,BB_DATE_FORMAT);
  if( end_min > 0 ) stream << ",end_min=" << dateTime(end_min,BB_DATE_FORMAT);
  if( tags[0] != '\0' ) stream << ",tags=" << tags;
  if( ordering[0] != '\0' ) stream << ",ordering=" << ordering;

  int httpsResponseCode = httpRequest(TUMMY_TIMES_ENDPOINT, "GET", "", query);
  Serial.println(httpsResponseCode);

  if(httpsResponseCode <= 0)
  {
    return outcome;
  }

  searchResultParser( &outcome.count, &outcome.next, &outcome.previous);

  JsonArray results = response["results"].as<JsonArray>();

  for (JsonObject tummyTimeRecord : results)
  {
    outcome.results[count].id = tummyTimeRecord["id"];
    outcome.results[count].child = tummyTimeRecord["child"];
    outcome.results[count].start = tummyTimeRecord["start"];
    outcome.results[count].end = tummyTimeRecord["end"];
    outcome.results[count].duration = tummyTimeRecord["duration"];
    outcome.results[count].milestone = tummyTimeRecord["milestone"];
    copyArray(tummyTimeRecord["tags"].as<JsonArray>(),outcome.results[count].tags);

    count++;
  }

  return outcome;
}

BabyApi::TummyTime BabyApi::logTummyTime(
    uint16_t child = 0,          // Required unless a Timer value is provided.
    time_t start = {}, // Required unless a Timer value is provided.
    time_t end = {},   // Required unless a Timer value is provided.
    uint16_t timer = 0,          // May be used in place of the Start, End, and/or Child values.
    const char * milestone = {},
    const char * tags[MAX_TAGS] = {})
{
  BabyApi::TummyTime outcome;

  doc.clear();

  // if no tmer value present, child start and end are required fields
  if (timer == 0 && (child == 0 || start == 0 || end == 0))
  {
    Serial.println("Missing child, start and end, these are required if no timer id provided:");
    Serial.print("child: ");
    Serial.println(child);
    Serial.print("start: ");
    Serial.println(dateTime(start,BB_DATE_FORMAT));
    Serial.print("end: ");
    Serial.println(dateTime(end,BB_DATE_FORMAT));
    return outcome;
  }

  if (child > 0)
    doc["child"] = child;
  doc["start"] = dateTime(start,BB_DATE_FORMAT);
  doc["end"] = dateTime(end,BB_DATE_FORMAT);
  if (timer > 0)
    doc["timer"] = timer;
  doc["milestone"] = milestone;
  copyArray(tags, doc["tags"]);

  int httpsResponseCode = httpRequest(TUMMY_TIMES_ENDPOINT, "POST", "", "", true);
  Serial.println(httpsResponseCode);

  if(httpsResponseCode <= 0)
  {
    return outcome;
  }

  outcome.id = response["id"];
  outcome.child = response["child"];
  outcome.start = response["start"];
  outcome.end = response["end"];
  outcome.duration = response["duration"];
  outcome.milestone = response["milestone"];
  copyArray(response["tags"].as<JsonArray>(),outcome.tags);

  return outcome;
}

BabyApi::TummyTime BabyApi::getTummyTime(uint16_t id)
{
  BabyApi::TummyTime outcome;

  snprintf(parameters, 256,"/%d/",id);

  int httpsResponseCode = httpRequest(TUMMY_TIMES_ENDPOINT, "GET", parameters);
  Serial.println(httpsResponseCode);

  if(httpsResponseCode <= 0)
  {
    return outcome;
  }

  outcome.id = response["id"];
  outcome.child = response["child"];
  outcome.start = response["start"];
  outcome.end = response["end"];
  outcome.duration = response["duration"];
  outcome.milestone = response["milestone"];
  copyArray(response["tags"].as<JsonArray>(),outcome.tags);

  return outcome;
}

BabyApi::TummyTime BabyApi::updateTummyTime(
    uint16_t id,
    uint16_t child = 0,
    time_t start = {},
    time_t end = {},
    bool updateMilestone = false,
    const char * milestone = {},
    bool updateTags = false,
    const char * tags[MAX_TAGS] = {})
{
  BabyApi::TummyTime outcome;

  doc.clear();

  if (child > 0)
    doc["child"] = child;
  if (start != '\0')
    doc["start"] = dateTime(start,BB_DATE_FORMAT);
  if (start != '\0')
    doc["end"] = dateTime(end,BB_DATE_FORMAT);
  if (updateMilestone)
    doc["milestone"] = milestone;
  if (updateTags)
    copyArray(tags, doc["tags"]);

  

  snprintf(parameters, 256,"/%d/",id);

  int httpsResponseCode = httpRequest(TUMMY_TIMES_ENDPOINT, "PATCH", parameters, "", true);
  Serial.println(httpsResponseCode);

  if(httpsResponseCode <= 0)
  {
    return outcome;
  }

  outcome.id = response["id"];
  outcome.child = response["child"];
  outcome.start = response["start"];
  outcome.end = response["end"];
  outcome.duration = response["duration"];
  outcome.milestone = response["milestone"];
  copyArray(response["tags"].as<JsonArray>(),outcome.tags);

  return outcome;
}

bool BabyApi::removeTummyTime(uint16_t id)
{
  snprintf(parameters, 256,"/%d/",id);

  int httpsResponseCode = httpRequest(TUMMY_TIMES_ENDPOINT, "DELETE", parameters);
  Serial.println(httpsResponseCode);

  return httpsResponseCode == 204;
}

BabyApi::searchResults<BabyApi::Weight> BabyApi::findWeightRecords(
    uint16_t offset = 0,
    uint16_t child = 0,
    time_t date = {},
    const char * ordering = {})
{
  BabyApi::searchResults<BabyApi::Weight> outcome;
  uint16_t count = 0;
  std::ostringstream stream;

  stream << "limit=" << SEARCH_LIMIT;
  if( offset > 0 ) stream << ",offset=" << offset;
  if( child > 0 ) stream << ",child=" << child;
  if( date > 0  ) stream << ",date=" << dateTime(date,BB_DATE_FORMAT);
  if( ordering[0] != '\0' ) stream << ",ordering=" << ordering;

  int httpsResponseCode = httpRequest(WEIGHT_ENDPOINT, "GET", "", query);
  Serial.println(httpsResponseCode);

  if(httpsResponseCode <= 0)
  {
    return outcome;
  }

  searchResultParser(&outcome.count, &outcome.next, &outcome.previous);

  JsonArray results = response["results"].as<JsonArray>();

  for (JsonObject weightRecord : results)
  {
    outcome.results[count].id = weightRecord["id"];
    outcome.results[count].child = weightRecord["child"];
    outcome.results[count].weight = weightRecord["weight"];
    outcome.results[count].date = weightRecord["date"];
    outcome.results[count].notes = weightRecord["notes"];
    copyArray(weightRecord["tags"].as<JsonArray>(),outcome.results[count].tags);

    count++;
  }

  return outcome;
}

BabyApi::Weight BabyApi::logWeight(
    uint16_t child,
    float weight,
    time_t date,
    const char * notes = {},
    const char * tags[MAX_TAGS] = {})
{
  BabyApi::Weight outcome;

  doc.clear();

  doc["child"] = child;
  doc["date"] = dateTime(date,BB_DATE_FORMAT);
  doc["weight"] = weight;
  doc["notes"] = notes;
  copyArray(tags, doc["tags"]);

  int httpsResponseCode = httpRequest(WEIGHT_ENDPOINT, "POST", "", "", true);
  Serial.println(httpsResponseCode);

  if(httpsResponseCode <= 0)
  {
    return outcome;
  }

  outcome.id = response["id"];
  outcome.child = response["child"];
  outcome.date = response["date"];
  outcome.weight = response["weight"];
  outcome.notes = response["notes"];
  copyArray(response["tags"].as<JsonArray>(),outcome.tags);

  return outcome;
}

BabyApi::Weight BabyApi::getWeight(uint16_t id)
{
  BabyApi::Weight outcome;

  snprintf(parameters, 256,"/%d/",id);

  int httpsResponseCode = httpRequest(WEIGHT_ENDPOINT, "GET", parameters);
  Serial.println(httpsResponseCode);

  if(httpsResponseCode <= 0)
  {
    return outcome;
  }

  outcome.id = response["id"];
  outcome.child = response["child"];
  outcome.date = response["date"];
  outcome.weight = response["weight"];
  outcome.notes = response["notes"];
  copyArray(response["tags"].as<JsonArray>(),outcome.tags);

  return outcome;
}

BabyApi::Weight BabyApi::updateWeight(
    uint16_t id,
    uint16_t child = 0,
    float weight = NAN,
    time_t date = {},
    bool updateNotes = false,
    const char * notes = {},
    bool updateTags = false,
    const char * tags[MAX_TAGS] = {})
{
  BabyApi::Weight outcome;

  doc.clear();

  if (child > 0)
    doc["child"] = child;
  if (date != '\0')
    doc["date"] = dateTime(date,BB_DATE_FORMAT);
  if (!isnan(weight))
    doc["weight"] = weight;
  if (updateNotes)
    doc["notes"] = notes;
  if (updateTags)
    copyArray(tags, doc["tags"]);

  

  snprintf(parameters, 256,"/%d/",id);

  int httpsResponseCode = httpRequest(WEIGHT_ENDPOINT, "PATCH", parameters, "", true);
  Serial.println(httpsResponseCode);

  if(httpsResponseCode <= 0)
  {
    return outcome;
  }

  outcome.id = response["id"];
  outcome.child = response["child"];
  outcome.date = response["date"];
  outcome.weight = response["weight"];
  outcome.notes = response["notes"];
  copyArray(response["tags"].as<JsonArray>(),outcome.tags);

  return outcome;
}

bool BabyApi::removeWeight(uint16_t id)
{
  snprintf(parameters, 256,"/%d/",id);

  int httpsResponseCode = httpRequest(WEIGHT_ENDPOINT, "DELETE", parameters);
  Serial.println(httpsResponseCode);

  return httpsResponseCode == 204;
}

BabyApi::Profile BabyApi::getProfile()
{
  BabyApi::Profile outcome;

  int httpsResponseCode = httpRequest(PROFILE_ENDPOINT, "GET");
  Serial.println(httpsResponseCode);

  outcome.user.id = response["user"]["id"];
  outcome.user.username = response["user"]["username"];
  outcome.user.first_name = response["user"]["first_name"];
  outcome.user.last_name = response["user"]["last_name"];
  outcome.user.email = response["user"]["email"];
  outcome.user.is_staff = response["user"]["is_staff"];
  outcome.language = response["language"];
  outcome.timezone = response["timezone"];
  outcome.api_key = response["api_key"];

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

  slept = babyApi.logSleep(0,0,0,timerId);

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
      0,
      0,
      timerId);

  return tummy.id;
}

uint8_t BabyApi::recordNappyChange(uint16_t child, bool wet, bool solid, uint16_t colour)
{
  BabyApi::DiaperChange changed;
  char buffer[15];

  strcpy(buffer,stoolColours[colour]);

  changed = babyApi.logDiaperChange(child,wet,solid,buffer);

  return changed.id;
}
