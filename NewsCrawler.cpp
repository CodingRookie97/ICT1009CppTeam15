#pragma once
#include <iostream>
#include <algorithm>
#include <cpr/cpr.h>
#include <windows.h>
#include "rapidjson/document.h"
#include "Article.h"
#include "TOC.h"
#include "CNA.h"

using namespace std;
using namespace rapidjson;

//Source: https://www.educative.io/edpresso/how-to-sort-a-map-by-value-in-cpp
//This is the utility comparator function to compare the occurences (value) in descending order and pass to the sort() module
bool sortByValDesc(const pair<string, int>& a, const pair<string, int>& b) {
	return (a.second > b.second);
}
int main(void) {
	//Initializes the constants
	const int PAGESIZE = 100;
	const string CNAURL = "channelnewsasia.com";
	const string TOCURL = "theonlinecitizen.com";
	const string APIKEY = "ad80aa4ebd9c4f849cd8ee1636eb84da";
	const string THEONLINECITIZEN = "TOC";
	//Filtered dictionary to determine whether the news articles belongs to Singapore, Business, or else World (For TOC)
	string singaporeKeyWords[] = { "Singapore", "SG", "Singaporean", "multi-ministry taskforce", "Multi-Ministry Taskforce", "Baey Yam Keng", "Khaw Boon Wan", "Lee Hsien Loong", "Ong Ye Kung", "Tan Cheng Bock", "Teo Chee Hean", "People's Voice", "Reform Party", "Workers' Party", "Temasek Holdings", "HDB", "LTA", "MOE", "MOH", "NEA", "NMP", "NSP", "PAP", "SPH", "WP", "SBS Transit", "SMRT", "FairPrice", "Geylang", "Holland-Bukit Timah", "Jurong", "Kranji", "MacPherson", "Potong Pasir" };
	string businessKeyWords[] = { "Budget", "cents", "Economic Development Board", "economy", "economic", "finance", "Finance", "income", "recession", "salary", "stock", "tax", "DBS", "EDB", "GDP", "GST", "MAS", "SGX", "STI", "$" };
	Document doc;
	bool canCategorise = false;
	vector<Article> tocArticles;
	//Creates new child article objects
	TOC toc;
	CNA cna;
	//Initialize the query URL to use for News API to return all news articles belonging to The Online Citizen
	string getNewsArticlesURL = "http://newsapi.org/v2/everything?domains=";
	getNewsArticlesURL.append(TOCURL);
	getNewsArticlesURL.append("&pageSize=" + to_string(PAGESIZE));
	getNewsArticlesURL.append("&sortBy=publishedAt");
	getNewsArticlesURL.append("&apiKey=" + APIKEY);
	//Handles the HTTP REST Request
	//Source: https://www.codeproject.com/Articles/1244632/Making-HTTP-REST-Request-in-Cplusplus
	auto r = cpr::Get(cpr::Url{ getNewsArticlesURL });
	//Configures the console to allow displaying of all characters encoded to UTF-8
	//Source: https://stackoverflow.com/questions/2492077/output-unicode-strings-in-windows-console-app
	SetConsoleOutputCP(CP_UTF8);
	//Parses the document
	doc.Parse(r.text.c_str());
	cout << "News Articles retrieved from " << TOCURL << endl;
	//Deencapsulates the JSON layer to retrieve the values from specified attributes (keys)
	//Source: https://rapidjson.org/md_doc_tutorial.html
	for (SizeType i = 0; i < 20; i++) {
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
		if (doc["articles"][i].HasMember("source")) {
			if (doc["articles"][i]["source"].HasMember("name")) {
				if (doc["articles"][i]["source"]["name"].IsString()) {
					source = doc["articles"][i]["source"]["name"].GetString();
					toc.setSource(source);
				}
			}
		}
		if (doc["articles"][i].HasMember("title")) {
			if (doc["articles"][i]["title"].IsString()) {
				title = doc["articles"][i]["title"].GetString();
				//This is to determine whether the news articles belong to Singapore, Business or world via the defined keywords in the array from the article's title attribute from JSON
				while (checkIndex < businessKeyWordLen) {
					//If is not categorised yet
					if (!canCategorise) {
						//Perform a comparison to check if the content matches the business keywords from the array
						if (strstr(title.c_str(), businessKeyWords[checkIndex].c_str())) {
							//If the comparison matches, the news is determined to be a business news
							category = "Business";
							canCategorise = true;
						}
						else {
							checkIndex++;
						}
					}
					//Exit the while loop if the Business category is already determined
					else {
						break;
					}
				}
				//Reset index count to determine Singapore News next, if cannot determine Business news
				checkIndex = 0;
				while (checkIndex < singaporeKeyWordLen) {
					if (!canCategorise) {
						//Perform a comparison to check if the content matches the Singapore keywords from the array
						if (strstr(title.c_str(), singaporeKeyWords[checkIndex].c_str())) {
							//If the comparison matches, the news is determined to be a Singapore news
							category = "Singapore";
							canCategorise = true;
						}
						else {
							checkIndex++;
						}
					}
					//Exit the while loop if the Singapore category is already determined
					else {
						break;
					}
				}
				toc.setCategory(category);
			}
			toc.setTitle(title);
		}
		if (doc["articles"][i].HasMember("url")) {
			url = doc["articles"][i]["url"].GetString();
			toc.setURL(url);
		}
		if (doc["articles"][i].HasMember("publishedAt")) {
			date = doc["articles"][i]["publishedAt"].GetString();
			if (sscanf(date.c_str(), "%d-%d-%d", &year, &month, &day)) {
				date = to_string(day) + "/" + to_string(month) + "/" + to_string(year);
			}
			else {
				cout << "Error parsing date!" << endl;
			}
			toc.setDate(date);
		}
		checkIndex = 0;
		if (doc["articles"][i].HasMember("description")) {
			if (doc["articles"][i]["description"].IsString()) {
				description = doc["articles"][i]["description"].GetString();
				//This is to determine whether the news articles belong to Singapore, Business or world via the defined keywords in the array from the article's description attribute from JSON
				while (checkIndex < businessKeyWordLen) {
					//If is not categorised yet
					if (!canCategorise) {
						//Perform a comparison to check if the content matches the business keywords from the array
						if (strstr(description.c_str(), businessKeyWords[checkIndex].c_str())) {
							//If the comparison matches, the news is determined to be a business news
							category = "Business";
							canCategorise = true;
						}
						else {
							checkIndex++;
						}
					}
					//Exit the while loop if the Business category is already determined
					else {
						break;
					}
				}
				//Reset index count to determine Singapore News next, if cannot determine Business news
				checkIndex = 0;
				while (checkIndex < singaporeKeyWordLen) {
					if (!canCategorise) {
						//Perform a comparison to check if the content matches the Singapore keywords from the array
						if (strstr(description.c_str(), singaporeKeyWords[checkIndex].c_str())) {
							//If the comparison matches, the news is determined to be a Singapore news
							category = "Singapore";
							canCategorise = true;
						}
						else {
							checkIndex++;
						}
					}
					//Exit the while loop if the Singapore category is already determined
					else {
						break;
					}
				}
				//If any of the content does not match Business/Singapore keywords, the news will be categorised into World news
				if (!canCategorise) {
					category = "World";
				}
				toc.setCategory(category);
			}
			toc.setContent(description);
		}
		//Add the objects to the vector array
		tocArticles.push_back(toc);
	}
	//Populate into a CSV file
	toc.createCSV(THEONLINECITIZEN, tocArticles);
	//Iterate and display the crawling results into console
	for (int i = 0; i < tocArticles.size(); i++) {
		cout << "News Article #" << i + 1 << endl;
		cout << "Title: " << tocArticles[i].getTitle() << endl;
		cout << "Source: " << tocArticles[i].getSource() << endl;
		cout << "Date: " << tocArticles[i].getDate() << endl;
		cout << "Description: " << tocArticles[i].getContent() << endl;
		cout << "URL: " << tocArticles[i].getURL() << endl;
		cout << "Category: " << tocArticles[i].getCategory() << endl;
		cout << endl;
	}
	cout << "Meaningcloud Results" << endl;
	//Returns the list of classifications
	vector<string> getClassifications = toc.analyzeClassification(THEONLINECITIZEN, tocArticles);
	map<string, int> cQuantity;
	cout << "Get Classification:" << endl;
	//Iterate the classifications retrieved by the MeaningCloud API
	//Source: https://stackoverflow.com/questions/34292384/counting-occurrences-of-integers-in-map-c
	for (int i = 0; i < getClassifications.size(); i++) {
		cout << getClassifications[i] << endl;
		//If the classification is NOT empty, perform the adding of quantity else ignore it
		if (getClassifications[i] != "") {
			//If the key is not present in the map, add the new key (classification) and its value (occurence)
			if (cQuantity.find(getClassifications[i]) == cQuantity.end()) {
				cQuantity.insert(std::pair<string, int>(getClassifications[i], 1));//single count of current number
			}
			else {
				//If the key is not present in the map, simply update its value (occurence) by incrementing
				cQuantity[getClassifications[i]]++;
			}
		}
	}
	cout << "All Classifications: " << endl;
	//Iterate every entry of the map
	for (std::map<string, int>::iterator it = cQuantity.begin(); it != cQuantity.end(); it++) {
		cout << it->first << ": " << it->second << " time(s)" << endl;
	}
	//Source: https://www.educative.io/edpresso/how-to-sort-a-map-by-value-in-cpp
	//Create a empty vector of pairs
	vector<pair<string, int>> cQuantityVec;
	//Copy key-value pairs from the map to the newly created vector
	for (std::map<string, int>::iterator it = cQuantity.begin(); it != cQuantity.end(); it++) {
		cQuantityVec.push_back(make_pair(it->first, it->second));
	}
	//Sort the vector by decreasing order of its pair's second value, this will be populated into the bar chart to display the Top 5 News Classifications
	sort(cQuantityVec.begin(), cQuantityVec.end(), sortByValDesc);
	cout << "All Classifications (Sorted) In Descending Order: " << endl;
	for (int j = 0; j < cQuantityVec.size(); j++) {
		cout << cQuantityVec[j].first << ": " << cQuantityVec[j].second << " time(s)" << endl;
	}
	//Returns the list of sentiment results that will be populated to pie chart
	vector<int> getSentiments = toc.sentimentAnalysis(tocArticles);
	cout << "Get Sentiments:" << endl;
	for (int k = 0; k < getSentiments.size(); k++) {
		cout << getSentiments[k] << endl;
	}
}