#pragma once
#include <iostream>
#include <algorithm>
#include <cpr/cpr.h>
#include <windows.h>
#include "rapidjson/document.h"
#include "Article.h"
#include "News.h"

using namespace std;
using namespace rapidjson;

//Source: https://www.educative.io/edpresso/how-to-sort-a-map-by-value-in-cpp
//This is the utility comparator function to compare the occurences (value) in descending order and pass to the sort() module
bool sortByValDesc(const pair<string, int> &a, const pair<string, int> &b)
{
	return (a.second > b.second);
}
int main(void)
{
	const string WEBSITES[] = { "TOC", "CNA", "TST", "thestar" };
	News news;
	int flag;

	// Temporary Input
	cout << "Enter source number (0: TOC, 1: CNA, 2: TST, 3: thestar): ";
	cin >> flag;
	// Temporary Input

	vector<Article> newsArticles = news.crawl(flag);
	//Populate into a CSV file
	news.createCSV(WEBSITES[flag], newsArticles);
	//Iterate and display the crawling results into console
	for (int i = 0; i < newsArticles.size(); i++)
	{
		cout << "News Article #" << i + 1 << endl;
		cout << "Title: " << newsArticles[i].getTitle() << endl;
		cout << "Source: " << newsArticles[i].getSource() << endl;
		cout << "Date: " << newsArticles[i].getDate() << endl;
		cout << "Description: " << newsArticles[i].getContent() << endl;
		cout << "URL: " << newsArticles[i].getURL() << endl;
		cout << "Category: " << newsArticles[i].getCategory() << endl;
		cout << endl;
	}
	cout << "Meaningcloud Results" << endl;
	//Returns the list of classifications
	vector<string> getClassifications = news.analyzeClassification(WEBSITES[flag], newsArticles);
	map<string, int> cQuantity;
	cout << "Get Classification:" << endl;
	//Iterate the classifications retrieved by the MeaningCloud API
	//Source: https://stackoverflow.com/questions/34292384/counting-occurrences-of-integers-in-map-c
	for (int i = 0; i < getClassifications.size(); i++)
	{
		cout << getClassifications[i] << endl;
		//If the classification is NOT empty, perform the adding of quantity else ignore it
		if (getClassifications[i] != "")
		{
			//If the key is not present in the map, add the new key (classification) and its value (occurence)
			if (cQuantity.find(getClassifications[i]) == cQuantity.end())
			{
				cQuantity.insert(std::pair<string, int>(getClassifications[i], 1)); //single count of current number
			}
			else
			{
				//If the key is not present in the map, simply update its value (occurence) by incrementing
				cQuantity[getClassifications[i]]++;
			}
		}
	}
	cout << "All Classifications: " << endl;
	//Iterate every entry of the map
	for (std::map<string, int>::iterator it = cQuantity.begin(); it != cQuantity.end(); it++)
	{
		cout << it->first << ": " << it->second << " time(s)" << endl;
	}
	//Source: https://www.educative.io/edpresso/how-to-sort-a-map-by-value-in-cpp
	//Create a empty vector of pairs
	vector<pair<string, int>> cQuantityVec;
	//Copy key-value pairs from the map to the newly created vector
	for (std::map<string, int>::iterator it = cQuantity.begin(); it != cQuantity.end(); it++)
	{
		cQuantityVec.push_back(make_pair(it->first, it->second));
	}
	//Sort the vector by decreasing order of its pair's second value, this will be populated into the bar chart to display the Top 5 News Classifications
	sort(cQuantityVec.begin(), cQuantityVec.end(), sortByValDesc);
	cout << "All Classifications (Sorted) In Descending Order: " << endl;
	for (int j = 0; j < cQuantityVec.size(); j++)
	{
		cout << cQuantityVec[j].first << ": " << cQuantityVec[j].second << " time(s)" << endl;
	}
	//Returns the list of sentiment results that will be populated to pie chart
	vector<int> getSentiments = news.sentimentAnalysis(newsArticles);
	cout << "Get Sentiments:" << endl;
	for (int k = 0; k < getSentiments.size(); k++)
	{
		cout << getSentiments[k] << endl;
	}
}