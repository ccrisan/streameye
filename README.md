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

The following command will stream your camera (assuming it's at `/dev/video0`), with 30 frames per second at 640x480:

    ffmpeg -f video4linux2 -i /dev/video0 -r 30 -s 640x480 -f mjpeg -qscale 5 - 2>/dev/null | streameye

## Extras

### raspimjpeg.py

This script continuously captures JPEGs from a Raspberry PI's CSI camera and writes them to standard output. It works out-of-the-box on Raspbian. The following command will make a simple MJPEG streamer out of your Raspberry PI:

    raspimjpeg.py -w 640 -h 480 -r 15 | streameye

Why not `raspivid` or `raspistill`? Well, at the time of writing `raspivid` doesn't output JPEGs and `raspistill` only works in *stills* mode.

Why Python and not C? Because most of the stuff is done by the GPU, so the insignificant performance gain would not make it worth writing C code. And of course because [picamera](https://picamera.readthedocs.org/) is an amazing library.
