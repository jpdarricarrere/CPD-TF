#pragma once

#define NUMBER_OF_MOVIES 27278
// More or less double the number of movies, prime: https://www.planetmath.org/goodhashtableprimes
#define FLIM_TABLE_SIZE_PRIME 49157
// Linear progression to handle collisions, prime.
#define FILM_TABLE_STEP_PRIME 3079

struct FilmHashTable {
	unsigned short int index_table[FLIM_TABLE_SIZE_PRIME] = {};
	unsigned short int max_index = 1;
	FilmData* data_table[NUMBER_OF_MOVIES + 1]; // Plus one to account for the invalid zero.

	unsigned short int hash_number(unsigned int value) {
		unsigned short int hash = (value + FILM_TABLE_STEP_PRIME) % FLIM_TABLE_SIZE_PRIME;
		if (hash == 0) {
			hash = FILM_TABLE_STEP_PRIME;
		}
		return hash;
	};

	void insert_movie(unsigned int movie_id, string &title, vector<string> &genres_vector) {
		unsigned int key = movie_id;
		for (;;) {
			key = hash_number(key);
			unsigned short int index = index_table[key];
			if (index != 0) {
				continue;
			}
			data_table[max_index] = new FilmData{ movie_id, title, genres_vector };
			index_table[key] = max_index++;
			break;
		};
	};

	FilmData* get_movie(unsigned int id) {
		unsigned int key = id;
		for (;;) {
			key = hash_number(key);
			unsigned short int index = index_table[key];
			if (index == 0) {
				return NULL;
			}
			FilmData* movie = data_table[index];
			if (movie->id != id) {
				continue;
			}
			return movie;
		}
	}

	void add_review_unsafe(unsigned int movie_id, float review) {
		FilmData* movie = get_movie(movie_id);
		movie->add_review(review);
	}

	// DEBUG
	/*
	void print_movies() {
		for (FilmData* data : data_table) {
			data->print_data();
		}
	};

	void print_table_occupation() {
		unsigned int counter = 0;
		for (unsigned short int index : index_table) {
			cout << (index == 0 ? 'X' : 'O');
			if (++counter % 100 == 0) {
				cout << endl;
			}
		}
		cout << endl;
	}
	*/
};
