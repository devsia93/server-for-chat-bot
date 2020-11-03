#include <iostream>
#include <string>
#include <regex>
#include <unordered_map>
#include "algorithm"
using namespace std;

string to_lower(string input){
    std::transform(input.begin(), input.end(), input.begin(),
                   [](unsigned char c){ return std::tolower(c); });
    return input;
}

void bot(string text){
    cout << "[BOT]: " << text << endl;
}

string user(){
    string question;
    cout << "[USER]: ";
    getline(cin, question);
    question = to_lower(question);
    return question;
}

int main() {
    unordered_map<string, string> database = {
            {"hello", "Oh, hello, human!"},
            {"how are you", "I am fine, thanks!"},
            {"what are you doing", "Answering some stupid questions..."},
            {"what is your name", "My name is Ivan."}
    };
    string question;

    bot("Hello user, I can answer your questions!");

    while(question.compare("exit") != 0) {
        int count_found_phrases = 0;
        question = user();

        for(auto entry : database){
            auto pattern_phrase = regex(".*"+entry.first+".*");
            if(regex_match(question, pattern_phrase))
                bot(entry.second);
            ++count_found_phrases;
        }
        if (count_found_phrases == 0){
            bot("I don't understand your question. Sorry:(");
        }
    }
    bot("Ok, bye!");
    return 0;
}
