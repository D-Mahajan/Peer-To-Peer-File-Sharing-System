# Peer to Peer Group Based File Sharing System
* The File Sharing System allows users to share, download files from the group they belong to. Download is parallel with multiple pieces from multiple peers.
* A tracker program maintains client information with their files to assist the clients for the communication between peers.

## Working
1. Client needs to create an account in order to be part of the network.
2. Client can create any number of groups (with different group IDs) and hence will be owner of those groups.
3. Client needs to be part of the group from which it wants to download the file.
4. Client will send join request to join a group.
5. Owner Client Will Accept/Reject the request.
6. After joining group ,client can see list of all the files in the group.
7. Client can send the download command to tracker with the group name and filename and tracker will send the details of the group members which are currently
sharing that particular file
8. After fetching the peer info from the tracker, client will communicate with peers about the portions of the file they contain and hence accordingly decide which part of data to take from which peer (Piece Selection Algorithm).
9. After logout, the client should temporarily stop sharing the currently shared files till the next login.

## Tracker Commands
1. Run Tracker: `./tracker tracker_info.txt tracker_no tracker_info.txt` <br>
    `tracker_info.txt` Contains ip, port details of all the trackers.
    
## Client Commands    
1. Run Client: `./client <IP>:<PORT> tracker_info.txt` <br>
    `tracker_info.txt` Contains ip, port details of all the trackers.
2. Create User Account: `create_user <user_id> <passwd>`
3. Login: `login <user_id> <passwd>`
4. Create Group: `create_group <group_id>`
5. Join Group: `join_group <group_id>`
6. Leave Group: `leave_group <group_id>`
7. List pending join: `requests list_requests <group_id>`
8. Accept Group Joining Request: `accept_request <group_id> <user_id>`
9. List All Group In Network: `list_groups`
10. List All sharable Files In Group: `list_files <group_id>`
11. Upload File: `upload_file <file_path> <group_id>`
12. Download File: `download_file <group_id> <file_name> <destination_path>`
13. Logout: `logout`
14. Stop sharing: `stop_share <group_id> <file_name>`
15. Exit: `exit`