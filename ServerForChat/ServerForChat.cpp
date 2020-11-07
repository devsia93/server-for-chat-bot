#include <iostream>
#include <string>
#include <unordered_map>
#include <algorithm>
#include <thread>
#include <uwebsockets/App.h>
using namespace std;


template <class K, class V, class Compare = std::less<K>, class Allocator = std::allocator<std::pair<const K, V>>>
class syncronized_map {
private:
	map <K, V, Compare, Allocator> _map;
	mutex	_mutex;
public:
	void set(K key, V value) {
		lock_guard<mutex> lg(this->_mutex);
		this->_map[key] = value;
	}

	V& get(K key) {
		std::lock_guard<mutex> lg(this->_mutex);
		return this->_map[key];
	}

	vector<V> get_all_values() {
		lock_guard<mutex> lg(this->_mutex);
		vector<auto> result;
		for (auto entry : this->_map) {
			result.push_back(entry.second);
		}
		return result;
	}

	vector<V> get_all_keys() {
		lock_guard<mutex> lg(this->_mutex);
		vector<auto> result;
		for (auto entry : this->_map) {
			result.push_back(entry.first);
		}
		return result;
	}

	int get_size() {
		lock_guard<mutex> lg(this->_mutex);
		return _map.size();
	}
};

syncronized_map<unsigned long, string> user_names;

string to_lower(string input) {
	std::transform(input.begin(), input.end(), input.begin(),
		[](unsigned char c) { return std::tolower(c); });
	return input;
}

void bot(string text) {
	cout << "[BOT]: " << text << endl;
}

string user() {
	string question;
	cout << "[USER]: ";
	getline(cin, question);
	question = to_lower(question);
	return question;
}

struct user_connection {
	unsigned long user_id;
	string user_name;
};

void server_start() {
	atomic_ulong latest_user_id = 0;
	vector<thread*> threads(thread::hardware_concurrency());

	transform(threads.begin(), threads.end(), threads.begin(), [&latest_user_id](thread* thr) {
		return new thread([&latest_user_id]() {
			uWS::App().ws<user_connection>("/*", {
				//actions
				.open = [&latest_user_id](auto* ws) {
					//some code when new connection available
					user_connection* data = (user_connection*)ws->getUserData();
					data->user_id = latest_user_id++;
					cout << "New user connected, id = " << data->user_id << endl;

					ws->subscribe("broadcast");//common channel
					ws->subscribe("user#" + to_string(data->user_id));//personal channel

					data->user_name = "Unnamed";
					user_names.set(data->user_id, data->user_name);

					//send existing users
					for (int i = 0; i < user_names.get_size(); ++i) {
						cout << user_names.get(i);
						string result = "";
						//("id: " + to_string(i) + " name: " + user_names.get(i))
						result.append("id: ").append(to_string(i)).append(", name: ").append(user_names.get(i)).append(";\n");
						ws->send(result, uWS::OpCode::TEXT, false);
					}
				},

				.message = [](auto* ws, string_view message, uWS::OpCode opCode) {
					//some code when new message available
					//example: USER_NAME=SomeName
					string_view sub_message = message.substr(0, 10);
					user_connection* data = (user_connection*)ws->getUserData();

					if (sub_message.compare("USER_NAME=") == 0) {
						string_view name = message.substr(10);
						data->user_name = string(name);
						cout << "New user. Name: " << string(name) << ", ID: " << to_string(data->user_id) << endl;
						//example: NEW_USER,SomeName,id
						string broadcast_message = "NEW_USER," + data->user_name + "," + to_string(data->user_id);
						ws->publish("broadcast", string_view(broadcast_message), opCode, false);
						user_names.set(data->user_id, data->user_name);
					}

					//example: MESSAGE_TO,id_which_send_message,text_message
					sub_message = message.substr(0, 11);
						if (sub_message.compare("MESSAGE_TO,") == 0) {
							string_view rest = message.substr(11);
							int comma_pos = rest.find(",");
							string_view id_string_to = rest.substr(0, comma_pos);
							string_view user_message = rest.substr(comma_pos + 1);
							string header = "[" + string(data->user_name) + "(" + to_string(data->user_id) + ")]: ";//[SomeName(id)]:
							ws->publish("user#" + string(id_string_to), string_view(header + string(user_message)), opCode, false);
							cout << "New private message from id = " << data->user_id << " to  id = "
								<< id_string_to << ". Message text: " << user_message << endl;
						}

						//example: PUBLIC_MESSAGE,text_message
						sub_message = message.substr(0, 15);
						if (sub_message.compare("PUBLIC_MESSAGE,") == 0) {
							string_view pub_message = message.substr(15);
							string header = "(public)[" + string(data->user_name) + "(" + to_string(data->user_id) + ")]: ";//(public)[SomeName(id)]:
							ws->publish("broadcast", string_view(header + string(pub_message)), opCode, false);
							cout << "New public message from id = " << data->user_id << ". Message text: " << string(pub_message) << endl;
						}
				}

				})
				.listen(8000, [](auto* token) {
					if (token) {
						cout << "Server successfull started.\n";
					}
					else cerr << "Faill started server.\n";
					}).run();
			});
		});

	for_each(threads.begin(), threads.end(), [](thread* thr) {
		thr->join();
		});
}

int main() {

	server_start();
	return 0;
}
