from picamera import PiCamera
from time import sleep

import argparse
import io
import os
import signal

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


def run(self, format, fps):
  fifo = '/tmp/hudview_camera_output'
  self.close = False;

  def close_camera(sig, frame):
    self.close = True

  signal.signal(signal.SIGINT, close_camera)
  signal.signal(signal.SIGTERM, close_camera)

  #try:
  #  os.mkfifo(fifo)
  #except OSError as err:
  #  return err

  with PiCamera(resolution='160x120', framerate=10) as camera:
    with open(fifo, "w") as file:
      stream = StreamingObject()
      camera.hflip = True
      camera.start_recording(file, format='rgb')
      if self.close:
        camera.close()
        return



if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument('--format', metavar='N', type=int, nargs="+", help="Video output format")
    parser.add_argument('--fps', metavar='N', type=int, nargs="+", help="Video framerate")

    args = parser.parse_args()

    run(args.format, args.fps)


