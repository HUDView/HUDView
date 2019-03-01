from picamera import PiCamera
from time import sleep

import argparse
import io
import os

class StreamingObject(object):
  def __init__(self):
    self.frame = None
    self.buffer = io.BytesIO()

  def write(self, buf):
    if buf.startswith(b'\xff\xd8'):
      # New frame recieved
      self.buffer.truncate()
      self.buffer.seek(0)

    return self.buffer.write(buf)


def run(format, fps):
  fifo = '/tmp/hudview_camera_output'

  try:
    os.mkfifo(fifo)
  except OSError as err:
    return err

  with PiCamera(resolution='160x120', framerate=10) as camera:
    with open(fifo, "w") as pipe:
      stream = StreamingObject()
      camera.hflip = True
      camera.start_recording(pipe, format='rgb')


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument('--format', metavar='N', type=int, nargs="+", help="Video output format")
    parser.add_argument('--fps', metavar='N', type=int, nargs="+", help="Video framerate")

    args = parser.parse_args()

    run(args.format, args.fps)


