#pragma once
#include <iostream>
#include <fstream>
#include <curl/curl.h>
#include <cpr/cpr.h>
#include <algorithm>
#include <string>
#include "rapidjson/document.h"
#include "News.h"

//For writing curl output into string
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp)
{
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}
using namespace std;
using namespace rapidjson;

/*
createCSV(string fileName, vector<Article> articles): For outputting of the news articled crawled into the CSV file
*/
void Article::createCSV(string fileName, vector<Article> articles) {
    string title, source, content, date, url, category;
    std::ofstream newsCSV;
    newsCSV.open(fileName + ".csv");
    //Added to support encoding of UTF-8 characters to output to csv file with Byte-Order Mark (BOM)
    //Source: https://stackoverflow.com/questions/21272006/how-to-save-a-csv-file-as-utf-8
    const char* bom = "\xef\xbb\xbf";
    newsCSV << bom;
    newsCSV << "Title, Source, Date, Content, URL, Category" << endl;
    for (int i = 0; i < articles.size(); i++) {
        title = articles[i].getTitle();
        for (int j = 0; j < title.length(); ++j) {
            //Prevent separation of columns by replacing commas with semicommas
            if (title[j] == ',') {
                title[j] = ';';
            }
        }
        source = articles[i].getSource();
        date = articles[i].getDate();
        content = articles[i].getContent();
        for (int k = 0; k < content.length(); ++k) {
            //Prevent separation of columns by replacing commas with semicommas
            if (content[k] == ',') {
                content[k] = ';';
            }
        }
        url = articles[i].getURL();
        category = articles[i].getCategory();
        newsCSV << title + "," << source + "," << date + "," << content + "," << url + "," << category << "\n";
    }
    newsCSV.close();
}
/*
public queryString(string): parses normal string into URL querry format.
Source: https://en.wikipedia.org/wiki/Query_string
private ReplaceStuff(): used by queryString() to escape stuff.
Source:https://stackoverflow.com/questions/2896600/how-to-replace-all-occurrences-of-a-character-in-string
*/
string ReplaceStuff(string str, const string& from, const string& to) {
    size_t start_pos = 0;
    while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length(); // Handles case where 'to' is a substring of 'from'
    }
    return str;
}
string queryString(string s) {
    //replace(s.begin(), s.end(), ' ', ""); // replace all 'x' to 'y'
    s = ReplaceStuff(s, " ", "%20");
    s = ReplaceStuff(s, "'", "%27");
    s = ReplaceStuff(s, "\"", "%22");
    return s;
}
/*
vector<string> textClassification(string,vector<Article>): returns list of all MeaningCloud text classification.
Source: https://www.meaningcloud.com/developer/text-classification/doc/1.1/request
Source 2: https://stackoverflow.com/questions/9786150/save-curl-content-result-into-a-string-in-c
*/
vector<string> Article::analyzeClassification(string fileName, vector<Article> article) {
    CURL* curl;
    CURLcode res;
    const string MEANINGCLOUDCLASSURL = "https://api.meaningcloud.com/class-1.1";
    const string APIKEY = "?key=d1d248b93272995b72eb5b36b6035f66";
    Document doc;
    string classification, result, relevance, text;
    News news;
    vector<string> allClassificationsJSON;
    vector<string> allClassifications;
    vector<string> allRelevance;
    for (int i = 0; i < article.size(); i++) {
        curl = curl_easy_init();
        text = article[i].getContent();
        text = queryString(text);
        string apiURL = "";
        apiURL.append(MEANINGCLOUDCLASSURL);
        apiURL.append(APIKEY);
        apiURL.append("&of=json");//specify output to be json
        apiURL.append("&txt=" + text);
        apiURL.append("&model=IPTC_en");//set language as english
        if (curl) {
            curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");
            curl_easy_setopt(curl, CURLOPT_URL, apiURL.c_str());
            curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
            curl_easy_setopt(curl, CURLOPT_DEFAULT_PROTOCOL, "https");
            struct curl_slist* headers = NULL;
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
            //for saving results
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &result);
            res = curl_easy_perform(curl);
        }
        else {
            cout << "Text classification failed for " << text << endl;
        }
        curl_easy_cleanup(curl);
        allClassificationsJSON.push_back(result);
        result = "";
    }
    cout << "Text Classification Results:" << endl;
    //Do JSON crawling on retrieval of classifications
    for (int j = 0; j < allClassificationsJSON.size(); j++) {
        cout << allClassificationsJSON[j] << endl;
        doc.Parse(allClassificationsJSON[j].c_str());
        string status = doc["status"].GetObjectW()["code"].GetString();
        //If the status retrieved is ok, check if the meaningcloud managed to classify the news articles based on its content
        if (status == "0") {
            if (doc["category_list"].GetArray().Size() > 0) {
                if (doc["category_list"][0].HasMember("label")) {
                    classification = doc["category_list"][0]["label"].GetString();
                    allClassifications.push_back(classification);
                }
                if (doc["category_list"][0].HasMember("relevance")) {
                    relevance = doc["category_list"][0]["relevance"].GetString();
                    allRelevance.push_back(relevance);
                }
            }
            else {
                allClassifications.push_back("");
                allRelevance.push_back("0");
            }
        }
        else {
            allClassifications.push_back("");
            allRelevance.push_back("0");
        }
    }
    news.createAnalyzedCSV(fileName, article, allClassifications, allRelevance);
    return allClassifications;
}

/*
createAnalyzedCSV(string fileName, vector<Article> articles, vector<string> classificationList, vector<string> relevanceList): This is to output the updated results together with the classification results from MeaningCloud API
*/
void Article::createAnalyzedCSV(string fileName, vector<Article> articles, vector<string> classificationList, vector<string> relevanceList) {
    string title, source, content, date, url, category, classification, relevance;
    std::ofstream newsCSV;
    newsCSV.open(fileName + "_Analyzed.csv");
    //Added to support encoding of UTF-8 characters to output to csv file with Byte-Order Mark (BOM)
    //Source: https://stackoverflow.com/questions/21272006/how-to-save-a-csv-file-as-utf-8
    const char* bom = "\xef\xbb\xbf";
    newsCSV << bom;
    newsCSV << "Title, Source, Date, Content, URL, Category, Classification, Relevance" << endl;
    for (int i = 0; i < articles.size(); i++) {
        title = articles[i].getTitle();
        for (int j = 0; j < title.length(); ++j) {
            //Prevent separation of columns by replacing commas with semicommas
            if (title[j] == ',') {
                title[j] = ';';
            }
        }
        source = articles[i].getSource();
        date = articles[i].getDate();
        content = articles[i].getContent();
        for (int k = 0; k < content.length(); ++k) {
            //Prevent separation of columns by replacing commas with semicommas
            if (content[k] == ',') {
                content[k] = ';';
            }
        }
        url = articles[i].getURL();
        category = articles[i].getCategory();
        classification = classificationList[i];
        for (int l = 0; l < classification.length(); ++l) {
            //Prevent separation of columns by replacing commas with semicommas
            if (classification[l] == ',') {
                classification[l] = ';';
            }
        }
        relevance = relevanceList[i];
        newsCSV << title + "," << source + "," << date + "," << content + "," << url + "," << category + "," << classification + "," << relevance << "\n";
    }
    newsCSV.close();
}

/*
vector<int> Article::sentimentAnalysis(vector<Article> article): This will run the sentiment anaylysis function using MeaningCloud API and will return a set of sentimental values such as each score tag, confidence and subjectivity levels
*/
vector<int> Article::sentimentAnalysis(vector<Article> article) {
    CURL* curl;
    CURLcode res;
    Document doc;
    int confidence;
    string score_tag, subjectivity, result, text;
    curl = curl_easy_init();
    const string MEANINGCLOUDSENTIMENTURL = "https://api.meaningcloud.com/sentiment-2.1";
    const string APIKEY = "?key=09a2709de3d1e0d7a59e3ef3b8fe0fce";
    vector<string> allSentimentJSON;
    vector<int> sentimentArray;
    /*Initialises the array index content, being
      ARRAY INDEX CONTENT (SUM)
      0 - P+
      1 - P
      2 - NEU
      3 - N
      4 - N+
      5 - NONE
      6 - CONFIDENCE
      7 - SUBJECTIVE
    */
    for (int i = 0; i < 8; i++) {
        sentimentArray.push_back(0);
    }
    for (int i = 0; i < article.size(); i++) {
        curl = curl_easy_init();
        text = article[i].getContent();
        text = queryString(text);
        string apiURL = "";
        apiURL.append(MEANINGCLOUDSENTIMENTURL);
        apiURL.append(APIKEY);
        apiURL.append("&txt=" + text);
        apiURL.append("&of=json");
        apiURL.append("&lang=en");
        apiURL.append("&model=general");
        News news;
        if (curl) {
            curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");
            curl_easy_setopt(curl, CURLOPT_URL, apiURL.c_str());
            curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
            curl_easy_setopt(curl, CURLOPT_DEFAULT_PROTOCOL, "https");
            struct curl_slist* headers = NULL;
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
            //for saving results
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &result);
            res = curl_easy_perform(curl);
        }
        else {
            cout << "Sentimental Analysis failed for " << text << endl;
        }
        curl_easy_cleanup(curl);
        cout << "Sentimental Result: " << result << endl;
        allSentimentJSON.push_back(result);
        result = "";
    }
    //Do JSON crawling on retrieval of sentimental result
    for (int j = 0; j < allSentimentJSON.size(); j++) {
        doc.Parse(allSentimentJSON[j].c_str());
        string status = doc["status"].GetObjectW()["code"].GetString();
        if (status == "0") {
            /*Check tone of article
            P+: strong positive
            P: positive
            NEU: neutral
            N: negative
            N+: strong negative
            NONE: without sentiment
            */
            if (doc.HasMember("score_tag")) {
                cout << "Score_Tag Test: " << doc["score_tag"].GetString() << endl;
                score_tag = doc["score_tag"].GetString();
                if (score_tag == "P+") {
                    sentimentArray[0] += 1;
                }
                else if (score_tag == "P") {
                    sentimentArray[1] += 1;
                }
                else if (score_tag == "NEU") {
                    sentimentArray[2] += 1;
                }
                else if (score_tag == "N") {
                    sentimentArray[3] += 1;
                }
                else if (score_tag == "N+") {
                    sentimentArray[4] += 1;
                }
                else if (score_tag == "NONE") {
                    sentimentArray[5] += 1;
                }
            }
            //check if article is ironic
            if (doc.HasMember("confidence")) {
                cout << "Confidence Test: " << doc["confidence"].GetString() << endl;
                confidence = stoi(doc["confidence"].GetString());
                if (confidence > 0) {
                    sentimentArray[6] += confidence;
                }
            }
            //check what the article is all about
            if (doc.HasMember("subjectivity")) {
                cout << "Subjectivity Test: " << doc["subjectivity"].GetString() << endl;
                subjectivity = doc["subjectivity"].GetString();
                if (subjectivity == "SUBJECTIVE") {
                    sentimentArray[7] += 1;
                }
            }
        }
    }
    return sentimentArray;
}
/*
vector<Article> Article::crawl(int flag): Crawls the news sources selected using News API
*/
vector<Article> Article::crawl(int flag) {
    //Initializes the constants
    const int PAGESIZE = 100;
    const string SOURCES[] = { "theonlinecitizen.com", "channelnewsasia.com", "straitstimes.com", "thestar.com.my" };
    const string APIKEYS[] = { "ad80aa4ebd9c4f849cd8ee1636eb84da", "d21901395a414e5b98483b9d6630e18b" };
    const string WEBSITES[] = { "TOC", "CNA", "TST", "thestar" };
    //const string APIKEY = "ad80aa4ebd9c4f849cd8ee1636eb84da";
    //Filtered dictionary to determine whether the news articles belongs to Singapore, Business, or else World (For TOC & The Star)
    string singaporeKeyWords[] = { "Singapore", "SG", "Singaporean", "multi-ministry taskforce", "Multi-Ministry Taskforce", "Baey Yam Keng", "Khaw Boon Wan", "Lee Hsien Loong", "Ong Ye Kung", "Tan Cheng Bock", "Teo Chee Hean", "People's Voice", "Reform Party", "Workers' Party", "Temasek Holdings", "HDB", "LTA", "MOE", "MOH", "NEA", "NMP", "NSP", "PAP", "SPH", "WP", "SBS Transit", "SMRT", "FairPrice", "Lianhe Zaobao", "The Straits Times", "Geylang", "Holland-Bukit Timah", "Jurong", "Kranji", "MacPherson", "Potong Pasir" };
    string businessKeyWords[] = { "Business", "businesses", "Budget", "cents", "Economic Development Board", "economy", "economies", "economic", "employment", "finance", "Finance", "income", "purchasing managers' index", "recession", "salary", "tax", "wages", "DBS", "EDB", "GDP", "GST", "MAS", "SGX", "STI", "$" };
    Document doc;
    bool canCategorise = false;
    vector<Article> newsArticles;
    //Creates new child article object called News
    News news;
    //Initialize the query URL to use for News API to return all news articles belonging to The Online Citizen
    string getNewsArticlesURL = "http://newsapi.org/v2/everything?domains=";
    getNewsArticlesURL.append(SOURCES[flag]);
    getNewsArticlesURL.append("&pageSize=" + to_string(PAGESIZE));
    getNewsArticlesURL.append("&sortBy=publishedAt");
    getNewsArticlesURL.append("&apiKey=" + APIKEYS[1]);
    //Handles the HTTP REST Request
    //Source: https://www.codeproject.com/Articles/1244632/Making-HTTP-REST-Request-in-Cplusplus
    auto r = cpr::Get(cpr::Url{ getNewsArticlesURL });
    //Configures the console to allow displaying of all characters encoded to UTF-8
    //Source: https://stackoverflow.com/questions/2492077/output-unicode-strings-in-windows-console-app
    SetConsoleOutputCP(CP_UTF8);
    //Parses the document
    doc.Parse(r.text.c_str());
    cout << "News Articles retrieved from " << SOURCES[flag] << endl;
    //Deencapsulates the JSON layer to retrieve the values from specified attributes (keys)
    //Source: https://rapidjson.org/md_doc_tutorial.html
    for (SizeType i = 0; i < PAGESIZE; i++)
    {
        string source;
        string title;
        string url;
        string date;
        int day;
        int month;
        int year;
        int checkIndex = 0;
        string description;
        string category;
        int singaporeKeyWordLen = sizeof(singaporeKeyWords) / sizeof(singaporeKeyWords[0]);
        int businessKeyWordLen = sizeof(businessKeyWords) / sizeof(businessKeyWords[0]);
        canCategorise = false;
        if (doc["articles"][i].HasMember("source"))
        {
            if (doc["articles"][i]["source"].HasMember("name"))
            {
                if (doc["articles"][i]["source"]["name"].IsString())
                {
                    source = doc["articles"][i]["source"]["name"].GetString();
                    news.setSource(source);
                }
            }
        }
        if (doc["articles"][i].HasMember("title"))
        {
            if (doc["articles"][i]["title"].IsString())
            {
                title = doc["articles"][i]["title"].GetString();
                // if TOC or thestar
                if (flag == 0 || flag == 3)
                {
                    //This is to determine whether the news articles belong to Singapore, Business or world via the defined keywords in the array from the article's title attribute from JSON
                    while (checkIndex < businessKeyWordLen)
                    {
                        //If is not categorised yet
                        if (!canCategorise)
                        {
                            //Perform a comparison to check if the content matches the business keywords from the array
                            if (strstr(title.c_str(), businessKeyWords[checkIndex].c_str()))
                            {
                                //If the comparison matches, the news is determined to be a business news
                                category = "Business";
                                canCategorise = true;
                            }
                            else
                            {
                                checkIndex++;
                            }
                        }
                        //Exit the while loop if the Business category is already determined
                        else
                        {
                            break;
                        }
                    }
                    //Reset index count to determine Singapore News next, if cannot determine Business news
                    checkIndex = 0;
                    while (checkIndex < singaporeKeyWordLen)
                    {
                        if (!canCategorise)
                        {
                            //Perform a comparison to check if the content matches the Singapore keywords from the array
                            if (strstr(title.c_str(), singaporeKeyWords[checkIndex].c_str()))
                            {
                                //If the comparison matches, the news is determined to be a Singapore news
                                category = "Singapore";
                                canCategorise = true;
                            }
                            else
                            {
                                checkIndex++;
                            }
                        }
                        //Exit the while loop if the Singapore category is already determined
                        else
                        {
                            break;
                        }
                    }
                    news.setCategory(category);
                }
                news.setTitle(title);
            }
            else {
                continue;
            }
        }
        if (doc["articles"][i].HasMember("url"))
        {
            url = doc["articles"][i]["url"].GetString();
            news.setURL(url);
            // if not TOC and not thestar
            if (flag != 0 && flag != 3) {
                int startIndex;
                // if CNA
                if (flag == 1) {
                    startIndex = 36;
                }
                else if (flag == 2) {
                    startIndex = 28;
                }
                else {
                    break;
                }
                //Filter the websites to different categories based on different URLs
                string sg = url.substr(startIndex, 11);
                string bu = url.substr(startIndex, 10);
                string wr = url.substr(startIndex, 7);
                string as = url.substr(startIndex, 6);
                if (sg == "/singapore/") {
                    category = "Singapore";
                }
                else if (bu == "/business/") {
                    category = "Business";
                }
                else if (wr == "/world/" || as == "/asia/") {
                    category = "World";
                }
                else {
                    category = "NULL";
                    continue;
                }
                news.setCategory(category);
            }
        }
        if (doc["articles"][i].HasMember("publishedAt"))
        {
            date = doc["articles"][i]["publishedAt"].GetString();
            if (sscanf(date.c_str(), "%d-%d-%d", &year, &month, &day))
            {
                date = to_string(day) + "/" + to_string(month) + "/" + to_string(year);
            }
            else
            {
                cout << "Error parsing date!" << endl;
            }
            news.setDate(date);
        }
        checkIndex = 0;
        if (doc["articles"][i].HasMember("description"))
        {
            if (doc["articles"][i]["description"].IsString())
            {
                description = doc["articles"][i]["description"].GetString();
                // if TOC or thestar
                if (flag == 0 || flag == 3) {
                    //This is to determine whether the news articles belong to Singapore, Business or world via the defined keywords in the array from the article's description attribute from JSON
                    while (checkIndex < businessKeyWordLen)
                    {
                        //If is not categorised yet
                        if (!canCategorise)
                        {
                            //Perform a comparison to check if the content matches the business keywords from the array
                            if (strstr(description.c_str(), businessKeyWords[checkIndex].c_str()))
                            {
                                //If the comparison matches, the news is determined to be a business news
                                category = "Business";
                                canCategorise = true;
                            }
                            else
                            {
                                checkIndex++;
                            }
                        }
                        //Exit the while loop if the Business category is already determined
                        else
                        {
                            break;
                        }
                    }
                    //Reset index count to determine Singapore News next, if cannot determine Business news
                    checkIndex = 0;
                    while (checkIndex < singaporeKeyWordLen)
                    {
                        if (!canCategorise)
                        {
                            //Perform a comparison to check if the content matches the Singapore keywords from the array
                            if (strstr(description.c_str(), singaporeKeyWords[checkIndex].c_str()))
                            {
                                //If the comparison matches, the news is determined to be a Singapore news
                                category = "Singapore";
                                canCategorise = true;
                            }
                            else
                            {
                                checkIndex++;
                            }
                        }
                        //Exit the while loop if the Singapore category is already determined
                        else
                        {
                            break;
                        }
                    }
                    //If any of the content does not match Business/Singapore keywords, the news will be categorised into World news
                    if (!canCategorise)
                    {
                        category = "World";
                    }
                    news.setCategory(category);
                }
                news.setContent(description);
            }
            else {
                continue;
            }
        }
        //Add the objects to the vector array
        // If TST or CNA
        if (flag != 0 && flag != 3) {
            if (category != "NULL") {
                newsArticles.push_back(news);
            }
        }
        // if TOC or thestar
        else {
            newsArticles.push_back(news);
        }
    }
    return newsArticles;
}