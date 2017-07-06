Youtube-test
============

This test is a simple C based YouTube test for actively measuring the
quality of YouTube video download in your network. The test was designed
to run on SamKnows probes for large scale active measurements from the
user end. It reports multiple metrics including stall events, start up
delay, throughput etc. and also the media servers that were used for
fetching the audio and video streams. The test does not render or decode
the media, only demuxes it to read timestamps of the frames to maintain
a playout buffer and predict stalls and startup delay. A stall occurs
when either the audio or video buffer is empty. The length of the buffer
can be provided as an argument. 

For the trigger points for different metric measurements and general
flow of the test see:

![](http://i.imgur.com/Gv0Jw3z.png)  

For citation and further details: 

[[1] Ahsan, S., Bajpai, V., Ott, J., & Schönwälder, J. (2015, March).
Measuring YouTube from dual-stacked hosts. In International Conference
on Passive and Active Network Measurement (pp. 249-261). Springer,
Cham.](https://www.google.fi/url?sa=t&rct=j&q=&esrc=s&source=web&cd=1&cad=rja&uact=8&ved=0ahUKEwiL2u32g-DUAhXrHJoKHeqDDqIQFggmMAA&url=https%3A%2F%2Fwww.netlab.tkk.fi%2F~jo%2Fpapers%2F2015-03-PAM-YouTube-Dualstacked.pdf&usg=AFQjCNGsT8Y_zLny22pXLaG5IzXXQFuQ4A)


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


## Releases

There have been these major releases of the test:

| Releases      | Dates         |
| ------------- |:-------------:|
| YOUTUBE.2     | 21/08/2014    |
| YOUTUBE.3     | 10/12/2014    |
| YOUTUBE.4     | 22/01/2015    |
| YOUTUBE.5     | 05/09/2016    |

YOUTUBE.3   

- Introduce chunk downloading with chunk size 1 or 5(max)
- Total download calculated as : (video size + audio size) / (total_test_time),  instead of (video download rate + audio download rate)
- Added first connection time (after audio bitrate) to the report
- Added start up delay (time between beginning of test and first media download + time to buffer first 2 sec of video) to the report
- Intermediate report: 
  - Add test start time
  - Swap position of current time and test start time


YOUTUBE.4  

- Startup delay is updated as the time to fetch the youtube page plus time to buffer first 2 sec of video
- Change download rate in bytes per microsecond to bytes per second. 
- Add error message to the report
- Add max bitrate available (before error code) to the report


YOUTUBE.5  

- Add video framerate, video code, audio codec,  video resolution, video quality, video quality descriptor (SD,HD,FHD or UHD), audio abr, video frame bitrate IQR to the report
- Deprecated the total download rate from report
- Chunk download size determined based on free buffer space instead of fixed 5 sec 
- Reset curl download time and size whenever stall occurs
- Calculate the curl download time from PRETRANSFER instead of STARTTRANSFER
- Fix the bug where the libcurl stats (size and time) for the last chunk were added twice.

## License: 

This code is distributed under GPL v. 3. 
