#include <iostream>
#include <unistd.h> 
#include <stdio.h> 
#include <arpa/inet.h> 
#include <sys/socket.h> 
#include <stdlib.h> 
#include <netinet/in.h> 
#include <string>
#include <string.h> 
#include <pthread.h>
#include <fstream>
#include <sstream>
#include <map>
#include <vector>
#include <unordered_set>
#include <algorithm>
#include <sys/stat.h>
#include <cmath>
using namespace std;

string get_file_name(string file_path) {
    if(file_path[file_path.length()-1]=='/') {
        file_path.resize(file_path.length()-1);
    }
    return file_path.substr(file_path.find_last_of('/')+1);
}

string set_file_path(string path, string file_name) {
    if(path[path.length()-1]!='/') {
        path += '/';
    }
    path += file_name;
    return path;
}

bool validate_file_path(string file_path) {
    struct stat buffer;   
    return (stat(file_path.c_str(), &buffer) == 0);
}