Nim Online Game
=======

Todo list fot the next build

## Server

1. ~~Refuse extra connections~~

2. Game
	
    1. ~~Move logic to external file (include in header)~~
    2. ~~Add game status - Waiting for clients~~ __Note: Check what to do if the fist client disconnects before second client connects__
	3. ~~Messages:~~
		* ~~Regex to check the message prefix.~~
		* ~~Add correct prefix to sent message.~~
		* ~~Send the message.~~
	4. Game commands:
		* Turn timeout. (On every cycle according to game state)
		* ~~Wrong command - The player looses his turn.~~
		* ~~Ignore commands out of turn~~ + Document this behavior.
		* ~~Delete computer player~~
		* ~~Fix winning message~~
	5. Check how to handle quits and timeouts (what message to send to the other player).
3. Clean server shutdown.

## Client

1. Change to select architecture.

2. Message handling (incoming and outgoing).

3. Quit handling.