# camio1.0
[DEPRECATED] The first version of the CamIO library

This library is deprecated an no longer maintained. It is kept only for the QJump project backwards compatibility (https://github.com/camsas/qjump-camio-tools).

To build it, you must have the "cake" build system installed. You can obtain cake from https://github.com/Zomojo/Cake

Use the cake-3.0.856-1 version of the "cake" build system (https://github.com/Zomojo/compiletools/releases/tag/cake-3.0.856-1). Other versions might work as well but are not tested.

To download and install cake version 3.0.856-1: *sudo may be required when copying files*
  1. `wget https://github.com/Zomojo/compiletools/archive/cake-3.0.856-1.tar.gz`
  2. `tar -xvzf cake-3.0.856-1.tar.gz`
  3. `cd compiletools-cake-3.0.856-1`
  4. Copy the appropriate configuration file based on your distribution. For Ubuntu 14.04: `cp etc.cake.ubuntu.14.04 /etc/cake.conf`
  5. `cp cake /usr/bin/cake`

*Similar instructions can be found in the "INSTALL" file in the "cake" project directory, but they are slightly changed here.*

To build camio, run "build.sh" in the root directory.
