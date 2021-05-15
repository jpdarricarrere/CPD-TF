#pragma once

void print_input_request() {
	cout << "Insert query, use the \"help\" command for instructions or insert \"exit\" to leave it." << endl << endl;
}

void print_input_request_successful() {
	cout << "Query sucessful. Insert next command." << endl << endl;
}

string ltrim(const string& s) {
	return regex_replace(s, regex("^\\s+"), "");
}

string rtrim(const string& s) {
	return regex_replace(s, regex("\\s+$"), "");
}

string trim(string& s) {
	return ltrim(rtrim(s));
}

string sanitize_tag(string& s) {
	return regex_replace(s, regex("([\\\\])([\\\\'])"), "$2"); // Regex: ([\\])([\\'])
}
