#!/usr/bin/python

# Copyright (c) Calin Crisan
# This file is part of streamEye.
#
# streamEye is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.


import argparse
import io
import logging
import picamera
import signal
import sys
import time


VERSION = '0.5'

last_time = 0
options = None
camera = None
running = True


def configure_signals():
    def bye_handler(signal, frame):
        global running

        if running:
            logging.info('interrupt signal received, shutting down')
            running = False

    signal.signal(signal.SIGINT, bye_handler)
    signal.signal(signal.SIGTERM, bye_handler)


def parse_options():
    global options
    
    def colfx_arg(string):
        try:
            parts = string.split(':')
            u, v = int(parts[0]), int(parts[1])
            if not (0 <= u <= 255) or not (0 <= v <= 255):
                raise Exception()

            return u, v

        except:
            raise argparse.ArgumentTypeError('invalid value')
    
    parser = argparse.ArgumentParser(add_help=False,
            usage='%(prog)s -w WIDTH -h HEIGHT -r FRAMERATE [options]',
            description='This program continuously captures JPEGs from the CSI camera and writes them to standard output.')
    
    parser.add_argument('-w', '--width', help='capture width, in pixels (64 to 1920, required)',
            type=int, dest='width', required=False)
    parser.add_argument('-h', '--height', help='capture height, in pixels (64 to 1080, required)',
            type=int, dest='height', required=False)
    parser.add_argument('-r', '--framerate', help='number of frames per second (1 to 30, required)',
            type=int, dest='framerate', required=False)
    parser.add_argument('-q', '--quality', help='jpeg quality factor (1 to 100, defaults to 50)',
            type=int, dest='quality', default=50)

    parser.add_argument('--vflip', help='flip image vertically',
            action='store_true', dest='vflip', default=False)
    parser.add_argument('--hflip', help='flip image horizontally',
            action='store_true', dest='hflip', default=False)
    parser.add_argument('--rotation', help='rotate image',
            type=int, dest='rotation', default=0, choices=(0, 90, 180, 270))

    parser.add_argument('--brightness', help='image brightness (0 to 100, defaults to 50)',
            type=int, dest='brightness', default=50)
    parser.add_argument('--contrast', help='image contrast (-100 to 100, defaults to 0)',
            type=int, default=0)
    parser.add_argument('--saturation', help='image saturation (-100 to 100, defaults to 0)',
            type=int, dest='saturation', default=0)
    parser.add_argument('--sharpness', help='image sharpness (-100 to 100, defaults to 0)',
            type=int, dest='sharpness', default=0)

    parser.add_argument('--iso', help='capture ISO (100 to 800)',
            type=int, dest='iso', default=None)
    parser.add_argument('--ev', help='EV compensation (-25 to 25)',
            type=int, dest='ev', default=None)
    parser.add_argument('--shutter', help='shutter speed, in microseconds (0 to 6000000)',
            type=int, dest='shutter', default=None)

    parser.add_argument('--exposure', help='exposure mode',
            type=str, dest='exposure', default=None,
            choices=('off', 'auto', 'night', 'nightpreview', 'backlight', 'spotlight', 'sports', 'snow',
                    'beach', 'verylong', 'fixedfps', 'antishake', 'fireworks'))
    parser.add_argument('--awb', help='set automatic white balance',
            type=str, dest='awb', default=None,
            choices=('off', 'auto', 'sunlight', 'cloudy', 'shade', 'tungsten', 'fluorescent', 'incandescent', 'flash', 'horizon'))
    parser.add_argument('--metering', help='metering mode',
            type=str, dest='metering', default=None,
            choices=('average', 'spot', 'backlit', 'matrix'))
    parser.add_argument('--drc', help='dynamic range compression',
            type=str, dest='drc', default=None,
            choices=('off', 'low', 'medium', 'high'))
    parser.add_argument('--vstab', help='turn on video stabilization',
            action='store_true', dest='vstab', default=False)

    parser.add_argument('--imxfx', help='image effect',
            type=str, dest='imxfx', default='none',
            choices=('none', 'negative', 'solarize', 'sketch', 'denoise', 'emboss', 'oilpaint', 'hatch', 'gpen', 'pastel',
                    'watercolor', 'film', 'blur', 'saturation', 'colorswap', 'washedout', 'posterise', 'colorpoint',
                    'colorbalance', 'cartoon', 'deinterlace1', 'deinterlace2'))
    parser.add_argument('--colfx', help='color effect (U:V format, 0 to 255, e.g. 128:128)',
            type=colfx_arg, dest='colfx', default=None)

    parser.add_argument('-s', '--stills', help='use stills mode instead of video mode (considerably slower)',
            action='store_true', dest='stills', default=False)
    parser.add_argument('-d', '--debug', help='debug mode, increase verbosity',
            action='store_true', dest='debug', default=False)
    parser.add_argument('--help', help='show this help message and exit',
            action='help', default=argparse.SUPPRESS)
    parser.add_argument('-v', '--version',
            action='version', default=argparse.SUPPRESS)
    
    parser._optionals.title = 'Available options'
    parser.version = '%(prog)s ' + VERSION

    options = parser.parse_args()
    
    def validate_option(name, min=None, max=None, required=False):
        value = getattr(options, name, None)

        if value is None:
            if required:
                return 'a value is required'
            
            else:
                return
            
        if (None not in [min, max]) and not (min <= value <= max):
            return 'must be between %(min)s and %(max)s' % {'min': min, 'max': max}

    def validate_or_exit(name, **kwargs):
        msg = validate_option(name, **kwargs)
        if msg:
            parser.print_usage(sys.stderr)
            sys.stderr.write('%(prog)s: error: option %(name)s: %(msg)s\n' % {
                    'prog': parser.prog,
                    'name': name,
                    'msg': msg})

            sys.exit(-1)

    validate_or_exit('width', min=64, max=1920, required=True)
    validate_or_exit('height', min=64, max=1080, required=True)
    validate_or_exit('framerate', min=1, max=30, required=True)
    validate_or_exit('quality', min=1, max=100)
    validate_or_exit('brightness', min=0, max=100)
    validate_or_exit('contrast', min=-100, max=100)
    validate_or_exit('saturation', min=-100, max=100)
    validate_or_exit('sharpness', min=-100, max=100)
    validate_or_exit('iso', min=100, max=800)
    validate_or_exit('ev', min=-25, max=25)
    validate_or_exit('shutter', min=0, max=6000000)


def streams_iter():
    global last_time

    frame_interval = 1.0 / options.framerate

    while running:
        now = time.time()
        if now - last_time >= frame_interval:
            last_time = now
            logging.debug('jpeg frame ready')
            yield sys.stdout

        else:
            sys.stdout.flush()
            time.sleep(frame_interval * 0.01)


def init_camera():
    global camera

    logging.debug('initializing camera')

    camera = picamera.PiCamera(resolution=(options.width, options.height))
    
    camera.framerate = options.framerate
    camera.vflip = options.vflip
    camera.hflip = options.hflip
    camera.rotation = options.rotation

    camera.brightness = options.brightness
    camera.contrast = options.contrast
    camera.saturation = options.saturation
    camera.sharpness = options.sharpness
    
    if options.iso is not None:
        camera.iso = options.iso
    if options.ev is not None:
        camera.exposure_compensation = options.ev
    if options.shutter is not None:
        camera.shutter = options.shutter

    if options.exposure is not None:
        camera.exposure_mode = options.exposure
    if options.awb is not None:
        camera.awb_mode = options.awb
    if options.metering is not None:
        camera.meter_mode = options.metering
    if options.drc is not None:
        camera.drc_strength = options.drc
    camera.video_stabilization = options.vstab
    
    if options.imxfx is not None:
        camera.image_effect = options.imxfx
    if options.colfx is not None:
        camera.color_effects = options.colfx
    
    logging.debug('camera initialized')


def run():
    logging.debug('starting capture')
    camera.capture_sequence(streams_iter(), format='jpeg',
            use_video_port=not options.stills, thumbnail=None, quality=options.quality)


if __name__ == '__main__':
    #configure_signals() this doesn't seem to work and I don't really know why
    parse_options()

    logging.basicConfig(filename=None, level=logging.DEBUG if options.debug else logging.INFO,
            format='%(asctime)s: %(levelname)5s: %(message)s', datefmt='%Y-%m-%d %H:%M:%S')

    logging.info('raspimjpeg.py %s' % VERSION)
    logging.info('hello!')
    init_camera()
    run()
    logging.info('bye!')

