## streamEye

streamEye is a simple MJPEG streamer for Linux. It acts as an HTTP server and is capable of serving multiple simultaneous clients.

It will feed the JPEGs read at input to all connected clients, in a MJPEG stream. The JPEG frames at input have to be delimited by a given separator.

## Usage

Usage: `<jpeg stream> | streameye [options]`
Available options:
    `-d`  - debug mode, increased log verbosity
    `-h`  - print this help text
    `-l`  - listen only on localhost interface
    `-p port`  - tcp port to listen on (defaults to 8080)
    `-q`  - quiet mode, log only errors
    `-s separator`  - the separator between jpeg frames received at input (mandatory)
    `-t timeout`  - client read timeout, in seconds

## Examples

The following shell script will serve the JPEG files in the current directory, in a loop, with 2 frames per second:

	while true; do
		for file in *.jpg; do
			cat $file
			echo -n "--separator--"
			sleep 0.5
		done
	done | streameye -s "--separator--"


