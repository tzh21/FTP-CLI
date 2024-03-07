## How to write a chat program with UDP

Every client should start two processes, one for sending messages and another for listening. 

To ensure the correctness of messages sequence, every message should be sent with a sequence number. When a listener receives a message, it should check if the sequence number is exactly the successor of the last message received. If it is, the listener should immediately show the message. If there are missing sequence numbers, the listener should temporaily store the arriving messages until the correct message arrives.

To identify the squence of messages from two sides, every message should be sent with a timestamp. Based on the timestamps, listener can show a chat history.
