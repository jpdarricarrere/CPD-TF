#pragma once

// From ASCII 32 to 126 = 94 possibilities
#define TRIE_FIRST_CHAR 32 // Whitespace
#define TRIE_LAST_CHAR 126 // ~
#define TRIE_NUM_ELEM (TRIE_LAST_CHAR - TRIE_FIRST_CHAR + 1)

struct FilmTrieNode {
	unsigned int movie_id = NULL;
	FilmTrieNode* branches[TRIE_NUM_ELEM]{};

	char letter_to_trie_index(char letter) {
		return letter - TRIE_FIRST_CHAR;
	}

	char trie_index_to_letter(char index) {
		return index + TRIE_FIRST_CHAR;
	}

	void insert_movie_id(unsigned int id) {
		movie_id = id;
	}

	FilmTrieNode* go_to_node(char letter) {
		return branches[letter_to_trie_index(letter)];
	};

	FilmTrieNode* search_or_insert_node(char letter) {
		char index = letter_to_trie_index(letter);
		FilmTrieNode* letter_node_ptr = branches[index];
		if (letter_node_ptr == NULL) {
			letter_node_ptr = new FilmTrieNode{};
			branches[index] = letter_node_ptr;
		}
		return letter_node_ptr;
	};

	unsigned int search_movie_id(string key) {
		FilmTrieNode* current_node = this;
		char current_index = 0;
		for (char letter : key) {
			current_index = letter_to_trie_index(letter);
			current_node = current_node->branches[current_index];
			if (current_node == NULL) {
				return NULL;
			}
		}
		if (current_node->movie_id == NULL) {
			return NULL;
		}
		return current_node->movie_id;
	};

	FilmTrieNode* search_node(string key) {
		FilmTrieNode* current_node = this;
		for (char c : key) {
			current_node = current_node->go_to_node(c);
			if (current_node == NULL) {
				break;
			}
		}
		return current_node;
	};

	void scan_movie_ids_rec(vector<unsigned int>& uis_vector) {
		if (movie_id != NULL) {
			uis_vector.push_back(movie_id);
		}
		for (char i = 0; i < TRIE_NUM_ELEM; i++) {
			FilmTrieNode* node = branches[i];
			if (node == NULL) {
				continue;
			}
			node->scan_movie_ids_rec(uis_vector);
		}
	};

	vector<unsigned int> scan_movies_by_prefix(string prefix) {
		FilmTrieNode* current_node = search_node(prefix);
		vector<unsigned int> uids;
		if (current_node == NULL) {
			return uids;
		}
		current_node->scan_movie_ids_rec(uids);
		return uids;
	}

	void insert_movie(unsigned int id, string key) {
		FilmTrieNode* current_node = this;
		for (char letter : key) {
			current_node = current_node->search_or_insert_node(letter);
		}
		current_node->insert_movie_id(id);
	};

	// DEBUG
	/*
	void print_node_rec(string accumulated) {
		if (movie_id != NULL) {
			cout << "Key: " << accumulated << " | Movie ID: " << movie_id << endl;
		}
		for (char i = 0; i < TRIE_NUM_ELEM; i++) {
			FilmTrieNode* node = branches[i];
			if (node == NULL) {
				continue;
			}
			string branch_accumulated = accumulated;
			branch_accumulated += trie_index_to_letter(i);
			node->print_node_rec(branch_accumulated);
		}
	};

	void print_movies_from_prefix(string prefix) {
		vector<unsigned int> uids = scan_movies_by_prefix(prefix);
		cout << "Found " << uids.size() << " after scanning prefix \"" << prefix << '\"' << endl;
		for (unsigned int uid_found : uids) {
			cout << "UID: " << uid_found << endl;
		}
	}
	*/
};
