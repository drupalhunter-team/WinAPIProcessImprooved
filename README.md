WinAPIProcessImprooved
======================

#ProcessManager v1.2

Main class is ProcessManager.
Class can get either PID of running process or command line arguments to start and monitor process

##Building
Windows 8 is required. Project uses the C++11 and was made in Visual Studio 13.

##Brief explanation
ProcessManager class gets either command line arguments or the PID of the process as a parameter in it's constructors. You can also pass to the constructor callback functions for such events: starting, stopping, crashing and restarting.
If you pass the PID, class retrieves the process command line arguments (if it can) and use them to restart it later.
If you pass the command line arguments, class will use them to start the process. All events are logged in a file.

© 2014 Pavlo Liasota All Rights Reserved
