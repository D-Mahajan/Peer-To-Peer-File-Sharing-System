#include "header_files.h"
#define MSG_SIZE 1000000
#define SERVER_QUEUE 10
using namespace std;

struct user_info_struct {
    string password, ip, port;
    bool logged_in;
    map<string, unordered_set<string>> group_file_map;
};

struct group_info_struct {
    string owner;
    unordered_set<string> peers;
    unordered_set<string> files;
    unordered_set<string> outstanding_requests;
};

map<string, user_info_struct> user_info_map;
map<string, group_info_struct> tracker_info_map;
map<string, map<string, map<string, user_info_struct>>> file_info_map;

string check_peer_message(string peer_msg_str, string current_user_id) {
    string res, command, peer_ip, peer_port;

    stringstream ss(peer_msg_str);
    getline(ss, peer_ip, ' ');
    getline(ss, peer_port, ' ');
    getline(ss, command, ' ');

    if(command == "create_user") {
        string user_id;
        getline(ss, user_id, ' ');

        //check if user exists
        if(user_info_map.find(user_id)!=user_info_map.end()) {
            res = "User already exists";
        } else {
            user_info_struct user;

            getline(ss, user.password, ' ');

            user.ip = peer_ip;
            user.port = peer_port;
            user.logged_in = false;
            user_info_map[user_id] = user;   
            res = "User created successfully";
        }
        current_user_id = user_id;
    } else if(command == "login") {
        string user_id, user_password;
        getline(ss, user_id, ' ');

        //check if user exists
        if(user_info_map.find(user_id)==user_info_map.end()) {
            res = "User does not exist";
        } else {
            getline(ss, user_password, ' ');

            if(user_info_map[user_id].password != user_password) {
                res = "Wrong password";
            } else if(user_info_map[user_id].logged_in == true) {
                res = "User already logged in";
            } else {
                user_info_map[user_id].logged_in = true;   
                res = "User logged in successfully";
            }
        }
        current_user_id = user_id;
    } else if(command == "logout") {
        if(user_info_map.find(current_user_id)==user_info_map.end()) {
            res = "User does not exist";
        } else {
            if(user_info_map[current_user_id].logged_in == false) {
                res = "User did not log in";
            } else {
                user_info_map[current_user_id].logged_in = false; 
                res = "User logged out successfully";
            }
        } 
        current_user_id = ""; 
    } else if(command == "exit") {
        if(current_user_id!="" && user_info_map.find(current_user_id)!=user_info_map.end()) {
            user_info_map[current_user_id].logged_in = false; 
        } 
        res = "Exiting";
        current_user_id = ""; 
    } else if(command == "create_group") {
        string group_id;
        getline(ss, group_id, ' ');
        
        if(tracker_info_map.find(group_id) != tracker_info_map.end()) {
            res = "Group already exists";
        } else {
            if(current_user_id=="" || user_info_map.find(current_user_id)==user_info_map.end() || user_info_map[current_user_id].logged_in == false) {
                res = "User not logged in";
            } else {
                tracker_info_map[group_id].owner = current_user_id;
                tracker_info_map[group_id].peers.insert(current_user_id);
                res = "Group created successfully";
            }
        }
    } else if(command == "join_group") {
        string group_id;
        getline(ss, group_id, ' ');
        
        if(tracker_info_map.find(group_id) == tracker_info_map.end()) {
            res = "Group does not exist";
        } else {
            if(current_user_id=="" || user_info_map.find(current_user_id)==user_info_map.end() || user_info_map[current_user_id].logged_in == false) {
                res = "User not logged in";
            } else if(tracker_info_map[group_id].owner == current_user_id) {
                res = "User is already owner of group, no need to join";
            } else if(find(tracker_info_map[group_id].outstanding_requests.begin(), tracker_info_map[group_id].outstanding_requests.end(), current_user_id)!=tracker_info_map[group_id].outstanding_requests.end()) {
                res = "Join request already created";
            }
            else {
                tracker_info_map[group_id].outstanding_requests.insert(current_user_id);
                res = "Join request created successfully";
            }
        }
    } else if(command == "leave_group") {
        string group_id;
        getline(ss, group_id, ' ');

        if(tracker_info_map.find(group_id) == tracker_info_map.end()) {
            res = "Group does not exist";
        } else {
            if(current_user_id=="" || user_info_map.find(current_user_id)==user_info_map.end() || user_info_map[current_user_id].logged_in == false) {
                res = "User not logged in";
            } else if(find(tracker_info_map[group_id].peers.begin(), tracker_info_map[group_id].peers.end(), current_user_id)==tracker_info_map[group_id].peers.end()) {
                //user not in this group
                res = "User not a member of this group";

                //check if this user has any outstanding join request
                if(find(tracker_info_map[group_id].outstanding_requests.begin(), tracker_info_map[group_id].outstanding_requests.end(), current_user_id)!=tracker_info_map[group_id].outstanding_requests.end()) {
                    tracker_info_map[group_id].outstanding_requests.erase(current_user_id);
                }
            } else if(tracker_info_map[group_id].owner == current_user_id && tracker_info_map[group_id].peers.size()==1) {

                //delete group
                tracker_info_map.erase(group_id);

                //delete entry from user_info_map
                user_info_map[current_user_id].group_file_map.erase(group_id);

                //delete entry from file_info_map               
                file_info_map.erase(group_id);

                res = "Leave group request complete, group deleted";
            } else if(tracker_info_map[group_id].owner == current_user_id) {
                for(string file_name: user_info_map[current_user_id].group_file_map[group_id]) {
                    if(file_info_map[group_id][file_name].size() > 1) {
                        file_info_map[group_id][file_name].erase(current_user_id);
                    } else {
                        tracker_info_map[group_id].files.erase(file_name);
                        file_info_map[group_id].erase(file_name);
                    }
                }
                user_info_map[current_user_id].group_file_map.erase(group_id);

                //make next peer in set as owner of group
                tracker_info_map[group_id].peers.erase(current_user_id);  
                tracker_info_map[group_id].owner = *tracker_info_map[group_id].peers.begin();                                                                                                                                                                                                                                                                                                                                                              
                res = "Leave group request complete";
            }
            else {             
                for(string file_name: user_info_map[current_user_id].group_file_map[group_id]) {
                    if(file_info_map[group_id][file_name].size() > 1) {
                        file_info_map[group_id][file_name].erase(current_user_id);
                    } else {
                        tracker_info_map[group_id].files.erase(file_name);
                        file_info_map[group_id].erase(file_name);
                    }   
                }
                user_info_map[current_user_id].group_file_map.erase(group_id);

                tracker_info_map[group_id].peers.erase(current_user_id);                                                                                        
                res = "Leave group request complete";
            }
        }
    } else if(command == "list_requests") {
        string group_id;
        getline(ss, group_id, ' ');

        if(tracker_info_map.find(group_id) == tracker_info_map.end()) {
            res = "Group does not exist";
        } else {
            if(current_user_id=="" || user_info_map.find(current_user_id)==user_info_map.end() || user_info_map[current_user_id].logged_in == false) {
                res = "User not logged in";
            } else {                                                         
                int i=0, len=tracker_info_map[group_id].outstanding_requests.size();
                if(!len) {
                    res = "No join requests made";
                } 
                for(string user: tracker_info_map[group_id].outstanding_requests) {
                    res+=user;
                    if(i!=(len-1)) {
                        res+="\n";
                    } 
                    ++i;
                }                  
            }
        }
    } else if(command == "accept_request") {
        string group_id, user_id;
        getline(ss, group_id, ' ');
        getline(ss, user_id, ' ');

        if(tracker_info_map.find(group_id) == tracker_info_map.end()) {
            res = "Group does not exist";
        } else {
            if(current_user_id=="" || user_info_map.find(current_user_id)==user_info_map.end() || user_info_map[current_user_id].logged_in == false) {
                res = "User not logged in";
            } else if(user_id=="" || user_info_map.find(user_id)==user_info_map.end()) {
                res = "Specified user does not exist";
            } else if(tracker_info_map[group_id].outstanding_requests.find(user_id)==tracker_info_map[group_id].outstanding_requests.end()) {
                res = "Specified user is not in request list for this group";
            } else if(tracker_info_map[group_id].owner != current_user_id) {
                res = "User is not owner of this group, cannot accept request";
            } else {
                tracker_info_map[group_id].outstanding_requests.erase(user_id);
                tracker_info_map[group_id].peers.insert(user_id);
                res = "Request accepted successfully";
            }
        }
    } else if(command == "list_groups") {
        if(current_user_id=="" || user_info_map.find(current_user_id)==user_info_map.end() || user_info_map[current_user_id].logged_in == false) {
            res = "User not logged in";
        } else {
            int i=0, len=tracker_info_map.size();
            if(tracker_info_map.size()==0) {
                res = "No groups";
            }
            for(auto it = tracker_info_map.begin(); it!=tracker_info_map.end(); ++it) {
                res += it->first;
                if(i!=(len-1)) {
                    res+= "\n";
                }
                ++i;
            }
        }        
    } else if(command == "upload_file") {
        if(current_user_id=="" || user_info_map.find(current_user_id)==user_info_map.end() || user_info_map[current_user_id].logged_in == false) {
            res = "User not logged in";
        } else {
            string temp_str, file_path, group_id, file_name;
            int last;

            getline(ss,temp_str);
            last = temp_str.find_last_of(' ');
            file_path = temp_str.substr(0, last);
            group_id = temp_str.substr(last+1);
            file_name = get_file_name(file_path);

            if(tracker_info_map.find(group_id) == tracker_info_map.end()) {
                res = "Group does not exist";
            } else if(find(tracker_info_map[group_id].peers.begin(), tracker_info_map[group_id].peers.end(), current_user_id)==tracker_info_map[group_id].peers.end()) {
                res = "User is not a member of specified group";                
            } else if(find(tracker_info_map[group_id].files.begin(), tracker_info_map[group_id].files.end(), file_name)!=tracker_info_map[group_id].files.end()) {
                res = "File already uploaded";
            } else {
                tracker_info_map[group_id].files.insert(file_name);
                user_info_map[current_user_id].group_file_map[group_id].insert(file_name);
                file_info_map[group_id][file_name][current_user_id] = user_info_map[current_user_id];
                res = "File uploaded successfully";
            }
        }
    } else if(command == "list_files") {
        string group_id;
        getline(ss, group_id, ' ');

        if(tracker_info_map.find(group_id) == tracker_info_map.end()) {
            res = "Group does not exist";
        } else if(current_user_id=="" || user_info_map.find(current_user_id)==user_info_map.end() || user_info_map[current_user_id].logged_in == false) {
            res = "User not logged in";
        } else if(find(tracker_info_map[group_id].peers.begin(), tracker_info_map[group_id].peers.end(), current_user_id)==tracker_info_map[group_id].peers.end()) {
            res = "User is not a member of specified group";                
        } else {
            int i=0, len=tracker_info_map[group_id].files.size();

            if(len == 0) {
                res = "No files being shared in this group";
            } else {
                for(auto it = tracker_info_map[group_id].files.begin(); it!=tracker_info_map[group_id].files.end(); ++it) {
                    res += *it;
                    if(i!=(len-1)) {
                        res+= "\n";
                    }
                    ++i;
                }
            }
        }
    } else if(command == "stop_share") {
        string group_id, file_name;

        getline(ss, group_id, ' ');
        getline(ss, file_name, ' ');

        if(tracker_info_map.find(group_id) == tracker_info_map.end()) {
            res = "Group does not exist";
        } else if(current_user_id=="" || user_info_map.find(current_user_id)==user_info_map.end() || user_info_map[current_user_id].logged_in == false) {
            res = "User not logged in";
        } else if(find(tracker_info_map[group_id].peers.begin(), tracker_info_map[group_id].peers.end(), current_user_id)==tracker_info_map[group_id].peers.end()) {
            res = "User is not a member of specified group";                
        } else if(find(tracker_info_map[group_id].files.begin(), tracker_info_map[group_id].files.end(), file_name)==tracker_info_map[group_id].files.end()) {
            res = "File specified has not been uploaded";
        } else if(user_info_map[current_user_id].group_file_map.find(group_id)==user_info_map[current_user_id].group_file_map.end() || find(user_info_map[current_user_id].group_file_map[group_id].begin(), user_info_map[current_user_id].group_file_map[group_id].end(), file_name)==user_info_map[current_user_id].group_file_map[group_id].end()) {
            res = "User has not uploaded this file";
        } else {
            if(file_info_map[group_id][file_name].size() > 1) {
                file_info_map[group_id][file_name].erase(current_user_id);
            } else {
                file_info_map[group_id].erase(file_name);
                tracker_info_map[group_id].files.erase(file_name);
            }
            user_info_map[current_user_id].group_file_map[group_id].erase(file_name);
            
            //delete entry in user info if no files are being shared
            if(user_info_map[current_user_id].group_file_map[group_id].size() == 0) {
                user_info_map[current_user_id].group_file_map.erase(group_id);
            }
            res = "File share stopped successfully";
        }
    } else if(command == "download_file") {
        string group_id, file_name, dest_path;

        getline(ss, group_id, ' ');
        getline(ss, file_name, ' ');
        getline(ss, dest_path, ' ');

        if(tracker_info_map.find(group_id) == tracker_info_map.end()) {
            res = "Group does not exist";
        } else if(current_user_id=="" || user_info_map.find(current_user_id)==user_info_map.end() || user_info_map[current_user_id].logged_in == false) {
            res = "User not logged in";
        } else if(find(tracker_info_map[group_id].peers.begin(), tracker_info_map[group_id].peers.end(), current_user_id)==tracker_info_map[group_id].peers.end()) {
            res = "User is not a member of specified group";                
        } else if(find(tracker_info_map[group_id].files.begin(), tracker_info_map[group_id].files.end(), file_name)==tracker_info_map[group_id].files.end()) {
            res = "File specified has not been uploaded";
        } else {
            res = "Peer_list_compiled_successfully";
            for(auto it = file_info_map[group_id][file_name].begin(); it!=file_info_map[group_id][file_name].end(); ++it) {
                res += " "+it->first+":"+it->second.ip+":"+it->second.port;
            }
        }
    } else {
        res = "Command not found";
    }
    return res;
}                                                                                                                                                                       

void* read_from_peer(void* arg) {
    int peer_sd = *((int *)arg);
    string peer_msg_str, tracker_msg_str, current_user_id="", temp_str;
    char peer_msg[MSG_SIZE] = {0}, *tracker_msg = new char[MSG_SIZE]; 
    bool res;
    while(1) {
        memset(peer_msg, 0, sizeof(peer_msg));
        read(peer_sd, peer_msg, MSG_SIZE);
        peer_msg_str = peer_msg;
        if(peer_msg_str.find("exit")==string::npos) {
            //check if command is create_user, then store current user id
            temp_str = peer_msg_str;
            stringstream ss(temp_str);
            getline(ss, temp_str, ' ');
            getline(ss, temp_str, ' ');
            getline(ss, temp_str, ' ');
            if(temp_str=="create_user" || temp_str == "login") {
                getline(ss, temp_str, ' ');
                current_user_id = temp_str;
            } 
        }
        tracker_msg_str = check_peer_message(peer_msg_str, current_user_id);
        cout<<peer_msg<<endl;
        
        if(peer_msg_str.find("exit")!=string::npos) {
            break;
        }

        cout<<"Sending message to socket descriptor "<<peer_sd<<endl;
        strcpy(tracker_msg, tracker_msg_str.c_str());
        send(peer_sd, tracker_msg, strlen(tracker_msg), 0 ); 
    }
    close(peer_sd);
    delete [] tracker_msg;
    return NULL;
}

int main(int argc, char const *argv[]) 
{ 
    struct sockaddr_in address; 
    int server_sd, peer_sd, opt = 1, addrlen; 
    string ip, port, file_str;
    char *end;
    pthread_t newthread;
    fstream file;
    addrlen  = sizeof(address); 

    if(argc!=3) {
        cout<<"Wrong number of arguments entered"<<endl;
        exit(0);
    }

    file.open(argv[1],ios::in);
    if(file.is_open()) {
        getline(file, file_str);
        ip = file_str;
        getline(file, file_str);
        port = file_str;
        file.close();
    } else {
        cout<<"Could not open file"<<endl;
        exit(0);
    }

    cout<<"Tracker "<<argv[2]<<" IP: "<<ip<<endl<<"Tracker "<<argv[2]<<" Port: "<<port<<endl;

    if((server_sd = socket(AF_INET, SOCK_STREAM, 0)) == 0) { 
        cout<<"Socket failed"<<endl; 
        exit(0); 
    } 

    if(setsockopt(server_sd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) { 
        cout<<"Setsockopt failed"<<endl; 
        exit(0); 
    } 
    address.sin_family = AF_INET; 
    address.sin_addr.s_addr = inet_addr(ip.c_str());
    address.sin_port = htons((uint16_t)strtol(port.c_str(), &end, 10)); 
       
    if(bind(server_sd, (struct sockaddr *)&address, sizeof(address))<0) { 
        cout<<"Bind failed"<<endl; 
        exit(0); 
    } 
    if(listen(server_sd, SERVER_QUEUE) < 0) { 
        cout<<"Listen failed"<<endl; 
        exit(0); 
    } 

    while(1) {
        if((peer_sd = accept(server_sd, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0) { 
            cout<<"Accept failed"<<endl; 
            exit(0); 
        } 
        pthread_create(&newthread, NULL, read_from_peer, (void *)&peer_sd);
    }
    
    close(server_sd);
    return 0; 
} 