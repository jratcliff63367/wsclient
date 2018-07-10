# WSCLIENT

This project is a fork of easywsclient which is a simple websockets compatible client

The purpose of this fork is to add support for a server mode as well.

To build for windows you first need CMAKE installed on your machine.

Then run 'build_win64.bat' or 'generate_projects.bat' to create the Visual Studio Solution and project files

To build for Linux you need CMAKE installed on your machine.

Then run 'build_linux64.sh' or 'generate_projects.sh'

Note: To build this on Linux you need 'ncurses' installed.
If you do not already have 'ncurses' installed you will get a compile error

The magic linux comand to install 'ncurses' is:

sudo apt-get install libncurses5-dev

Ncurses is a library that supports terminal commands.

For this demo we want to support a simple chat server and need
keyboard/terminal input for that.
