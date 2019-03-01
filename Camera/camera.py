from picamera import PiCamera
from time import sleep

import io

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


with PiCamera(resolution='960x540', framerate=15) as camera: 
  stream = StreamingObject()
  camera.start_recording("video.h264", format='h264')
  sleep(5)
  camera.stop_recording()

