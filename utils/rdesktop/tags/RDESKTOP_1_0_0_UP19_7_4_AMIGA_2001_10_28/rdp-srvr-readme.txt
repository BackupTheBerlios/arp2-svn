Well, here it is.

rdp-srvr  started as a quick hack to debug rdesktop without the need
of a real server. It first just displayed a dull gray background and
a moving cursor and it printed keyboard and mouse events to the
standard output. It had so much code in common with rdesktop that I
added some additional routines between #ifdef SERVER and #endif blocks
instead of forking off separate source files. This made it also much
easyer to follow the regular patches for rdesktop.

After a while I was wondering if I could display something more
interesting than just a moving arrowhead. The fastest way to get
some result was adding the code of a VNC client.

FILES:
------
Makefile               modified to also generate rdp-server

iso.c                  \
mcs.c                   |  original files from rdesktop with some
proto.h                 |  modifications to also accept incoming
rdesktop.c              |  connections. All new code is between
rdp.c                   |  #ifdef SERVER
secure.c                |    and
tcp.c                   |  #endif /* SERVER */
xwin.c                 /     lines

d3des.c                \
d3des.h                 |  these files ar just stolen^Wborrowed from
vncauth.c               |  the VNC sources.
vncauth.h              /

rfb.c                  my implementation of a VNC client

vnc.c                  the low-level tcp-ip routines used by rfb.c
                       (mostly a copy of tcp.c)

TO MAKE rdp-srvr:
-----------------
make a fresh directory with the rdesktop sources and apply the
current patches (either 19-5-10beta or 19-6-6)
then rename the directory and apply the rdp-srvr patchfile.

     tar xvzf rdesktop-1.0.0.tar.gz
     bzcat rdesktop-unified-patch19-5-10beta.bz2 | patch -p0
     mv rdesktop-1.0.0 rdesktop-1.0.0-pl19-5-10beta
     zcat rdp-srvr-19-5-10.diff.gz | patch -p0
or
     tar xvzf rdesktop-1.0.0.tar.gz
     bzcat rdesktop-unified-patch19-6-6.bz2 | patch -p0
     mv rdesktop-1.0.0 rdesktop-1.0.0-pl19-6-6
     zcat rdp-srvr-19-6-6.diff.gz | patch -p0

How to run it:
--------------
start vncserver

if you normally can connect with:

  vncviewer localhost:2

with password: "geheim" type

  ./rdp-srvr -p geheim localhost:2

on an other xterm type e.g.:

  ./rdesktop localhost -g 400x300 -bpp 8

TO DO:
------
- document it.
- make it compile without tons of warnings
- replace lines commented out with // with some usefull code.
- implement other screendepths than 8 bpp
- close rdp link properly when rfb link is closed by the vncserver.
- debug the initial exchange of messages to support other clients
  than rdesktop.
- check it out on non-US-QWERTY keyboards.
- remove all endianess related bugs
- improve security, at this moment a coredump will reveal the password
- make it faster !!!!!
  Well if you consider the number of protocol translations you do
  between the user and the application it will be clear that this
  solution is not a speed champion.

    O      __      |~~~~~|       |~~~~~|       |~~~~~|      |~~~~~|
   /|\  __/ | X11  rdesktop  RDP rdp-srvr  VNC |Xvnc | X11  Xclient
   / \  `---'------|_____|-------|_____|-------|_____|------|_____|

- Make a real server for the RDP protocol by combining it with an X11
  server. Just as Xvnc is a translation between the X11 and the RFB
  protocol, make an Xrdp as a translation between X11 and RDP.

    O      __                    |~~~~~|                    |~~~~~|
   /|\  __/ |        RDP         |Xrdp |        X11         Xclient
   / \  `---'--------------------|_____|--------------------|_____|
  
  Or make a "rdpdrv.o" that replaces the "x11drv.o" and "ttydrv.o"
  in Wine.

    O      __                                               |~~~~~|
   /|\  __/ |                      RDP                      Wine-TS
   / \  `---'-----------------------------------------------|_____|

Disclaimer:
-----------
- consider this code as pre-ALFA, the only warranty  I can give is
  that it compiles on my computer. This code is yet not ready to be
  used in a critical environment such as running a nuclear powerplant.


Note for Peter:
---------------
Since this email contains two binary attachments I didn't send it to
the mailinglist but directly to you (CC-ed to Kin Lau, who originally
asked for it.) Maybe you can tell the mailinglist when it is available
on your website.



I hope you can do something usefull with this code,

Mark.

