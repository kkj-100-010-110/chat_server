# chat_server

lang  : C<br/>
API   : EPOLL

build : make<br/>
run   : ./server

- How to reset the port number
	1. Create '.env' file in root.
	2. Write 'PORT=[number]'
- User & room lists are stored in RB tree(FYI, https://github.com/kkj-100-010-110/tree)
- To test using telnet. "telnet [IP] [PORT]"
- COMMAND
  - ID [your id] : Change your id, the default is "NO ID"
  - LIST : See the list of rooms
  - OPEN [room name] : Create a chat room
  - JOIN [room name] : Join a chat room
  - MSG [message] : Send a message to members where you join
  - MEMBER : See the list of members
  - LEAVE : Leave the room

You can also check the client program here, https://github.com/kkj-100-010-110/chat_client
