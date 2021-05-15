using namespace std;

#include <chrono>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <algorithm>
#include <regex>
#include "FilmData.h"
#include "FilmHashTable.h"
#include "GenreHashTable.h"
#include "TagHashTable.h"
#include "FilmTrie.h"
#include "UserData.h"
#include "QueryLogic.h"

#define LINE_BUFFER_MEDIUM 4096
#define LINE_BUFFER_SMALL 512
#define TOTAL_USERS 138494
#define TOTAL_REVIEWS 20000263
#define MIN_REVIEWS_FOR_TOP 1000
#define DECIMAL 10

int main()
{
    cout << "Program start..." << endl << endl << "Initializing data structures..." << endl << endl;
    using milli = chrono::milliseconds;
    auto start_main = chrono::high_resolution_clock::now();

    regex escaped_quotes_regex("(\\\\\"\\\\)"); // Had to escape the \ twice, once for the string, once for the regex, resulting in 4 meaning 1.

    auto start_movie = chrono::high_resolution_clock::now();
    FilmTrieNode movies_trie = {};
    FilmHashTable film_table = {};
    {
        filebuf fb;
        if (!fb.open("movie_clean.csv", ios::in)) {
            cout << "ERROR: Failed to read file \"movie_clean.csv\"" << endl;
            return EXIT_FAILURE;
        }
        istream is(&fb);

        char line[LINE_BUFFER_MEDIUM];
        is.getline(line, LINE_BUFFER_MEDIUM); // Extract the first line.
        while (is.getline(line, LINE_BUFFER_MEDIUM)) {
            const size_t line_len = strlen(line);
            if (line_len == 0) {
                break;
            }
            char* next_word;
            const unsigned int movie_id = strtoul(line, &next_word, DECIMAL);
            next_word += 2; // Skip comma and quote.
            char* word_end = find(next_word, line + line_len, '"');
            bool escape_chars = false;
            for (;;) {
                const char* previous_char = word_end - 1;
                if (*previous_char != '\\') {
                    break; // Check if the quotes are escaped.
                }
                escape_chars = true;
                word_end = find(word_end + 1, line + line_len, '"');
            }
            string title(next_word, word_end);
            if (escape_chars) { // Sanitize string.
                title = regex_replace(title, escaped_quotes_regex, "");
            }
            next_word = word_end + 3; // Skip comma and two quote signs.
            word_end = find(next_word, line + line_len, '"');
            *word_end = NULL; //Let's remove the final quotes and end the string there.
            vector<string> genres_vector;
            const char* delim = "|";
            char* genres_token = strtok_s(next_word, delim, &word_end);
            if (genres_token != NULL && genres_token[0] != '(') { // NULL or "(no genres listed)"
                do {
                    genres_vector.push_back(genres_token);
                    genres_token = strtok_s(NULL, delim, &word_end);
                } while (genres_token != NULL);
            }
            film_table.insert_movie(movie_id, title, genres_vector);
            movies_trie.insert_movie(movie_id, title);
        }
    }
    auto finish_movie = chrono::high_resolution_clock::now();

    auto start_rating = chrono::high_resolution_clock::now();
    UserData* user_array = new UserData[TOTAL_USERS];
    ReviewData* reviews_array = new ReviewData[TOTAL_REVIEWS];
    {
        filebuf fb;
        if (!fb.open("rating.csv", ios::in)) {
            cout << "ERROR: Failed to read file \"rating.csv\"" << endl;
            return EXIT_FAILURE;
        }
        istream is(&fb);

        unsigned int current_review_index = 0;
        unsigned int last_id = 0;
        unsigned int last_review_index = 0;
        unsigned int current_id;
        char line[LINE_BUFFER_SMALL];
        is.getline(line, LINE_BUFFER_SMALL); // Extract the first line.
        while (is.getline(line, LINE_BUFFER_SMALL) && strlen(line) > 0) {
            char* next_word;
            current_id = strtoul(line, &next_word, DECIMAL);
            const unsigned int movie_id = strtoul(next_word + 1, &next_word, DECIMAL); // +1 to skip the comma.
            const float rating = strtof(next_word + 1, NULL); // +1 to skip the comma.
            if (current_id != last_id) {
                user_array[last_id] = UserData{ last_review_index , current_review_index };
                last_review_index = current_review_index;
                last_id = current_id;
            }
            reviews_array[current_review_index++] = ReviewData{ movie_id, rating };
        }
        user_array[last_id] = UserData{ last_review_index , current_review_index }; // For the final one.
        fb.close();
    }
    
    for (unsigned int i = 1; i < TOTAL_USERS; i++) {
        UserData user = user_array[i];
        unsigned int review_end = user.review_end;
        for (unsigned int j = user.review_start; j < review_end; j++) {
            ReviewData review = reviews_array[j];
            film_table.add_review_unsafe(review.movie_id, review.rating);
        }
    }
    auto finish_rating = chrono::high_resolution_clock::now();

    auto start_genre = chrono::high_resolution_clock::now();
    GenreHashTable genre_table{};
    for (FilmData* movie : film_table.data_table) {
        if (movie == NULL || movie->total_ratings < MIN_REVIEWS_FOR_TOP) {
            continue;
        }
        genre_table.feed_film(movie);
    }
    auto finish_genre = chrono::high_resolution_clock::now();
    
    auto start_tag = chrono::high_resolution_clock::now();
    TagHashTable tag_table = {};
    {
        filebuf fb;
        if (!fb.open("tag_clean.csv", ios::in)) {
            cout << "ERROR: Failed to read file \"tag_clean.csv\"" << endl;
            return EXIT_FAILURE;
        }
        istream is(&fb);
        char line[LINE_BUFFER_MEDIUM];
        is.getline(line, LINE_BUFFER_MEDIUM); // Extract the first line.
        while (is.getline(line, LINE_BUFFER_MEDIUM)) {
            const size_t line_len = strlen(line);
            if (line_len == 0) {
                break;
            }
            char* next_word = find(line, line + line_len, ',') + 1; // Skip the user ID and comma.
            unsigned int movie_id = strtoul(next_word, &next_word, DECIMAL);
            next_word += 2; // Skip comma and quotation mark.
            char* word_end = find(next_word, line + line_len, '\"');
            bool escape_chars = false;
            for (;;) {
                const char* previous_char = word_end - 1;
                const char* next_char = word_end + 1;
                if (*previous_char != '\\' || *next_char == ',') {
                    break; // Check if the quotes are escaped, plus funny edge case in where \ is not actually being used to escape the quotes.
                }
                escape_chars = true;
                word_end = find(word_end + 1, line + line_len, '"');
            }
            string tag(next_word, word_end);
            if (escape_chars) { // Sanitize string.
                tag = regex_replace(tag, escaped_quotes_regex, "");
            }
            tag_table.insert_tag(movie_id, tag);
        }
        fb.close();
    }
    auto finish_tag = chrono::high_resolution_clock::now();

    auto finish_main = chrono::high_resolution_clock::now();
    cout << "Movie initialization time: " << chrono::duration_cast<milli>(finish_movie - start_movie).count() << " milliseconds" << endl;
    cout << "Rating initialization time: " << chrono::duration_cast<milli>(finish_rating - start_rating).count() << " milliseconds" << endl;
    cout << "Genre initialization time: " << chrono::duration_cast<milli>(finish_genre - start_genre).count() << " milliseconds" << endl;
    cout << "Tag initialization time: " << chrono::duration_cast<milli>(finish_tag - start_tag).count() << " milliseconds" << endl;
    cout << "Total initialization time: " << chrono::duration_cast<milli>(finish_main - start_main).count() << " milliseconds" << endl << endl;

    {
        print_input_request();

        const string profile_command = "profile";
        const string movie_command = "movie";
        const string user_command = "user";
        const string tags_command = "tags";
        const string top_command = "top";
        const string help_command = "help";
        const string exit_command = "exit";

        const string topics_help_command = "topics";
        const string genres_help_command = "genres";

        string input;
        for (;;) {
            getline(cin, input);
            stringstream input_stream(input);

            string token;
            if (!getline(input_stream, token, ' ') || token.size() < 4) {
                cout << endl << "Failed to detect valid input." << endl;
                if(token.size() > 1) {
                    cout << "Please double-check the spelling of the first command, if you inserted what you expected to be valid input." << endl;
                    cout << "Use \"help\" (without the quotes) for valid commands, or \"exit\" to leave the program.";
                }
                print_input_request();
                continue;
            }
            transform(token.begin(), token.end(), token.begin(), ::tolower);
            if (token == movie_command) {
                if (!getline(input_stream, token) || token.size() == 0) {
                    cout << endl << "Failed to detect valid movie title input." << endl;
                    cout << "Try adding a movie title or prefix to the query." << endl;
                    cout << "Example query: movie Star Wa" << endl << endl;
                    print_input_request();
                    continue;
                }
                vector<unsigned int> query_result = movies_trie.scan_movies_by_prefix(token);
                if (query_result.size() == 0) {
                    cout << endl << "No movies found matching given prefix or title." << endl;
                    print_input_request();
                    continue;
                }
                if (query_result.size() > 1) {
                    sort(query_result.begin(), query_result.end());
                }
                cout << endl << "* movieid\t* rating\t* count\t* title\t\t* genres" << endl;
                cout << "vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv" << endl << endl;
                for (unsigned int movie_id : query_result) {
                    FilmData* movie_data = film_table.get_movie(movie_id);
                    cout << endl << "* " << movie_data->id << "\t\t* " << movie_data->global_rating << "\t* " << movie_data->total_ratings << "\t* " << movie_data->title << "\t* ";
                    size_t genre_len = movie_data->genres.size();
                    if (genre_len == 0) {
                        cout << "No Genres Listed";
                    }
                    else {
                        genre_len--; // Take the last out to avoid a trailing |
                        for (unsigned int i = 0; i < genre_len; i++) {
                            cout << "" << movie_data->genres[i] << '|';
                        }
                        cout << "" << movie_data->genres[genre_len];
                    }
                    cout << endl;
                }
                cout << endl << "^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^" << endl;
                cout << "* movieid\t* rating\t* count\t* title\t\t* genres" << endl << endl;
                print_input_request_successful();
                continue;
            }
            if (token == user_command) {
                if (!getline(input_stream, token, ' ') || token.size() < 1) {
                    cout << endl << "Failed to detect valid user id input." << endl;
                    cout << "Insert a number after the user comand." << endl;
                    cout << "Example query: user 4" << endl << endl;
                    print_input_request();
                    continue;
                }
                istringstream token_stream(trim(token));
                int user_id_input;
                if (!(token_stream >> user_id_input) || user_id_input < 1 || user_id_input >= TOTAL_USERS) {
                    cout << endl << "Failed to detect valid user id input." << endl;
                    cout << "Insert a valid number from 1 to " << (TOTAL_USERS - 1) << "after the user comand." << endl;
                    cout << "Example query: user 32767" << endl << endl;
                    print_input_request();
                    continue;
                }
                cout << endl << "User: " << user_id_input << endl;
                cout << "* user_rating\t* global_rating\t\t* count\t\t* title" << endl;
                cout << "vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv" << endl;
                UserData user_data = user_array[user_id_input];
                for (int unsigned i = user_data.review_start; i < user_data.review_end; i++) {
                    ReviewData review_data = reviews_array[i];
                    FilmData* movie = film_table.get_movie(review_data.movie_id);
                    cout << endl << "* " << review_data.rating << "\t\t* " << movie->global_rating << "\t\t* " << movie->total_ratings << "\t\t* " << movie->title << endl;

                }
                cout << endl << "^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^" << endl;
                cout << "* user_rating\t* global_rating\t\t* count\t\t* title" << endl << endl;
                print_input_request_successful();
                continue;
            }
            if (token == tags_command) {
                vector<string> tags_accumulator;
                string tags_string_buffer;
                getline(input_stream, tags_string_buffer);
                tags_string_buffer = trim(tags_string_buffer);
                size_t tag_start = tags_string_buffer.find('\'');
                if (tags_string_buffer.size() == 0 || tag_start == string::npos) {
                    cout << endl << "Failed to detect valid tags input." << endl;
                    cout << "Insert one or more tags enclosed in single quotation marks." << endl;
                    cout << "Example query: tags 'Brazil' 'drugs'" << endl;
                    cout << "Remember to escape special characters such as \' (single quote) and \\ (escape character) if you want to use them inside the tag." << endl;
                    cout << "Example escaped query: tags '...\\'Livin\\' In An Amish Paradise\\''" << endl << endl;
                    print_input_request();
                    continue;
                }
                bool malformed_query = false;
                do {
                    size_t tag_end = tags_string_buffer.find('\'', tag_start + 1);
                    if (tag_end == string::npos) {
                        malformed_query = true;
                        break;
                    }
                    for (;;) {
                        char previous_char = tags_string_buffer[tag_end - 1];
                        if (previous_char != '\\') { // Check if the single-quote is escaped.
                            break; // Not escaped, carry on.
                        }
                        unsigned int steps_backwards = 2;
                        previous_char = tags_string_buffer[tag_end - steps_backwards];
                        while (previous_char == '\\') { // Check if the escape symbol is escaped.
                            steps_backwards++; // It's turtles all the way down.
                            previous_char = tags_string_buffer[tag_end - steps_backwards];
                        }
                        if (steps_backwards % 2 != 0) {
                            break; // Escape symbol was actually escaped, carry on.
                        }
                        tag_end = tags_string_buffer.find('\'', tag_end + 1);
                        if (tag_end == string::npos) {
                            malformed_query = true;
                            break;
                        }
                    }
                    if (malformed_query) {
                        break;
                    }
                    string tag(tags_string_buffer.begin() + tag_start + 1, tags_string_buffer.begin() + tag_end);
                    tag = sanitize_tag(tag);
                    tags_accumulator.push_back(tag);
                    tag_start = tags_string_buffer.find('\'', tag_end + 1); // Go to next tag.
                } while (tag_start != string::npos);
                if (malformed_query) {
                    cout << endl << "Malformed query." << endl;
                    cout << "Please enclose each tag in single quotation marks." << endl;
                    cout << "Example query: tags 'Brazil' 'drugs'" << endl;
                    cout << "Remember to escape special characters such as \' (single quote) and \\ (escape character) if you want to use them inside the tag." << endl;
                    cout << "Example escaped query: tags '...\\'Livin\\' In An Amish Paradise\\''" << endl << endl;
                    print_input_request();
                    continue;
                }
                if (tags_accumulator.size() == 0) {
                    cout << endl << "Failed to detect valid tags input." << endl;
                    cout << "Insert one or more tags enclosed in single quotation marks." << endl;
                    cout << "Example query: tags 'Brazil' 'drugs'" << endl;
                    cout << "Remember to escape special characters such as \' (single quote) and \\ (escape character) if you want to use them inside the tag." << endl;
                    cout << "Example escaped query: tags '...\\'Livin\\' In An Amish Paradise\\''" << endl << endl;
                    print_input_request();
                    continue;
                }
                vector<unsigned int> movie_ids_found = tag_table.tag_intersection(tags_accumulator);
                if (movie_ids_found.size() == 0) {
                    cout << endl << "No movies match input label. Try another." << endl << endl;
                    print_input_request_successful();
                    continue;
                }
                vector<FilmData*> movie_references;
                for (unsigned int movie_id : movie_ids_found) {
                    FilmData* movie_data = film_table.get_movie(movie_id);
                    bool inserted = false;
                    for (unsigned int i = 0; i < movie_references.size(); i++) {
                        FilmData* other_movie = movie_references[i];
                        if (other_movie->global_rating >= movie_data->global_rating) {
                            continue;
                        }
                        movie_references.insert(movie_references.begin() + i, movie_data);
                        inserted = true;
                        break;
                    }
                    if (inserted) {
                        continue;
                    }
                    movie_references.push_back(movie_data);
                }
                cout << endl << "* rating\t* count\t* title\t\t* genres" << endl;
                cout << "vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv" << endl << endl;
                for (FilmData* movie_data : movie_references) {
                    cout << endl << "* " << movie_data->global_rating << "\t* " << movie_data->total_ratings << "\t* " << movie_data->title << "\t* ";
                    size_t genre_len = movie_data->genres.size();
                    if (genre_len == 0) {
                        cout << "No Genres Listed";
                    }
                    else {
                        genre_len--; // Take the last out to avoid a trailing |
                        for (unsigned int i = 0; i < genre_len; i++) {
                            cout << "" << movie_data->genres[i] << '|';
                        }
                        cout << "" << movie_data->genres[genre_len];
                    }
                    cout << endl;
                }
                cout << endl << "^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^" << endl;
                cout << "* rating\t* count\t* title\t\t* genres" << endl << endl;
                print_input_request_successful();
                continue;
            }
            if (token == help_command) {
                if (getline(input_stream, token)) {
                    token = trim(token);
                }
                else {
                    token.clear(); // To allow differentiating "help" from "help help"
                }
                if (token.size() > 0) {
                    if (token == topics_help_command) {
                        cout << endl << "Valid extra topics: " << topics_help_command << ", " << genres_help_command << endl;
                        print_input_request_successful();
                        continue;
                    }
                    if (token == genres_help_command) {
                        const vector<string> valid_genres = genre_table.get_all_genre_names();
                        cout << endl << "Valid genres:";
                        if (valid_genres.size() > 0) {
                            cout << " " << valid_genres[0];
                        }
                        for (unsigned int i = 1; i < valid_genres.size(); i++) {
                            cout << ", " << valid_genres[i];
                        }
                        cout << endl;
                        print_input_request_successful();
                        continue;
                    }
                    if (token == help_command) {
                        cout << endl << "Commands are not case-sensitive." << endl;
                        cout << "You can either input \"help\" alone as a command, or with an aditional topic for extra information." << endl;
                        cout << "Example of valid command input: help topics" << endl;
                        cout << "Help recursion ends here." << endl << endl;
                        print_input_request_successful();
                        continue;
                    }
                    if (token == profile_command) {
                        cout << endl << "Repeats the profiling information gathered." << endl;
                        cout << "That is, the time it took the program to build the data structures and be ready for taking queries." << endl;
                        cout << "Example of valid command input: " << profile_command << endl << endl;
                        print_input_request_successful();
                        continue;
                    }
                    if (token == movie_command) {
                        cout << endl << "Returns a list of movies that match the given prefix." << endl;
                        cout << "The longer the prefix, the more precise the matching, theoretically." << endl;
                        cout << "Example of valid command input: " << movie_command << " Nix" << endl << endl;
                        print_input_request_successful();
                        continue;
                    }
                    if (token == user_command) {
                        cout << endl << "Returns a list of reviews submitted by the user of the given id." << endl;
                        cout << "Example of valid command input: " << user_command << " 1234" << endl << endl;
                        print_input_request_successful();
                        continue;
                    }
                    if (token == tags_command) {
                        cout << endl << "Returns a list of movies that share all the given tags." << endl;
                        cout << "Two special characters must be escaped with the \"\\\" escape symbol." << endl;
                        cout << "The first is the \"'\" single quotation mark, used to separate the different tags." << endl;
                        cout << "The other is the escape symbol itself." << endl;
                        cout << "Example of valid command input: " << tags_command << " \'incredibly stupid \"plot\"\'" << endl << endl;
                        print_input_request_successful();
                        continue;
                    }
                    if (token == top_command) {
                        cout << endl << "Returns a list of highest ranked movies of the given genre." << endl;
                        cout << "Only movies with at least 1000 reviews are ranked." << endl;
                        cout << "The command is made of two parts." << endl;
                        cout << "First part is top<N>, where N is an integer number. Example: \"top99\"" << endl;
                        cout << "The second part is a valid movie genre among the available." << endl;
                        cout << "The list of the valid genres can be found by using the \"help " << genres_help_command << "\" command." << endl;
                        cout << "Genres are not case-sensitive." << endl;
                        cout << "Example of valid command input: " << top_command << "50 war" << endl << endl;
                        print_input_request_successful();
                        continue;
                    }
                    if (token == exit_command) {
                        cout << endl << "The exit command can be used at any time to finish the program." << endl;
                        cout << "Example of valid command input: " << exit_command << endl << endl;
                        print_input_request_successful();
                        continue;
                    }
                    cout << endl << "Failed to detect valid help topic." << endl;
                    cout << "Please double-check the spelling of the second command, if you inserted what you expected to be valid input." << endl;
                    cout << "Use \"help\" (without the quotes or anything else) for valid commands, or \"exit\" to leave the program." << endl << endl;
                    print_input_request();
                    continue;
                }
                cout << endl << "Valid commands: " << profile_command << ", " << movie_command << ", " << user_command;
                cout << ", " << tags_command << ", " << top_command << ", " << help_command << ", " << exit_command << endl << endl;
                cout << "Input a help plus a command to learn more about it, or input \"help topics\" to view aditional topics" << endl;
                cout << "Examples of valid help inputs (you can copypaste them):" << endl;
                cout << "help topics" << endl;
                cout << "help movie" << endl << endl;
                print_input_request_successful();
                continue;
            }
            if (token == profile_command) {
                cout << endl << "Movie initialization time: " << chrono::duration_cast<milli>(finish_movie - start_movie).count() << " milliseconds" << endl;
                cout << "Rating initialization time: " << chrono::duration_cast<milli>(finish_rating - start_rating).count() << " milliseconds" << endl;
                cout << "Genre initialization time: " << chrono::duration_cast<milli>(finish_genre - start_genre).count() << " milliseconds" << endl;
                cout << "Tag initialization time: " << chrono::duration_cast<milli>(finish_tag - start_tag).count() << " milliseconds" << endl;
                cout << "Total initialization time: " << chrono::duration_cast<milli>(finish_main - start_main).count() << " milliseconds" << endl << endl;
                print_input_request_successful();
                continue;
            }
            if (token == exit_command) {
                cout << endl << "Ending program..." << endl;
                break;
            }
            string token_first_part(token.begin(), token.begin() + 3);
            if (token_first_part == top_command) {
                istringstream token_second_part_stream(string(token.begin() + 3, token.end()));
                unsigned int top_number;
                if (!(token_second_part_stream >> top_number) || top_number < 1 || top_number > NUMBER_OF_MOVIES) {
                    cout << endl << "Malformed top command." << endl;
                    cout << "Use top<N> where N is a number from 1 to " << NUMBER_OF_MOVIES << ", followed by a valid movie genre enclosed in single quotation marks." << endl;
                    cout << "Example query: top10 'action'" << endl;
                    print_input_request();
                    continue;
                }
                if (!getline(input_stream, token)) {
                    cout << endl << "Failed to detect valid genre input for top command." << endl;
                    cout << "Use top<N> where N is a number from 1 to " << NUMBER_OF_MOVIES << ", followed by a valid movie genre enclosed in single quotation marks." << endl;
                    cout << "Example query: top10 'action'" << endl;
                    print_input_request();
                    continue;
                }
                token = trim(token);
                if ((token.size() < 3) || (token[0] != '\'') || (token[token.size() - 1] != '\'')) {
                    cout << endl << "Failed to detect valid genre input for top command." << endl;
                    cout << "Use top<N> where N is a number from 1 to " << NUMBER_OF_MOVIES << ", followed by a valid movie genre enclosed in single quotation marks." << endl;
                    cout << "Example query: top10 'action'" << endl;
                    print_input_request();
                    continue;
                }
                token = string(token.begin() + 1, token.end() - 1); // Remove quotation marks.
                GenreData* genre_data = genre_table.get_genre_data(token);
                if (genre_data == NULL) {
                    cout << endl << "Failed to find requested genre. Please make sure it was written correctly. The command is not case sensitive." << endl;
                    cout << "Use top<N> where N is a number from 1 to " << NUMBER_OF_MOVIES << ", followed by a valid movie genre enclosed in single quotation marks." << endl;
                    cout << "Example query: top10 'action'" << endl;
                    cout << "Use \"help genres\" in order to list the valid genres to be used here." << endl;
                    print_input_request();
                    continue;
                }
                unsigned int counter = 1;
                cout << endl << "* rating\t* count\t* title\t\t* genres" << endl;
                cout << "vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv" << endl << endl;
                for (FilmData* movie_data : genre_data->ordered_table) {
                    cout << endl << "* " << movie_data->global_rating << "\t* " << movie_data->total_ratings << "\t* " << movie_data->title << "\t* ";
                    size_t genre_len = movie_data->genres.size();
                    if (genre_len == 0) {
                        cout << "No Genres Listed";
                    }
                    else {
                        genre_len--; // Take the last out to avoid a trailing |
                        for (unsigned int i = 0; i < genre_len; i++) {
                            cout << "" << movie_data->genres[i] << '|';
                        }
                        cout << "" << movie_data->genres[genre_len];
                    }
                    cout << endl;
                    if (++counter > top_number) {
                        break;
                    }
                }
                cout << endl << "^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^" << endl;
                cout << "* rating\t* count\t* title\t\t* genres" << endl << endl;
                print_input_request_successful();
                continue;
            }
            cout << endl << "Failed to detect valid input." << endl;
            cout << "Please double-check the spelling of the first command, if you inserted what you expected to be valid input." << endl;
            cout << "Use \"help\" (without the quotes) for valid commands, or \"exit\" to leave the program." << endl;
            print_input_request();
        }
    }

    cout << "Program finished" << endl;
}
