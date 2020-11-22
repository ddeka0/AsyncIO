# AsyncIO
A CPP wrapper for a asychronous socket server. This code uses Jens Axboe's library "liburing".
Please check this talk at https://www.youtube.com/watch?v=-5T4Cjw46ys

#### This project is under development.

### IO Uring Diagram (image source : https://cor3ntin.github.io/posts/iouring/)
![io_uring](https://cor3ntin.github.io/posts/iouring/uring.svg)

##### Installation procedure for liburing library

    1. Download the latest Release from https://github.com/axboe/liburing/releases
       For example:
       wget https://github.com/axboe/liburing/archive/liburing-0.5.tar.gz
    2. tar -xvzf liburing-0.5.tar.gz
    3. cd liburing-0.5
    4. ./configure [please check the default location of installation using ./configure -h]
    5. make
    6. sudo make install    (or sudo checkinstall, helpfull for removing make install packages)

##### Sample Test

After cloning this repository, please create a dummy file to run the sample test program:
(`dd if=/dev/zero of=1GB.txt count=16 bs=64M`)
(please change the cmake target cpp files for your required test)
    
    1. cmake .
    2. make
    3. ./build/bin/liburing-test 1GB.txt
    
##### Socket Server Test
    1. cmake .
    2. make
    3. ./build/bin/liburing-test  (server is running)
    
    4. g++-10 -std=c++20 udp_client.cpp && ./a.out
    5. g++-10 -std=c++20 tcp_client.cpp && ./a.out
![TCP_UDP_server_running](https://github.com/ddeka0/io_uring_cpp/blob/master/demo-x.gif)
    
#### TODO for socket server features:
    Currently Socket server only supports UDP socket server
    1. Add TCP,SCTP support // partially done
    2. Add timer support // partially done
    3. Add a task system to parallelize the processing of the client requests.
    4. Co-Routine Support from C++20. 

#### Examples:

##### Socket Server App:
```
int main() {
    AsyncServer server;
    server.setServerConfig(SOCK_TCP | SOCK_UDP);
    
    auto entryPoint = [](AsyncServer *_server,void* buf,int len,client_info* _client) {
        std::string x((char*)(buf),len);
		std::cout <<"Client data : " << x <<" and client info :"
            <<_client->ip +":"+ _client->port<< std::endl;
        char str[] = "Hello from Server!";
        _server->Send(str,strlen(str) + 1,_client);
    };
    
    server.registerHandler(entryPoint);

    server.Run();
}
```

##### Timer App:

```
int main() {
    AsyncServer server;
    auto t100 = server.createNewTimer("T100",2);
    t100->repeatIndHandler = []() {
        std::cout <<"Retry again inside repeatInd handler" << std::endl;
    };
    t100->expiryIndHandler =[]() {
        std::cout <<"t100 has expired" << std::endl;
    };
    t100->setRepeatCount(3);
    server.startTimer(t100);
    server.Run();
}
```
![TimerApp Demo](https://github.com/ddeka0/io_uring_cpp/blob/master/timerApp-demo.gif)

#### For few Test cases, your linux kernel might need to be updated.
    Follow the steps to update your kernel (replace kernel version with latest once)
    1. sudo apt-get install fakeroot build-essential ncurses-dev xz-utils libssl-dev bc
    2. sudo apt-get install libncurses-dev flex bison libssl-dev wget
    3. wget https://cdn.kernel.org/pub/linux/kernel/v5.x/linux-5.6.2.tar.xz
    4. tar -xvf linux-5.6.2.tar.xz
    5. cd linux-5.6.2
    6. cp /boot/config-$(uname -r) .config
    7. make menuconfig   (same & exit)
    8. make -j 16    (replace with your total logical cores)
    9. sudo make modules_install -j 16
    10. sudo make install -j 16
    11. sudo update-initramfs -c -k 5.6.2
    12. sudo update-grub
    13. sudo reboot
    14. If "invalid signature" error appears in the boot process, then turn off
    secure boot option in the bios. And Restart
    15. Check new kernel with uname -r

Although gcc-10 is not required for this sample file. I am planning to createa a wrapper for Asychronous Socket Server Utilities using lastest C++ version. With gcc-10 we can use C++'s latest threading support as well as many cool features.

If you dont want to use g++-10, you can change the compiler manually in the `CMakeLists.txt` file.

If you want to use gcc-10, then please check:
https://github.com/ddeka0/cppLearn/tree/master/cpp20


##### Future Targets:
    
Before discussing the feature of this io uring based cpp networking library, let us discuss some of the existing library for asychronous cpp library.

###### libevent:

The libevent API provides a mechanism to execute a callback function when a specific event occurs on a file descriptor or after a timeout has been reached.
Furthermore, libevent also support callbacks due to signals or regular timeouts.

###### evpp:
1. Modern C++11 interface
2. Modern functional/bind style callback instead of C-style function pointer.
3. Multi-core friendly and thread-safe
4. A nonblocking multi-threaded TCP server
5. A nonblocking TCP client
6. A nonblocking multi-threaded HTTP server based on the buildin http server of libevent
7. A nonblocking HTTP client
8. A nonblocking multi-threaded UDP server
9. Async DNS resolving
10. EventLoop/ThreadPool/Timer

Therefore I pick the following features:

1. Modern functional/bind style callback instead of C-style function pointer.
2. Multi-core friendly and thread-safe
3. A nonblocking multi-threaded TCP server
3. A nonblocking multi-threaded UDP server
4. A nonblocking TCP client
5. A nonblocking UDP client
6. Task system or thread pool
7. .then() continuation
8. timer support, callback on timer expire.
9. Add Co-Routine support

##### Tutotial for programming with submission queue and completion queue:

Please go through the pdf included in the repository if you have not gone through it.
It explains the kernel API and liburing library APIs.

Then you can check this blog: https://cor3ntin.github.io/posts/iouring/#io-uring

Summary: For each non blocking task, programmer needs to do the following things:
1. Prepare an SQE for X purpose and submit it in the Ring. (X could be read, write to socker, file write, timer etc)
2. On completion of the X event, handle the event.
    1. Before handling the event X, make sure to perform one more Prepare SQE + submit in the Ring for X.
       If we want to handle more eventx like X. 
