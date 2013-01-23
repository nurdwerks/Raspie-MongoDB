Raspie-MongoDB
==============
MongoDB shell version: 2.1.1-pre-

Install the dependent packages 
------------------------------
    sudo apt-get install git-core build-essential scons libpcre++-dev xulrunner-dev libboost-dev libboost-program-options-dev libboost-thread-dev libboost-filesystem-dev

Clone / Compile / Install
------------
    
    # Get a clone and go the the cloned folder
    git clone https://github.com/DjustinK/Raspie-MongoDB
    cd Raspie-MongoDB
    
    # Compiling will take several hours, take a nap
    scons
    
    # Take another nap while installing
    sudo scons --prefix=/opt/mongo install
    
    # This will install mongo in /opt/mongo, to get other programs to see it, you can add this dir to your $PATH
    PATH=$PATH:/opt/mongo/bin/
    export PATH
    
Create the service script to start a database
--------------------------------------------------------

Starting & testing MongoDB
--------------------------

To run a single server database:

    mkdir /data/db
    ./mongod start
    # The mongo javascript shell connects to localhost and test database by default:
    ./mongo 
