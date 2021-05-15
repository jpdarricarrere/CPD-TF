#pragma once

struct TagData {
	const string tag;
	vector<unsigned int> films;
	TagData* next_data;

	void add_film(unsigned int film) {
		for (unsigned int entry : films) {
			if (entry == film) {
				return; // Already here, avoid duplicates.
			}
		}
		films.push_back(film);
	};

	// DEBUG
	/*
	void print_rec() {
		cout << "\tTag: " << tag << " | Films:";
		for (unsigned int id : films) {
			cout << " " << id;
		}
		cout << endl;
		if (next_data != NULL) {
			next_data->print_rec();
		}
	};
	*/
};

//#define NUMBER_OF_TAGS 38521
// More or less a third number of tags, prime: https://www.planetmath.org/goodhashtableprimes
#define TAG_TABLE_SIZE_PRIME 12289
// Exponential progression to handle collisions, prime.
#define TAG_TABLE_STEP_PRIME 53

struct TagHashTable{
	TagData* tag_data[TAG_TABLE_SIZE_PRIME];

	unsigned short int hash_string(string &key) {
		unsigned int hash = 0;
		unsigned short int iter = 1;
		for (char letter : key) {
			hash += TAG_TABLE_STEP_PRIME * letter * iter++;
		}
		return hash % TAG_TABLE_SIZE_PRIME;
	};

	void insert_tag(unsigned int film, string tag) {
		unsigned short int key = hash_string(tag);
		TagData* old_data = NULL;
		TagData* data = tag_data[key];
		for (;;) {
			if (data == NULL) {
				data = new TagData{ tag };
				data->add_film(film);
				if (old_data == NULL) {
					tag_data[key] = data;
				}
				else {
					old_data->next_data = data;
				}
				break;
			}
			if (data->tag == tag) {
				data->add_film(film);
				break;
			}
			old_data = data;
			data = data->next_data;
		}
	};

	TagData* extract_data(string tag) {
		TagData* data = tag_data[hash_string(tag)];
		for (;;) {
			if (data == NULL) {
				return NULL;
			}
			if (data->tag == tag) {
				return data;
			}
			data = data->next_data;
		}
	}

	vector<unsigned int> tag_intersection(vector<string> &tags) {
		vector<unsigned int> accumulator;
		vector<TagData*> valid_data;
		for (string tag : tags) {
			TagData* data = extract_data(tag);
			if (data == NULL) {
				return accumulator; // Invalid tag, intersection of anything with empty is empty. Return empty handed.
			}
			valid_data.push_back(data);
		}
		size_t valid_data_size = valid_data.size();
		if (valid_data_size == 0) {
			return accumulator; // No valid tags. Return empty handed.
		}
		accumulator = valid_data[0]->films;
		if (valid_data_size == 1) {
			return accumulator; // Intersection of A with A is A.
		}
		for (unsigned int i = 1; i < valid_data_size; i++) {
			TagData* data = valid_data[i];
			vector<unsigned int> intermediate = accumulator;
			accumulator.clear();
			for (unsigned int shared_id : intermediate) {
				for (unsigned int comparisson_id : data->films) {
					if (shared_id == comparisson_id) {
						accumulator.push_back(shared_id);
						break;
					}
				}
			}

		}
		return accumulator;
	}

	// DEBUG
	/*
	void print_tags() {
		for (unsigned int i = 0; i < TAG_TABLE_SIZE_PRIME; i++) {
			cout << i << endl;
			TagData* data = tag_data[i];
			if (data == NULL) {
				cout << "\tNULL" << endl;
				continue;
			}
			data->print_rec();
		}
	}

	unsigned short int free_slots() {
		unsigned short int counter = 0;
		for (TagData* data : tag_data) {
			if (data == NULL) {
				counter++;
			}
		}
		return counter;
	}
	*/
};
