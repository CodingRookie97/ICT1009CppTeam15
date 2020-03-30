#pragma once
#include <iostream>
#include "Article.h"
using namespace std;
void Article::setTitle(string t) {
	this->title = t;
}
string Article::getTitle() {
	return this->title;
}
void Article::setSource(string s) {
	this->source = s;
}
string Article::getSource() {
	return this->source;
}
void Article::setDate(string d) {
	this->date = d;
}
string Article::getDate() {
	return this->date;
}
void Article::setContent(string c) {
	this->content = c;
}
string Article::getContent() {
	return this->content;
}
void Article::setURL(string url) {
	this->url = url;
}
string Article::getURL() {
	return this->url;
}
void Article::setCategory(string c) {
	this->category = c;
}
string Article::getCategory() {
	return this->category;
}
ostream& operator<<(ostream& out, Article& news)
{
	out << "Title: " << news.getTitle() << endl;
	out << "Source: " << news.getSource() << endl;
	out << "Date: " << news.getDate() << endl;
	out << "Description: " << news.getContent() << endl;
	out << "URL: " << news.getURL() << endl;
	out << "Category: " << news.getCategory() << endl;
	out << endl;

	return out;
}