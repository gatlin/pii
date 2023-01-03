pii
===

© 2023- Gatlin Johnson <gatlin@niltag.net>

***ThIs is INCOMPLETE SOFTWARE.***
I have used it as a Bonjour chat client as a proof of concept;
I will humbly and gratefully accept any merge requests.

Immediate deficiencies:

- [ ] Make sure this works with protocols besides bonjour
- [ ] More specifically, create a convenient way to simply refer to existing
purple accounts (probably easy, one thing at a time you know?)
- [ ] track down cause of occasional hanging file descriptor glib somehow loses
track of.

***

Multi-protocol messenger which exposes conversations as files: an input pipe
`in` and an output log `out`.
Interactions with the client itself are also exposed in this manner;
commands are documented below.

This client is inspired by [ii][ii].
Protocol support is provided by [libpurple][libpurple].

synopsis
===

Review the config file.
```console
$ cat pii.conf
[pii]
workspace=/home/username/pii
libpurpledata=/home/username/.local/etc/piiaccount
bonjour=true
```

Start pii with the explicit config file.

```console
$ ls /home/username | grep pii
$ pii -c pii.conf &
$ ls -la /home/username | grep pii
drwx------   3 username username    4096 Jan  2 18:02 pii
$ ls -la /home/username/pii/
total 32
drwx------   3 username username    4096 Jan  2 18:02 .
drwx------ 111 username username   20480 Jan  2 18:09 ..
prwx------   1 username username       0 Jan  2 18:04 in
-rw-rw-r--   1 username username     502 Jan  2 18:04 out
```

Ask the server for a list of buddies.

```console
$ echo "ls" >/home/username/pii/in
$ cat /home/username/pii/out
1672704720 Begin buddies
1672704720 other@remote
1672704720 End buddies
```

Start a conversation with a buddy, creating a new directory for it.

```console
$ echo "msg other@remote" >/home/username/pii/in
$ ls -la /home/username/pii/
total 36
drwx------   3 username username    4096 Jan  2 18:02 .
drwx------ 111 username username   20480 Jan  2 18:09 ..
drwx------   2 username username    4096 Jan  2 18:13 other@remote
prwx------   1 username username       0 Jan  2 18:04 in
-rw-rw-r--   1 username username     649 Jan  2 18:04 out
$ ls -la /home/username/pii/other@remote
total 12
drwx------   3 username username    4096 Jan  2 18:13 .
drwx------ 111 username username    4096 Jan  2 18:13 ..
prwx------   1 username username       0 Jan  2 18:04 in
-rw-rw-r--   1 username username      13 Jan  2 18:04 out
```

Send a message to `other@remote`.

```console
$ echo "hey" >/home/username/pii/other@username/in
```

Log the ongoing conversation with `other@remote`.

```console
$ tail -f /home/username/pii/other@username/out
1672708134 pii hey
1672708139 Other Person suh
```

commands
===

The client itself operates as a very simple chat bot which accepts a narrow
set of **commands**.

This list is the bare minimum to be interesting and more will be supported
with any luck.

## `ls`

Lists all buddies associated with the account.

## `add <buddy-name>`

Add a buddy to the main buddy list.

## `rm <buddy-name>`

Removes the named buddy from the buddy list.

## `msg <buddy-name>`

Start a conversation with the named buddy.

# See also

[lchat](https://tools.suckless.org/lchat/), a basic console client for this
type of chat client.

[xii](https://github.com/younix/xii), a basic X11-based client for this type of
chat client.

[jj](https://github.com/aaronNGi/jj), an XMPP client which works similar to
[ii][ii] and pii.


[libpurple]: https://pidgin.im
[ii]: https://tools.suckless.org/ii/
