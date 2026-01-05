#include "KVStore.h"
#include <iostream>
#include <string>
#include <sstream>
using namespace std;

int main() {
    cout << "Supported Operation: PUT | GET | REMOVE | EXIT" << endl;

    KVStore kv;
    string line;

    while (getline(cin, line)) {
        if (line.empty()) continue;

        string cmd, key, val;

        stringstream ss(line);
        ss >> cmd;

        if (cmd == "EXIT") {
            cout << "Exiting KV Store" << endl;
            break;
        }

        ss >> key;
        if (key.empty()) {
            cout << "Error: Missing key" << endl;
            continue;
        }

        if (cmd == "PUT") {
            getline(ss, val);
            if (!val.empty() && val[0] == ' ')
                val.erase(0, 1);

            if (kv.put(key, val))
                cout << "INSERTED" << endl;
            else
                cout << "INVALID INPUT" << endl;

        } else if (cmd == "GET") {
            auto result = kv.get(key);
            if (result)
                cout << *result << endl;
            else
                cout << "NOT FOUND" << endl;

        } else if (cmd == "REMOVE") {
            if (kv.remove(key))
                cout << "REMOVED" << endl;
            else
                cout << "KEY NOT FOUND" << endl;

        } else {
            cout << "INVALID COMMAND" << endl;
        }
    }

    return 0;
}
