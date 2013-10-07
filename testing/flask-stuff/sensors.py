from app import app
import threading, random, time

class Sensor(threading.Thread):
	class Type:
		DIGITAL = 1
		ANALOGUE = 2
		RANDOM = 3 #lol
		
	def __init__(self, stype, path, calibration = None):
		threading.Thread.__init__(self)
		self.daemon = True
		self.type = stype
		self.path = path
		self.calibration = calibration
		self.lock = threading.Lock()
		self.value = None
		
		if stype == Sensor.Type.RANDOM:
			self.run = self.runRandom
		else:
			raise TypeError("Invalid sensor type")
		
	def runRandom(self):
		while True:
			with self.lock: #not really necessary
				self.value = random.randint(0, 1023)
			time.sleep(0.1)
		

		
