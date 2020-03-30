#pragma once
#include <iostream>
#include <vector>
using namespace std;
class Article
{
	friend ostream& operator<<(ostream&, Article&);
private:
	string title;
	string source;
	string date;
	string content;
	string url;
	string category;
public:
	void setTitle(string);
	string getTitle();
	void setSource(string);
	string getSource();
	void setDate(string);
	string getDate();
	void setContent(string);
	string getContent();
	void setURL(string);
	string getURL();
	void setCategory(string);
	string getCategory();
	void createCSV(string, vector<Article>);
	vector<string> analyzeClassification(string, vector<Article>);
	void createAnalyzedCSV(string, vector<Article>, vector<string>, vector<string>);
	vector<int> sentimentAnalysis(vector<Article>);
	vector<Article> crawl(int);
};


