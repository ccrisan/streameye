## streamEye

streamEye is a simple MJPEG streamer for Linux. It acts as an HTTP server and is capable of serving multiple simultaneous clients.

It will feed the JPEGs read at input to all connected clients, in a MJPEG stream. The JPEG frames at input may be delimited by a given separator.

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

The following command will stream your camera (assuming it's at `/dev/video0`), with 5 frames per second at 640x480:

    ffmpeg -f video4linux2 -i /dev/video0 -r 5 -s 640x480 -f mjpeg -qscale 5 - 2>/dev/null | ./streameye -d
