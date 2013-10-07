'''
python3, python3-dev, python3-pip
pip install flask
python3 main.py
'''
from flask import jsonify, abort
from app import app
import random
from sensors import Sensor

sensor1 = Sensor(Sensor.Type.RANDOM, "ha")

@app.route("/")
def hello():
    return "MCTX3420 API in Python!"

@app.route("/sensors/<int:sensor_id>")
def get_sensor(sensor_id):
	if sensor_id == 0:
		with sensor1.lock:
			return jsonify({'sensor_id' : 'winning', 'value' : sensor1.value})
	abort(404)

@app.route("/actuators/<int:actuator_id>/<value>")
def set_actuator(actuator_id, value):
	return jsonify({'actuator_id' : actuator_id, 'value' : value})

@app.route("/start")
def start_sensors():
	sensor1.start()
	return "start"

if __name__ == "__main__":
	app.run(debug=True)
