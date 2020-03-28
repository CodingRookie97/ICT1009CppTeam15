#pragma once
#include <iostream>
#include <fstream>
#include <curl/curl.h>
#include <algorithm>
#include <string>
#include <cpr/cpr.h>
#include <Windows.h>
#include "rapidjson/document.h"
#include "CNA.h"
using namespace std;
using namespace rapidjson;

// For writing output into string
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp)
{
    ((string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}
string ReplaceStuff(string str, const string& from, const string& to) {
    size_t start_pos = 0;
    while ((start_pos = str.find(from, start_pos)) != string::npos) {
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
// create normal csv
void Article::createCSV(string fileName, vector<Article> articles) {
    string title, source, content, date, url, category;
    ofstream cnaCSV;
    cnaCSV.open(fileName + ".csv");
    // added to support encoding of UTF-8 characters to output to csv file
    const char* bom = "\xef\xbb\xbf";
    cnaCSV << bom;
    cnaCSV << "Title, Source, Date, Content, URL, Category" << endl;
    for (int i = 0; i < articles.size(); i++) {
        title = articles[i].getTitle();
        for (int j = 0; j < title.length(); ++j) {
            // prevent separation of columns by replacing commas with semicolons
            if (title[j] == ',') {
                title[j] = ';';
            }
        }
        title.erase(remove(title.begin(), title.end(), ','), title.end());
        source = articles[i].getSource();
        date = articles[i].getDate();
        content = articles[i].getContent();
        for (int k = 0; k < content.length(); ++k) {
            // prevent separation of columns by replacing commas with semicolons
            if (content[k] == ',') {
                content[k] = ';';
            }
        }
        url = articles[i].getURL();
        category = articles[i].getCategory();
        cnaCSV << title + "," << source + "," << date + "," << content + "," << url + "," << category << "\n";
    }
    cnaCSV.close();
}
// create analyzed csv
void Article::createAnalyzedCSV(string fileName, vector<Article> articles, vector<string> classificationList, vector<string> relevanceList) {
    string title, source, content, date, url, category, classification, relevance;
    ofstream newsCSV;
    newsCSV.open(fileName + "_Analyzed.csv");
    // added to support encoding of UTF-8 characters to output to csv file with Byte-Order Mark (BOM)
    // source: https://stackoverflow.com/questions/21272006/how-to-save-a-csv-file-as-utf-8
    const char* bom = "\xef\xbb\xbf";
    newsCSV << bom;
    newsCSV << "Title, Source, Date, Content, URL, Category, Classification, Relevance" << endl;
    for (int i = 0; i < articles.size(); i++) {
        title = articles[i].getTitle();
        for (int j = 0; j < title.length(); ++j) {
            // prevent separation of columns by replacing commas with semicolons
            if (title[j] == ',') {
                title[j] = ';';
            }
        }
        source = articles[i].getSource();
        date = articles[i].getDate();
        content = articles[i].getContent();
        for (int k = 0; k < content.length(); ++k) {
            // prevent separation of columns by replacing commas with semicolons
            if (content[k] == ',') {
                content[k] = ';';
            }
        }
        url = articles[i].getURL();
        category = articles[i].getCategory();
        classification = classificationList[i];
        for (int l = 0; l < classification.length(); ++l) {
            // prevent separation of columns by replacing commas with semicommas
            if (classification[l] == ',') {
                classification[l] = ';';
            }
        }
        relevance = relevanceList[i];
        newsCSV << title + "," << source + "," << date + "," << content + "," << url + "," << category + "," << classification + "," << relevance << "\n";
    }
    newsCSV.close();
}

vector<string> Article::analyzeClassification(vector<Article> article) {
    const string MEANINGCLOUDCLASSURL = "https://api.meaningcloud.com/class-1.1";
    const string APIKEY = "?key=d1d248b93272995b72eb5b36b6035f66";
    CURL* curl;
    CURLcode res;
    Document doc;
    CNA cna;
    string classification, result, relevance, text;
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
            std::cout << "Text classification failed for " << text << endl;
        }
        curl_easy_cleanup(curl);
        std::cout << "\nResult: " << result << endl;
        allClassificationsJSON.push_back(result);
        result = "";
    }
    //Do JSON crawling on retrieval of classifications
    for (int j = 0; j < allClassificationsJSON.size(); j++) {
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
    cna.createAnalyzedCSV("CNA", article, allClassifications, allRelevance);
    return allClassifications;
}

vector<int> Article::sentimentAnalysis(vector<Article> article) {
    const string MEANINGCLOUDSENTIMENTURL = "https://api.meaningcloud.com/sentiment-2.1";
    const string APIKEY = "?key=09a2709de3d1e0d7a59e3ef3b8fe0fce";
    CURL* curl;
    CURLcode res;
    Document doc;
    string result, text, score_tag, subjectivity, confidence;
    vector<string> allSentimentJSON;
    vector<int> sentimentArray;
    sentimentArray.push_back(0);
    sentimentArray.push_back(0);
    sentimentArray.push_back(0);
    sentimentArray.push_back(0);
    sentimentArray.push_back(0);
    sentimentArray.push_back(0);
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
            std::cout << "Sentimental Analysis failed for " << text << endl;
        }
        curl_easy_cleanup(curl);
        std::cout << endl;
        std::cout << "Result: " << result << endl;
        allSentimentJSON.push_back(result);
        result = "";
    }
    std::cout << "\n\nSentimental results: ";
    //Do JSON crawling on retrieval of sentimental result
    for (int j = 0; j < allSentimentJSON.size(); j++) {
        doc.Parse(allSentimentJSON[j].c_str());
        string status = doc["status"].GetObjectW()["code"].GetString();
        if (status == "0") {
            if (doc.HasMember("score_tag")) {
                std::cout << "\nScore_Tag Test: " << doc["score_tag"].GetString() << endl;
            }
            if (doc.HasMember("confidence")) {
                std::cout << "Confidence Test: " << doc["confidence"].GetString() << endl;
            }
            if (doc.HasMember("subjectivity")) {
                std::cout << "Subjectivity Test: " << doc["subjectivity"].GetString() << endl;
            }
        }
    }
    return sentimentArray;
}