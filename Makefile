CFLAGS=-Wall -Wextra -std=c99 -g

08: 08.practical.work.server.nonblock

07: 07.practical.work.server.turn.delim.close 07.practical.work.client.turn.delim.close

05: 05.practical.work.server.turn 05.practical.work.client.turn

04.practical.work.client.setup: 04.practical.work.client.setup.c
	$(CC) $(CFLAGS) $+ -o $@

03.practical.work.server.setup: 03.practical.work.server.setup.c
	$(CC) $(CFLAGS) $+ -o $@

02.practical.work.gethostbyname: 02.practical.work.gethostbyname.c
	$(CC) $(CFLAGS) $+ -o $@
