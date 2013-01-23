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
    
    # Create a data folder for the database and give the pi user rights
    mkdir /data/db
    sudo chown pi /data/db
    
    # Create the log folder
    cd /var/log
    mkdir mongodb
    chmod 777 mongodb 
    
    
Create the service script to start a database
--------------------------------------------------------
    # Copy the file mongodb to the /etc/init.d folder
    cp Raspie-MongoDB/mongodb /etc/init.d/mongodb
    
    # Make init script executable
    sudo chmod +x /etc/init.d/mongodb
    
    # set up runtime links
    sudo update-rc.d mongodb defaults

Starting & testing MongoDB
--------------------------

To run a single server database:
    
    # Start the service (start, stop, reload)
    sudo /etc/init.d/MongoDB start
    
    # To test the database connection
    cd /opt/mongo/bin/
    ./mongo 
