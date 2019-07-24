# l4sdemo
Testbed and GUI to evaluate AQMs

This repository contains a set of tools to perform graphical evaluation of different AQMs on a 
testbed consisting of 5 nodes - 1 AQM node, 2 clients and 2 servers. This setup is a requirement for all the tools to work. 

The testbed can be provisionned using two scripts:
- setup_testbed.sh should be run on the AQM node
- setup_endhosts.sh will provision the clients and servers

Both scripts assume that some environment variables are set. You can override the defaults set in
environment.sh in a file named environment.local (which has to be executable).

Once the setup is complete, running run_demo.sh will start the GUI and let you measure the behavior of various
AQMs and congestion controls.

[More doc to come]
