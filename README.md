Youtube-test
============
This test is a simple C based YouTube test for actively measuring the quality of YouTube video download in your network. The test was designed to run on SamKnows probes for large scale active measurements from the user end. It reports multiple metrics including stall events, start up delay, throughput etc. and also the media servers that were used for fetching the video. The test does not render or decode the video, only demuxes it to read timestamps of the frames to maintain a playout buffer and predict stalls and startup delay.

Refer to [test sequence diagram](./sequence.pdf) for the trigger points for different metric measurements. 

## Compiling

To download and compile, run the following commands from a linux shell (the code works on Mac as well)

```shell
$ git clone https://github.com/sabyahsan/Youtube-test.git
$ cd Youtube-test
$ make
```




The Makefile downloads and compiles the required FFMPEG and libcurl libraries automatically. However, you may still need to install these additional libraries: yasm, libssl-dev, zlib1g-dev. 

## Running the Test 

Before running the test, set the LD_LIBRARY_PATH. From inside the Youtube-test folder, run the following commands: 

```shell
$ LD_LIBRARY_PATH=$PWD/build/ffmpeg_install/lib/
$ export LD_LIBRARY_PATH
```

The test runs with default paramaters in the absence of any arguments so to get started and make sure everything is working, you can run it with a YouTube video of your choice as shown below. Note that the download is throttled, so the test would run for almost the same duration as the duration of the video, in this case about 2 and a half minutes. 

```shell
$ ./vdo_client https://www.youtube.com/watch?v=j8cKdDkkIYY
YOUTUBE.3;1498562299;OK;j8cKdDkkIYY;mp4dash;144507334;0;0;0;149983000;101025;0;VIDEO;136;49792158;25261454;507338;r4---sn-ovgq0oxu-5goe.googlevideo.com;2001:948:7:1::f;7205;276343;AUDIO;140;83050717;2385881;28728;r4---sn-ovgq0oxu-5goe.googlevideo.com;2001:948:7:1::f;7347;16263;8339;684731;5;600;
```


### Arguments

A list of arguments that can be provided to the test in addition to the URL are given below. 

-	**[\<url\>]** => URL of the YouTube video
-	**--verbose** => Print instantaneous metric values when downloading video
-	**--range \<s\>** => The length of the playout buffer in seconds. Default value is 5, value of x must be greater than 5 
-	**--onebitrate** => when used the client does not switch to lower bit rate when a stall occurs. Disabled by default.
-	**--maxbitrate \<bps\>** => if specified, the client will not attempt to download qualities with a higher bit rate, default MAXINT
-	**--mintime \<s\>** => if a test finishes before this time, it is considered a failure, default value is 0
-	**--maxtime \<s\>** => maximum time for which the test should run, default value is MAXINT
- **-4** => IPv4 only
- **-6** => IPv6 only 

### Output

When the test finishes, it prints a ; separated list of metrics. The values in order are listed below. Note the values marked fixed will always appear as is. All times are in microseconds.

- YOUTUBE.3 => Fixed 
- Test identifier => Based on the time that the test ran
- OK|FAIL => Status of the test
- Video ID => YouTube video ID, extracted from the provided URL 
- mp4|web|mp4dash|flv|3gpp|webmdash|unknown => format of the video 
- download time => Time taken to download the video+audio content 
- Number of stall events
- Average duration of a stall event
- Total stall time 
- Duration of the downloaded content => will be same as video length only if the whole video is downloaded during the test
- Prebuffering time => Time taken to prebuffer 2 seconds of video
- Download rate 
- VIDEO => Fixed
- Video itag => Refer to our paper for an explanation of itag
- Download rate of video
- Size of the Video in bytes
- Time taken to download video
- Video content hostname (CDN)
- IP of the video host (CDN)
- TCP Connect time to the Video CDN 
- Advertised bitrate of the downloaded video
- AUDIO => Fixed
- Audio itag => Refer to our paper for an explanation of itag
- Download rate of audio
- Size of the audio in bytes
- Time taken to download audio
- Audio content hostname (CDN)
- IP of the audio host (CDN)
- TCP Connect time to the audio CDN 
- Advertised bitrate of the downloaded audio
- TCP Connect time to the YouTube server (primary URL)
- Startup delay => Prebuffering time + Fetching of primary URL and parsing for video+audio URLs 
- Buffer Length (in seconds) => As specified by the argument --range, just used for reporting
- Error code => Internal error code in case of failure. 



## License: 

This code is distributed under GPL v. 3. 
