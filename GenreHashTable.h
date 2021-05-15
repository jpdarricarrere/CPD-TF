#pragma once

#define GENRES_SIZE 53 // Prime number.
#define NUM_OF_GENRES 19 // Also a prime number.
#define GENRE_STEP NUM_OF_GENRES
#define TOP_TEN_MAX 10

struct GenreData {
	const string name;
	vector<FilmData*> ordered_table;

	void feed_film(FilmData* movie) {
		size_t table_len = ordered_table.size();
		for (unsigned int i = 0; i < table_len; i++) {
			FilmData* existing_movie = ordered_table[i];
			if (existing_movie->global_rating >= movie->global_rating) {
				continue;
			}
			ordered_table.insert(ordered_table.begin() + i, movie);
			return;
		}
		ordered_table.push_back(movie);
	};
};

struct GenreHashTable {
	GenreData* genre_table[GENRES_SIZE];

	unsigned short int hash_string(string &key) {
		unsigned int hash = 0;
		for (char i = 1; i <= 3; i++) {
			hash += GENRE_STEP * key[i] * i;
		}
		return hash % GENRES_SIZE;
	};

	unsigned short int hash_number(unsigned short int value) {
		unsigned short int hash = (value + GENRE_STEP) % GENRES_SIZE;
		return hash;
	};

	GenreData* get_genre_data(string genre_name) {
		string lower_case_input = genre_name;
		transform(lower_case_input.begin(), lower_case_input.end(), lower_case_input.begin(), ::tolower);
		unsigned short int hash = hash_string(lower_case_input);
		GenreData* genre_data = genre_table[hash];
		if (genre_data == NULL) {
			return NULL;
		}
		string lower_case_existing = genre_data->name;
		transform(lower_case_existing.begin(), lower_case_existing.end(), lower_case_existing.begin(), ::tolower);
		if (lower_case_existing == lower_case_input) {
			return genre_data;
		}
		for (;;) {
			hash = hash_number(hash);
			genre_data = genre_table[hash];
			if (genre_data == NULL) {
				return NULL;
			}
			lower_case_existing = genre_data->name;
			transform(lower_case_existing.begin(), lower_case_existing.end(), lower_case_existing.begin(), ::tolower);
			if (lower_case_existing == lower_case_input) {
				return genre_data;
			}
		}
	};

	GenreData* get_or_insert_genre_data(string genre_name) {
		string lower_case_input = genre_name;
		transform(lower_case_input.begin(), lower_case_input.end(), lower_case_input.begin(), ::tolower);
		unsigned short int hash = hash_string(lower_case_input);
		GenreData* genre_data = genre_table[hash];
		if (genre_data == NULL) {
			genre_data = new GenreData{ genre_name };
			genre_table[hash] = genre_data;
			return genre_data;
		}
		string lower_case_existing = genre_data->name;
		transform(lower_case_existing.begin(), lower_case_existing.end(), lower_case_existing.begin(), ::tolower);
		if (lower_case_existing == lower_case_input) {
			return genre_data;
		}
		for (;;) {
			hash = hash_number(hash);
			genre_data = genre_table[hash];
			if (genre_data == NULL) {
				genre_data = new GenreData{ genre_name };
				genre_table[hash] = genre_data;
				return genre_data;
			}
			lower_case_existing = genre_data->name;
			transform(lower_case_existing.begin(), lower_case_existing.end(), lower_case_existing.begin(), ::tolower);
			if (lower_case_existing == lower_case_input) {
				return genre_data;
			}
		}
	};

	void feed_film(FilmData* movie) {
		for (string genre_name : movie->genres) {
			GenreData* data = get_or_insert_genre_data(genre_name);
			data->feed_film(movie);
		}
	};

	vector<string> get_all_genre_names() {
		vector<string> accumulator;
		for (GenreData* data : genre_table) {
			if (data == NULL) {
				continue;
			}
			accumulator.push_back(data->name);
		}
		return accumulator;
	}

	// DEBUG
	/*
	void print() {
		for (GenreData* data : genre_table) {
			if (data == NULL) {
				continue;
			}
			cout << "" << data->name << endl;
			for (int i = 0; i < 10; i++) {
				FilmData* movie = data->ordered_table[i];
				if (movie == NULL) {
					break;
				}
				cout << "\t" << i << ") " << movie->title << " | Rating: " << movie->global_rating << " | Reviews: " << movie->total_ratings << endl;
			}
			cout << endl;
		}
	};
	*/
};
