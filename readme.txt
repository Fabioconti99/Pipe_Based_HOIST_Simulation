################## ADVANCED ROBOT PROGRAMMING, Assignment 1 ##################

### RUNNING THE SYSTEM ###

To run the system you should first launch the INSTALL shell script running the following command on the command window:

$ source ./install.sh <path_name>

$ ./run.sh

### INTERACTION WITH THE SYMULATION ###

COMMAND KONSOLE:

To interact with the symulation you can use the following KEYS:

- Motion on the Z-Axis:
	- [up arrow] key to move the HOIST upwards.
	- [down arrow] key to move the HOIST downwards. 
	- [z] key to stop the motion on the Z-Axis.

- Motion on the X-Axis:
	- [left arrow] key to move the HOIST towards the LEFT direction.
	- [right arrow] key to move the HOIST the RIGHT direction. 
	- [x] key to stop the motion on the X-Axis.
	
- Exiting the simulation: 
	- [e] key. 
	
INSPECTION KONSOLE:

To interact with the simulation you can use the following KEYS:

- RESET of the system:
	- [r] key.

- STOP of the system:
	- [s] key.

### INTRODUCTION TO THE PROJECT ###

The code developed for this project is capable of testing and deploying an interactive simulation of an HOIST with 2 d.o.f., in which two different consoles allow the user to activate the hoist.

The project is composed by six processes:

1)master.cpp:
 A "master" process that creates all the fifo channels for communication and launches all the other processes. 

2)command.cpp:
 A "command" process that manages the movement of the HOIST position through the use of two different motors on for each shifting direction. To control the activation and the shifting direction of the motors the arrow keys are used. Two other buttons are used as brakes one for each of the motors.
This process along with the "inspection process" is run through the use of a separate console from the main shell. 

3-4)motor_x(_y):
 The two motors are simulated through two separate processes that receive commands from both the "inspection process" and the "command process" and send feedback information about their position to the inspector process. Every time the robot moves a little error will be added to the acquired distance to simulate the real behaviour of an actual motor.

5) inspector.cpp:
The previously mentioned "Inspection process" keeps track of the position of the HOIST printing it on screen 5 times a second. It also has the capability of RESETTING (bringing the HOIST to the initial position) and STOPPING (stopping the HOIST at the current position) the whole system through the use of two additional EMERGENCY buttons. This process also includes a graphical simulation of an actual Hoist moving according to the data recorded. 

6) watchdog.cpp:
The "Watchdog process" has the capability of RESETTING the entire system if no order interaction is recorded from the command and inspection processes within a 60 seconds time window. 

This project can be run using the "install.sh" and "run.sh" bash scripts included in the project folder. 



