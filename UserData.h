#pragma once

struct ReviewData {
	unsigned int movie_id;
	float rating;

	// DEBUG
	/*
	void print() {
		cout << "MUI: " << movie_id << " | Rating: " << rating << endl;
	}
	*/
};

struct UserData {
	unsigned int review_start;
	unsigned int review_end;

	// DEBUG
	/*
	void print() {
		cout << "Start: " << review_start << " | End: " << review_end << endl;
	}
	*/
};
