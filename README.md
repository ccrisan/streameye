## streamEye

*streamEye* is a simple MJPEG streamer for Linux. It acts as an HTTP server and is capable of serving multiple simultaneous clients.

It will feed the JPEGs read at input to all connected clients, in a MJPEG stream. The JPEG frames at input may be delimited by a given separator.
In the absence of a separator, *streamEye* will autodetect all JPEG frames.

## Installation

*streamEye* was tested on various Linux machines, but may work just fine on other platforms.
Assuming your machine has `git`, `gcc` and `make` installed, just type the following commands to compile and install:

    git clone https://github.com/ccrisan/streameye.git
    cd streameye
    make
    sudo make install

## Usage

Usage: `<jpeg stream> | streameye [options]`
Available options:

* `-d` - debug mode, increased log verbosity
* `-h` - print this help text
* `-l` - listen only on localhost interface
* `-p port` - tcp port to listen on (defaults to 8080)
* `-q` - quiet mode, log only errors
* `-s separator` - a separator between jpeg frames received at input (will autodetect jpeg frame starts by default)
* `-t timeout` - client read timeout, in seconds (defaults to 10)

## Examples

The following shell script will serve the JPEG files in the current directory, in a loop, with 2 frames per second:

    while true; do
        for file in *.jpg; do
            cat $file
            echo -n "--separator--"
            sleep 0.5
        done
    done | streameye -s "--separator--"


Most (if not all) usb webcams can output mjpeg natively without having to re-encode, so using ffmpeg try:


    ffmpeg -input_format mjpeg -video_size 640x480 -i /dev/video0 -c copy -f mjpeg - | streameye  


Which should stream your camera (assuming it's at `/dev/video0`), at 640x480 with the cameras default framerate.


if you would like ffmpeg to be quiet try:


    ffmpeg -v quiet -input_format mjpeg -video_size 640x480 -i /dev/video0 -c copy -f mjpeg - | streameye  


you can also try:


    ffmpeg -f v4l2 -list_formats compressed -i /dev/video0


which should return a list of output resolutions your camera supports in the mjpeg format


    [video4linux2,v4l2 @ 0x55c6a47d66c0] Compressed:       mjpeg :         \
    Motion-JPEG : 640x480 160x120 176x144 320x240 432x240 352x288 640x360 800x448 864x480 \
    1024x576 800x600 960x720 1280x720 1600x896 1920x1080 


then you could pick a different resolution, like:


    ffmpeg -v quiet -input_format mjpeg -video_size 1920x1080 -i /dev/video0 -c copy -f mjpeg - | streameye


Which should stream your camera (assuming it's at `/dev/video0`), at Full HD (1920x1080) 


if none of that works, you can have ffmpeg re-encode whatever video format is coming out of the camera into mjpeg,
albeit proboably with higher latency and lower quality and/or framerate.


    ffmpeg -v quiet -i /dev/video0 -r 30 -s 640x480 -f mjpeg -qscale 5 - | streameye


## Extras

### raspimjpeg.py

This script continuously captures JPEGs from a Raspberry PI's CSI camera and writes them to standard output. It works out-of-the-box on Raspbian. The following command will make a simple MJPEG streamer out of your Raspberry PI:

    raspimjpeg.py -w 640 -h 480 -r 15 | streameye

Why not `raspivid` or `raspistill`? Well, at the time of writing `raspivid` doesn't output JPEGs and `raspistill` only works in *stills* mode.

Why Python and not C? Because most of the stuff is done by the GPU, so the insignificant performance gain would not make it worth writing C code. And of course because [picamera](https://picamera.readthedocs.org/) is an amazing library.
