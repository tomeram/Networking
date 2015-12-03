Nim Online Game
=======

Todo list fot the next build

## Server

1. Refuse extra connections

2. Game
	a. Move logic to external file (include in header)

	b. Add gmae status - Waiting for clients
		Note: Check what to do if the fist client disconnects before second client connects.

	c. Messages:
		i. 		Regex to check the message prefix.
		ii. 	Add correct prefix to sent message.
		iii. 	Send the message.

	d. Game commands:
		i. 		Turn timeout. (On every cycle according to game state)
		ii.		Wrong command - The player looses his turn.
		iii.	Ignore commands out of turn + Document this behavior.
		iv. 	Delete computer player.
		v.		Fix winning message.

	e. Check how to handle quits and timeouts (what message to send to the other player).

3. Clean server shutdown.


## Client

1. Change to select architecture.

2. Message handling (incoming and outgoing).

3. Quit handling.