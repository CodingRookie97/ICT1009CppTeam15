/*#pragma once
#include <iostream>
#include <fstream>
#include <curl/curl.h>
#include <algorithm>
#include <string>
#include "rapidjson/document.h"
#include "CNA.h"

//For writting output into string
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp)
{
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}
using namespace std;
using namespace rapidjson;

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
vector<string> Article::analyzeClassification(string fileName, vector<Article> article) {
    CURL* curl;
    CURLcode res;
    const string MEANINGCLOUDCLASSURL = "https://api.meaningcloud.com/class-1.1";
    const string APIKEY = "?key=d1d248b93272995b72eb5b36b6035f66";
    Document doc;
    string classification, result, relevance, text;
    CNA cna;
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
        apiURL.append("&of=json");
        apiURL.append("&txt=" + text);
        apiURL.append("&model=IPTC_en");
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
        //If the status retrieved is ok, check if the meaningcloud managed to classify the news articles based on its title
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
    cna.createAnalyzedCSV(fileName, article, allClassifications, allRelevance);
    return allClassifications;
}
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
    vector<int> sentimentArray;*/
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
    /*for (int i = 0; i < 8; i++) {
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
        CNA cna;
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
        if (status == "0") {*/
        /*Check tone of article
        P+: strong positive
        P: positive
        NEU: neutral
        N: negative
        N+: strong negative
        NONE: without sentiment
        */
        /*if (doc.HasMember("score_tag")) {
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
}*/