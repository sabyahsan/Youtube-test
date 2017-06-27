Youtube-test
============
This test is a simple C based YouTube test for actively measuring the quality of YouTube video download in your network. The test was designed to run on SamKnows probes for large scale active measurements from the user end. The test reports multiple metrics including stall events, start up delay, throughput etc. and also the media servers that were used for fetching the video. 

## Compiling

The Makefile downloads and compiles the required FFMPEG and libcurl libraries automatically. However, you may still need to install these additional libraries: yasm, libssl-dev, zlib1g-dev


## Running the Test 

The test runs with default paramaters in the absence of any arguments, so just to get started, run the vdo_client, and it should look something like this. Note that it takes a few minutes to complete. 


```shell
$ ./vdo_client 
YOUTUBE.3;1498562299;OK;j8cKdDkkIYY;mp4dash;144507334;0;0;0;149983000;101025;0;VIDEO;136;49792158;25261454;507338;r4---sn-ovgq0oxu-5goe.googlevideo.com;2001:948:7:1::f;7205;276343;AUDIO;140;83050717;2385881;28728;r4---sn-ovgq0oxu-5goe.googlevideo.com;2001:948:7:1::f;7347;16263;8339;684731;5;600;
```



## License: 

This code is distributed under GPL v. 3. 
