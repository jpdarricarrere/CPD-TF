#pragma once

struct FilmData {
	const unsigned int id;
	const string title;
	const vector<string> genres;
	float global_rating = 0;
	unsigned short int total_ratings = 0;

	void add_review(float review) {
		global_rating = (review + (global_rating * total_ratings)) / (total_ratings + 1);
		total_ratings++;
	}

	// DEBUG
	/*
	void print_data() {
		cout << "Id: " << id << " | Title: " << title << endl;
	}
	*/
};
