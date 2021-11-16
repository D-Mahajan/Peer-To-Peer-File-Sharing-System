#include "header_files.h"
#define MSG_SIZE 1000000
#define SERVER_QUEUE 10
#define CHUNK_SIZE 5120
using namespace std;

struct connection {
    string ip, port;
};

struct file_info {
    string file_path;
    long long file_size;
    string chunk_vector;
};

map<string, map<string, map<string, file_info>>> peer_file_map;

string get_file_size(string file_path) {
    struct stat st;
    stat(file_path.c_str(), &st);
    return to_string(st.st_size);
}

long long get_number_of_chunks(string file_size) {
    long long size = stoll(file_size);
    return ceil(size/(double)(CHUNK_SIZE));
}

void* read_from_peer(void* arg) { 
    int peer_sd = *((int *)arg), file_read_size = CHUNK_SIZE;
    char peer_msg[MSG_SIZE] = {0}, *tracker_msg = new char[MSG_SIZE]; 
    string user_id, group_id, file_name, temp, tracker_msg_str;

    memset(peer_msg, 0, sizeof(peer_msg));
    read(peer_sd, peer_msg, MSG_SIZE);
        
    temp = peer_msg;
    stringstream ss(temp);
    getline(ss, temp, ' ');
            
    if(temp == "get_bit_map") {
        getline(ss, user_id, ' ');
        getline(ss, group_id, ' ');
        getline(ss, file_name, ' ');
        tracker_msg_str = peer_file_map[user_id][group_id][file_name].chunk_vector;
    } else if(temp == "download_chunk") {
        FILE *file;
        string chunk_number_str, file_path;
        long long chunk_number, file_size; 
        //download_size = CHUNK_SIZE;
        char download_buffer[CHUNK_SIZE];

        getline(ss, user_id, ' ');
        getline(ss, group_id, ' ');
        getline(ss, file_name, ' ');

        tracker_msg_str = to_string(peer_file_map[user_id][group_id][file_name].file_size);
        memset(tracker_msg, 0, sizeof(tracker_msg));
        strcpy(tracker_msg, tracker_msg_str.c_str());
        send(peer_sd, tracker_msg, strlen(tracker_msg), 0 ); 

        while(1) {

            memset(peer_msg, 0, sizeof(peer_msg));
            read(peer_sd, peer_msg, MSG_SIZE);
            temp = peer_msg;
            stringstream ss(temp);
            getline(ss, temp, ' ');

            if(temp == "download_finished") {
                tracker_msg_str = "Acknowledgement for download finished";
                fclose(file);
                break;
            }

            getline(ss, user_id, ' ');
            getline(ss, group_id, ' ');
            getline(ss, file_name, ' ');
            getline(ss, chunk_number_str, ' ');
            chunk_number = stoll(chunk_number_str);

            // get file size, download size check if its the last chunk
            file_size = peer_file_map[user_id][group_id][file_name].file_size;
            file_path = peer_file_map[user_id][group_id][file_name].file_path;
            if(chunk_number == (peer_file_map[user_id][group_id][file_name].chunk_vector.size()-1)) {
                file_read_size = file_size - (chunk_number * CHUNK_SIZE);
            }
            file = fopen(file_path.c_str(), "r");
            fseek(file, chunk_number*CHUNK_SIZE, SEEK_SET);
            memset(download_buffer, 0, sizeof(download_buffer));
            fread(download_buffer, sizeof(char), file_read_size, file);
            tracker_msg_str = download_buffer;

            cout<<"Sending message to socket descriptor "<<peer_sd<<endl;
            memset(tracker_msg, 0, sizeof(tracker_msg));
            strcpy(tracker_msg, tracker_msg_str.c_str());
            send(peer_sd, tracker_msg, strlen(tracker_msg), 0 ); 
        }
    } else if(temp == "get_size") {
        getline(ss, user_id, ' ');
        getline(ss, group_id, ' ');
        getline(ss, file_name, ' ');

        tracker_msg_str = to_string(peer_file_map[user_id][group_id][file_name].file_size);
    } else {
        tracker_msg_str = "Ping from server";
    }
        
    cout<<"Sending message to socket descriptor "<<peer_sd<<endl;
    memset(tracker_msg, 0, sizeof(tracker_msg));
    strcpy(tracker_msg, tracker_msg_str.c_str());
    send(peer_sd, tracker_msg, strlen(tracker_msg), 0 ); 

    close(peer_sd);
    delete [] tracker_msg;
    return NULL;
}

void * server(void *arg) {
    struct sockaddr_in address; 
    int server_sd, peer_sd, opt = 1, addrlen; 
    string ip = (*((connection *)arg)).ip, port = (*((connection *)arg)).port;
    char *end, peer_msg[MSG_SIZE] = {0}, *tracker_msg = new char[MSG_SIZE]; 
    pthread_t newthread;
    addrlen  = sizeof(address); 

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
    
    delete [] tracker_msg;
    close(server_sd);
    return NULL;
}

void * get_bit_vector(void * arg) {
    struct sockaddr_in address; 
    char *peer_msg = new char[100], *received_msg = new char[MSG_SIZE], *end; 
    int peer_sd = 0; 
    string peer_msg_str, temp=*((string *)arg), connection_ip, connection_port, user_id, group_id, file_name, peer_id;
    
    stringstream ss(temp);
    getline(ss, user_id, ' ');
    getline(ss, group_id, ' ');
    getline(ss, file_name, ' ');
    getline(ss, peer_id, ' ');
    getline(ss, connection_ip, ' ');
    getline(ss, connection_port, ' ');

    if((peer_sd = socket(AF_INET, SOCK_STREAM, 0)) < 0) { 
        cout<<"Socket creation error"<<endl; 
        exit(0); 
    } 
    address.sin_family = AF_INET; 
    address.sin_port = htons((uint16_t)strtol(connection_port.c_str(), &end, 10)); 

    if(inet_pton(AF_INET, connection_ip.c_str(), &address.sin_addr)<=0) { 
        cout<<"Invalid address/address not supported"<<endl; 
        exit(0);
    } 
   
    if(connect(peer_sd, (struct sockaddr *)&address, sizeof(address)) < 0) { 
        cout<<"Connection Failed"<<endl; 
        exit(0);
    } 
    peer_msg_str = "get_bit_map "+peer_id+" "+group_id+" "+file_name;
    strcpy(peer_msg, peer_msg_str.c_str());
    send(peer_sd, peer_msg, strlen(peer_msg), 0);

    memset(received_msg, 0, sizeof(received_msg));
    read(peer_sd, received_msg, MSG_SIZE);
    delete[] peer_msg;
    close(peer_sd);
    pthread_exit(received_msg);
}

map<string, vector<int>> piece_selection_algorithm(map<string, string> chunk_map) {
    map<string, vector<int>> result;
    int number_of_peers = chunk_map.size(), number_of_chunks = ((chunk_map.begin())->second).size(), j=0;

    auto it = chunk_map.begin();
    for(int i=0; i<number_of_chunks; ++i) {
        j=0;
        while((it->second)[i]!='1' && j<number_of_peers) {
            ++it;
            if(it==chunk_map.end()) {
                it = chunk_map.begin();
            }
            ++j;
        }
        if((it->second)[i]=='1') {
            result[it->first].push_back(i);
        }
        ++it;
        if(it==chunk_map.end()) {
            it = chunk_map.begin();
        }
    }

    return result;
}

void * download_chunks(void *arg) {
    struct sockaddr_in address; 
    char *peer_msg = new char[100], *received_msg = new char[MSG_SIZE], *end; 
    int peer_sd = 0; 
    string peer_msg_str, temp=*((string *)arg), connection_ip, connection_port, user_id, group_id, file_name, peer_id, file_path, path;
    vector<int> chunk_numbers;
    long long file_size, chunk_len;
    int i;
    FILE *file;

    stringstream ss(temp);
    getline(ss, temp, ' ');
    stringstream ss2(temp);
    getline(ss2, user_id, ':');
    getline(ss2, group_id, ':');
    getline(ss2, file_name, ':');
    getline(ss2, peer_id, ':');
    getline(ss2, connection_ip, ':');
    getline(ss2, connection_port, ':');
    getline(ss2, file_path, ':');
    while(ss>>temp) {
        chunk_numbers.push_back(stoll(temp));
    }

    path = set_file_path(file_path, file_name);
    peer_file_map[user_id][group_id][file_name].file_path = path;
    peer_file_map[user_id][group_id][file_name].file_size = 0;

    if((peer_sd = socket(AF_INET, SOCK_STREAM, 0)) < 0) { 
        cout<<"Socket creation error"<<endl; 
        exit(0); 
    } 
    address.sin_family = AF_INET; 
    address.sin_port = htons((uint16_t)strtol(connection_port.c_str(), &end, 10)); 

    if(inet_pton(AF_INET, connection_ip.c_str(), &address.sin_addr)<=0) { 
        cout<<"Invalid address/address not supported"<<endl; 
        exit(0);
    } 
   
    if(connect(peer_sd, (struct sockaddr *)&address, sizeof(address)) < 0) { 
        cout<<"Connection Failed"<<endl; 
        exit(0);
    } 

    peer_msg_str = "download_chunk "+peer_id+" "+group_id+" "+file_name;
    memset(peer_msg, 0, sizeof(peer_msg));
    strcpy(peer_msg, peer_msg_str.c_str());
    send(peer_sd, peer_msg, strlen(peer_msg), 0);

    memset(received_msg, 0, sizeof(received_msg));
    read(peer_sd, received_msg, MSG_SIZE);
    file_size = stoll(received_msg);

    peer_file_map[user_id][group_id][file_name].file_size = file_size;
    i=0;
    chunk_len = peer_file_map[peer_id][group_id][file_name].chunk_vector.length();
    for(long long chunk_number: chunk_numbers) {
        string received_msg_str, download_size_str;
        int buffer_size = CHUNK_SIZE;

        if(chunk_number==(chunk_len-1)) {
            buffer_size = file_size - (chunk_number * CHUNK_SIZE);
        }
        
        char download_buffer[buffer_size];
        download_buffer[buffer_size] = 0;

        peer_msg_str = "download_chunk "+peer_id+" "+group_id+" "+file_name+" "+to_string(chunk_number);
        memset(peer_msg, 0, sizeof(peer_msg));
        strcpy(peer_msg, peer_msg_str.c_str());
        send(peer_sd, peer_msg, strlen(peer_msg), 0);

        memset(download_buffer, 0, sizeof(download_buffer));
        read(peer_sd, download_buffer, MSG_SIZE);
        
        file = fopen(path.c_str(), "r+");
        fseek(file, chunk_number*CHUNK_SIZE, SEEK_SET);
        fwrite(download_buffer, sizeof(char), strlen(download_buffer), file);
        fclose(file);
        ++i;
    }

    peer_msg_str = "download_finished";
    memset(peer_msg, 0, sizeof(peer_msg));
    strcpy(peer_msg, peer_msg_str.c_str());
    send(peer_sd, peer_msg, strlen(peer_msg), 0);

    memset(received_msg, 0, sizeof(received_msg));
    read(peer_sd, received_msg, MSG_SIZE);
    
    close(peer_sd);
    delete[] peer_msg;
    return NULL;
}

void * get_size(void * arg) {
    struct sockaddr_in address; 
    char *peer_msg = new char[100], *received_msg = new char[MSG_SIZE], *end; 
    int peer_sd = 0; 
    string peer_msg_str, temp=*((string *)arg), connection_ip, connection_port, user_id, group_id, file_name, peer_id, file_path, path;
    FILE *file;

    stringstream ss(temp);
    getline(ss, group_id, ' ');
    getline(ss, file_name, ' ');
    getline(ss, peer_id, ' ');
    getline(ss, connection_ip, ' ');
    getline(ss, connection_port, ' ');

    if((peer_sd = socket(AF_INET, SOCK_STREAM, 0)) < 0) { 
        cout<<"Socket creation error"<<endl; 
        exit(0); 
    } 
    address.sin_family = AF_INET; 
    address.sin_port = htons((uint16_t)strtol(connection_port.c_str(), &end, 10)); 

    if(inet_pton(AF_INET, connection_ip.c_str(), &address.sin_addr)<=0) { 
        cout<<"Invalid address/address not supported"<<endl; 
        exit(0);
    } 
   
    if(connect(peer_sd, (struct sockaddr *)&address, sizeof(address)) < 0) { 
        cout<<"Connection Failed"<<endl; 
        exit(0);
    } 

    peer_msg_str = "get_size "+peer_id+" "+group_id+" "+file_name;
    memset(peer_msg, 0, sizeof(peer_msg));
    strcpy(peer_msg, peer_msg_str.c_str());
    send(peer_sd, peer_msg, strlen(peer_msg), 0);

    memset(received_msg, 0, sizeof(received_msg));
    read(peer_sd, received_msg, MSG_SIZE);
    
    close(peer_sd);
    delete[] peer_msg;
    pthread_exit(received_msg);
}

void * upload_file(void *arg) {
    struct sockaddr_in address; 
    char *peer_msg = new char[MSG_SIZE], tracker_msg[MSG_SIZE] = {0}, *end; 
    int peer_sd = 0; 
    string peer_msg_str, temp=*((string *)arg), tracker_ip, tracker_port, group_id, file_path;

    cout<<"In upload thread: "<<temp<<endl;

    stringstream ss(temp);
    getline(ss, tracker_ip, ' ');
    getline(ss, tracker_port, ' ');
    getline(ss, file_path, ' ');
    getline(ss, group_id, ' ');

    if((peer_sd = socket(AF_INET, SOCK_STREAM, 0)) < 0) { 
        cout<<"Socket creation error"<<endl; 
        exit(0); 
    } 
    address.sin_family = AF_INET; 
    address.sin_port = htons((uint16_t)strtol(tracker_port.c_str(), &end, 10)); 

    if(inet_pton(AF_INET, tracker_ip.c_str(), &address.sin_addr)<=0) { 
        cout<<"Invalid address/address not supported"<<endl; 
        exit(0);
    } 
   
    if(connect(peer_sd, (struct sockaddr *)&address, sizeof(address)) < 0) { 
        cout<<"Connection Failed"<<endl; 
        exit(0);
    } 
    peer_msg_str = "upload_file "+file_path+" "+group_id;
    strcpy(peer_msg, peer_msg_str.c_str());
    send(peer_sd, peer_msg, strlen(peer_msg), 0);

    memset(tracker_msg, 0, sizeof(tracker_msg));
    read(peer_sd, tracker_msg, MSG_SIZE);
    cout<<tracker_msg<<endl; 

    delete[] peer_msg;
    close(peer_sd);
    return NULL;
}

void * download(void * arg) {
    map<string, vector<int>> peer_chunk_map;
    map<string, string> chunk_map; 
    vector<string> connections;
    string temp, ip, port, str = *((string *)arg), user_id, group_id, file_name, peer_id, file_path, tracker_ip, tracker_port;
    stringstream ss(str);

    cout<<"before download: "<<str<<endl;
    getline(ss, user_id, ' ');
    getline(ss, group_id, ' ');
    getline(ss, file_name, ' ');
    getline(ss, file_path, ' ');
    getline(ss, temp, ' ');

    while(ss>>temp) {
        stringstream ss2(temp);
        getline(ss2, peer_id, ':');
        getline(ss2, ip, ':');
        getline(ss2, port, ':');
        connections.push_back(peer_id+":"+ip+":"+port);
    }

    vector<pthread_t> peer_threads(connections.size());
    char *bit_map[connections.size()];
    vector<string> temp2(connections.size());

    // loop for creating thread for each value in chunk_map
    for(int i=0; i<connections.size(); ++i) {
        stringstream ss2(connections[i]);
        getline(ss2, peer_id, ':');
        getline(ss2, ip, ':');
        getline(ss2, port, ':');
        temp2[i] = user_id+" "+group_id+" "+file_name+" "+peer_id+" "+ip+" "+port;
        pthread_create(&peer_threads[i], NULL, get_bit_vector, &temp2[i]);
    }
    for(int i=0; i<connections.size(); ++i) {
        pthread_join(peer_threads[i], (void **)&bit_map[i]);
    }
    
    for(int i=0; i<connections.size(); ++i) {
        stringstream ss2(connections[i]);
        getline(ss2, peer_id, ':');
        getline(ss2, ip, ':');
        getline(ss2, port, ':');
        chunk_map[user_id+":"+group_id+":"+file_name+":"+peer_id+":"+ip+":"+port] = bit_map[i];
    }

    // call piece selection algo
    peer_chunk_map = piece_selection_algorithm(chunk_map);

    // get_file_size() => to write
    vector<string> peer_chunk_str(peer_chunk_map.size());
    int i=0;
    peer_threads.resize(peer_chunk_map.size());
    peer_threads.clear();

    string temp3, path = set_file_path(file_path, file_name);
    char *file_size;
    pthread_t pthread1;
    stringstream ss2(*(connections.begin()));
    getline(ss2, peer_id, ':');
    getline(ss2, ip, ':');
    getline(ss2, port, ':');
    temp3 = group_id+" "+file_name+" "+peer_id+" "+ip+" "+port;
    pthread_create(&pthread1, NULL, get_size, &temp3);
    pthread_join(pthread1, (void **)&file_size);

    FILE *file = fopen(path.c_str(), "w");
    ftruncate(fileno(file), atoll(file_size));
    fclose(file);

    // create connection with each peer in parallel and download particular chunk, pass groupid, filename too
    for(auto it=peer_chunk_map.begin(); it!=peer_chunk_map.end(); ++it) {
        peer_chunk_str[i] = it->first+":"+file_path+":$";
        for(auto it2=(it->second).begin(); it2!=(it->second).end(); ++it2) {
            peer_chunk_str[i] += " " + to_string(*it2);
        }
        pthread_create(&peer_threads[i], NULL, download_chunks, &peer_chunk_str[i]);
        ++i;
    }
    for(i=0; i<peer_chunk_map.size(); ++i) {
        pthread_join(peer_threads[i], NULL);
    }
    cout<<"Download finished"<<endl;

    long long number_of_chunks;
    number_of_chunks = get_number_of_chunks(file_size);
    peer_file_map[user_id][group_id][file_name].chunk_vector.resize(number_of_chunks, '1');

    return NULL;
}

void * client(void *arg) {
    struct sockaddr_in address; 
    char *peer_msg = new char[MSG_SIZE], tracker_msg[MSG_SIZE] = {0}, *end; 
    int peer_sd = 0; 
    string peer_msg_str, tracker_ip = (*((vector<connection> *)arg))[0].ip, tracker_port = (*((vector<connection> *)arg))[0].port, peer_ip = (*((vector<connection> *)arg))[1].ip, peer_port = (*((vector<connection> *)arg))[1].port, temp_str, user_id, group_id, file_path, file_size, file_name, download_msg;
    connection peer_connection;
    pthread_t peer_thread;
    long long number_of_chunks;
    bool exit_flag = false;

    string base_msg = peer_ip + " " + peer_port + " ";

    if((peer_sd = socket(AF_INET, SOCK_STREAM, 0)) < 0) { 
        cout<<"Socket creation error"<<endl; 
        exit(0); 
    } 
    address.sin_family = AF_INET; 
    address.sin_port = htons((uint16_t)strtol(tracker_port.c_str(), &end, 10)); 

    if(inet_pton(AF_INET, tracker_ip.c_str(), &address.sin_addr)<=0) { 
        cout<<"Invalid address/address not supported"<<endl; 
        exit(0);
    } 
   
    if(connect(peer_sd, (struct sockaddr *)&address, sizeof(address)) < 0) { 
        cout<<"Connection Failed"<<endl; 
        exit(0);
    } 
    while(1) {
        getline(cin, peer_msg_str);

        temp_str = peer_msg_str;
        stringstream ss(temp_str);
        getline(ss, temp_str, ' ');
        if(temp_str == "create_user") {
            getline(ss, user_id, ' ');
        } else if(temp_str == "login") {
            getline(ss, user_id, ' ');
        } else if(temp_str == "upload_file") {
            int last;

            getline(ss,temp_str);
            last = temp_str.find_last_of(' ');
            file_path = temp_str.substr(0, last);
            group_id = temp_str.substr(last+1);

            //get file size
            file_size = get_file_size(file_path);
            
            //get file name from path
            file_name = get_file_name(file_path);
            
            //get number of chunks
            number_of_chunks = get_number_of_chunks(file_size);

            if(!validate_file_path(file_path)) {
                cout<<"Invalid file path"<<endl;
                continue;
            }
        } else if(temp_str == "leave_group") {
            getline(ss, group_id, ' ');
        } else if(temp_str == "stop_share") {
            getline(ss, group_id, ' ');
            getline(ss, file_name, ' ');
        } else if(temp_str == "download_file") {
            getline(ss, group_id, ' ');
            getline(ss, file_name, ' ');
            getline(ss, file_path, ' ');

            if(!validate_file_path(file_path)) {
                cout<<"Invalid file path"<<endl;
                continue;
            }
        }

        if(peer_msg_str=="exit") {
            exit_flag = true;
        } 
        // adding base msg
        peer_msg_str = base_msg + peer_msg_str;

        strcpy(peer_msg, peer_msg_str.c_str());
        send(peer_sd, peer_msg, strlen(peer_msg), 0);
        if(exit_flag) {
            exit(0);
        }

        memset(tracker_msg, 0, sizeof(tracker_msg));
        read(peer_sd, tracker_msg, MSG_SIZE);
        cout<<tracker_msg<<endl; 

        //for leave_group, delete from peer structure
        temp_str = tracker_msg;
        if(temp_str.find("Leave group request complete")!=string::npos) {
            //delete entity from peer structure
            peer_file_map[user_id].erase(group_id);
        } else if(temp_str.find("File uploaded successfully")!=string::npos) {
            peer_file_map[user_id][group_id][file_name].file_size = stoll(file_size);
            peer_file_map[user_id][group_id][file_name].file_path = file_path;

            //set chunk vector
            peer_file_map[user_id][group_id][file_name].chunk_vector.resize(number_of_chunks, '1');
            cout<<"uploaded and set bit vector to: "<<peer_file_map[user_id][group_id][file_name].chunk_vector<<endl;
        } else if(temp_str.find("File share stopped successfully")!=string::npos) {
            peer_file_map[user_id][group_id].erase(file_name);
        } else if(temp_str.find("Peer_list_compiled_successfully")!=string::npos) {
            download_msg = user_id+" "+group_id+" "+file_name+" "+file_path+" "+temp_str;
            pthread_create(&peer_thread, NULL, download, &download_msg);
        }

        exit_flag=false;
    } 
    delete[] peer_msg;
    close(peer_sd);

    return NULL;
}

int main(int argc, char const *argv[]) 
{ 
    string file_str, tracker_ip, tracker_port, peer_ip, peer_port;
    connection tracker_connection, peer_connection;
    fstream newfile;
    pthread_t server_thread, client_thread;

    if(argc!=3) {
        cout<<"Wrong number of arguments entered"<<endl;
        exit(0);
    }

    newfile.open(argv[2],ios::in);
    if(newfile.is_open()) {
        getline(newfile, file_str);
        tracker_ip = file_str;
        getline(newfile, file_str);
        tracker_port = file_str;
        newfile.close();
    } else {
        cout<<"Could not open file"<<endl;
        exit(0);
    }
    cout<<"Tracker IP: "<<tracker_ip<<endl<<"Tracker Port: "<<tracker_port<<endl;
    tracker_connection.ip = tracker_ip;
    tracker_connection.port = tracker_port;

    stringstream ss(argv[1]);
    getline(ss, peer_ip, ':');
    getline(ss, peer_port, ':');

    cout<<"Peer IP: "<<peer_ip<<endl<<"Peer Port: "<<peer_port<<endl;
    peer_connection.ip = peer_ip;
    peer_connection.port = peer_port;

    vector<connection> tracker_peer_info;
    tracker_peer_info.push_back(tracker_connection);
    tracker_peer_info.push_back(peer_connection);

    pthread_create(&server_thread, NULL, server, &peer_connection);
    pthread_create(&client_thread, NULL, client, &tracker_peer_info);
    pthread_join(server_thread, NULL);
    pthread_join(client_thread, NULL);
    return 0; 
} 