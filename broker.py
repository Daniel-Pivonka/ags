import paho.mqtt.client as mqttClient
import time
import datetime
import logging
import threading
import thread
import sys
import time
import signal

from flask import Flask, request, json, send_file
from flask_restful import Api, Resource

app = Flask(__name__)
api = Api(app)



ctrl_state = {
        "starttime": "not set",

        "lights": "not set",
        "lights_LU": "not set",

        "sunrise": "07:00",
        "sunset": "21:00",

        "pumpduration": 10,
        "pump_sw": "off",
        "pump_state": "not set",

        "temp": "not set",
        "temp_LU": "not set",

        "humdity": "not set",
        "humdity_LU": "not set",

        "lumens": "not set",
        "lumens_LU": "not set",

        "light_sw": "off",
        "light_sw_LU": "not set",

        "soil1": "not set",
        "soil1_LU": "not set",
        "soil2": "not set",
        "soil2_LU": "not set",
        "soil3": "not set",
        "soil3_LU": "not set",
        "soil4": "not set",
        "soil4_LU": "not set",
        "soil5": "not set",
        "soil5_LU": "not set",
        "soil6": "not set",
        "soil6_LU": "not set",
        "soil7": "not set",
        "soil7_LU": "not set",
        "soil8": "not set",
        "soil8_LU": "not set",
        "soil9": "not set",
        "soil9_LU": "not set",
        "soil10": "not set",
        "soil10_LU": "not set",
        "soil11": "not set",
        "soil11_LU": "not set",
        "soil12": "not set",
        "soil12_LU": "not set",
        }



light_control_path= ""
light = "off"		# desired light state
water = "off"		# desired pump state

## Initialize date and time on AGS100 Initialization.
ctrl_state['starttime'] =  str(datetime.datetime.now())

## create a mapping between moisture sensors and node numbers
## Node number is implied by slot assigned unique moister sensor ID

max_soil_sensors = 12;
soil_node_slot=["","","","","","","","","","","",""]


def is_time_between(sunrise, sunset, check_time=None):
    # If check time is not given, default to current time
    check_time = check_time or datetime.datetime.now().time()
    if sunrise < sunset:
        return check_time >= sunrise and check_time <= sunset
    else: # crosses midnight
        return check_time >= sunrise or check_time <= sunset


def light_control_thread():

  print "##########Light Control Thread Started########"

  ## Run forever unless killed by keyboard interrupt
  while True:
    ## Check to see whether current time is between sunrise and sunset
    ## If it is set desired light state to ON
    ## The main processing loop will turn the light on
    ## Check every 10 seconds
    sunrise= ctrl_state['sunrise']
    sunset= ctrl_state['sunset']

    #convert sunrise to time object
    sunrise = sunrise.split(':')
    try:
        sunrise = datetime.time(int(sunrise[0]), int(sunrise[1]))
    except ValueError as e:
        print e
        exit()

    #convert sunset to time object
    sunset = sunset.split(':')
    try:
        sunset = datetime.time(int(sunset[0]),int(sunset[1]))
    except ValueError as e:
        print e
        exit()


    if is_time_between( sunrise, sunset, None ):
      ## It is daytime
      print "#########Day##########"
      ctrl_state['light_sw'] = "on"
    else:
      ## It is nighttime
      print "########Night#########"
      ctrl_state['light_sw'] = "off"

    time.sleep(10)


def soil_node_assignment( publish_path ):

  loop_count = 0
  while(loop_count < max_soil_sensors ):
    if ((soil_node_slot[loop_count]) == "" or (soil_node_slot[loop_count] == publish_path)):
        soil_node_slot[loop_count] = publish_path
        return(loop_count+1)

    loop_count= loop_count+1

  return(loop_count)

# shows a value of a sensor
class Sensors(Resource):
    def get(self, sensor):

        if sensor == "AGS100":
            return ctrl_state
        else:
            return 'not a valid sensor'



# set value of cotrol state
class Controller(Resource):
    def post(self, controller):

        global light_control_path
        global light
        global client

        value = request.get_data()

        if controller == "light":
            ctrl_state['light_sw'] = value
            print "light is " + light
            client.publish( light_control_path, ctrl_state['light_sw'], 0, False ) # Send on or off
            print "Published " + light + " to " + light_control_path

        elif controller == "pump":
            ctrl_state['pump_sw'] = value
            if ctrl_state['pump_sw'] == 'on':
                client.publish( 'sensors/pump', ctrl_state['pumpduration'], 0, False ) # Send pump duration
            return "pumping for " + str(ctrl_state['pumpduration'])

        elif controller == "pumpduration":
            ctrl_state['pumpduration'] = int(value)
            return "duration is " + str(ctrl_state['pumpduration'])

        elif controller == "sunrise":
            ctrl_state['sunrise'] = value
            return "Sunrise is " + value

        elif controller == "sunset":
            ctrl_state['sunset'] = value
            return "Sunset is " + value

        else:
            return 'not valid controller'


class Image(Resource):
    def get(self):
        filename = '/home/pi/api-server/pizero/Documents/picture.jpg'
        return send_file(filename, mimetype='image/jpg')


##
## Actually setup the Api resource routing here
##
api.add_resource(Sensors, '/sensors/<sensor>')
api.add_resource(Controller, '/controller/<controller>')
api.add_resource(Image, '/image')

## Set up the MQTT broker an call back functions prior to
## falling into the API endless loop.

## MQTT setup and callback code.
def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("Connected to broker")
        client.subscribe("sensors/#")
    else:
        print("Connection failed")


def on_message(client1, userdata, message):

    global light_control_path
    global light
    global client

    print("Message received " + str(datetime.datetime.now()) + ": "  + str(message.payload) + " on " + message.topic)

    message_json = json.loads(message.payload.decode('utf8'))

    if message_json['Node_type'] == "Lgtctl":
        print "XXXXLight Controller StateXXXX"
        ctrl_state['lights'] = message_json['lightstatus']
        ctrl_state['lights_LU'] = str(datetime.datetime.now())
        print message.topic + "/toggle"
        light_control_path = message.topic + "/toggle"
        print "Setting up to send to: " + light_control_path

        if ctrl_state['lights'] != ctrl_state['light_sw']:
            light_control_path= message.topic + "/toggle"
            print "Setting up to send to: " + light_control_path
            print "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"
            try:
                client.publish( light_control_path, ctrl_state['light_sw'], 0, False ) # Send on or off
            except Exception as e:
                print e

    if message_json['Node_type'] == "Moisture":
        nodenum = str(soil_node_assignment(message.topic))
        print "moisture node " + nodenum
        if nodenum == "1":
              ctrl_state['soil1'] = message_json['Moisture_Level']
              ctrl_state['soil1_LU'] = str(datetime.datetime.now())
        elif nodenum == "2":
              ctrl_state['soil2'] = message_json['Moisture_Level']
              ctrl_state['soil2_LU'] = str(datetime.datetime.now())
        elif nodenum == "3":
              ctrl_state['soil3'] = message_json['Moisture_Level']
              ctrl_state['soil3_LU'] = str(datetime.datetime.now())
        elif nodenum == "4":
              ctrl_state['soil4'] = message_json['Moisture_Level']
              ctrl_state['soil4_LU'] = str(datetime.datetime.now())
        elif nodenum == "5":
              ctrl_state['soil5'] = message_json['Moisture_Level']
              ctrl_state['soil5_LU'] = str(datetime.datetime.now())
        elif nodenum == "6":
              ctrl_state['soil6'] = message_json['Moisture_Level']
              ctrl_state['soil6_LU'] = str(datetime.datetime.now())
        elif nodenum == "7":
              ctrl_state['soil7'] = message_json['Moisture_Level']
              ctrl_state['soil7_LU'] = str(datetime.datetime.now())
        elif nodenum == "8":
              ctrl_state['soil8'] = message_json['Moisture_Level']
              ctrl_state['soil8_LU'] = str(datetime.datetime.now())
        elif nodenum == "9":
              ctrl_state['soil9'] = message_json['Moisture_Level']
              ctrl_state['soil9_LU'] = str(datetime.datetime.now())
        elif nodenum == "10":
              ctrl_state['soil10'] = message_json['Moisture_Level']
              ctrl_state['soil10_LU'] = str(datetime.datetime.now())
        elif nodenum == "11":
              ctrl_state['soil11'] = message_json['Moisture_Level']
              ctrl_state['soil11_LU'] = str(datetime.datetime.now())
        elif nodenum == "12":
              ctrl_state['soil12'] = message_json['Moisture_Level']
              ctrl_state['soil12_LU'] = str(datetime.datetime.now())

    if message_json['Node_type'] == "THLc1":
        if str(message_json['temp']) != "nan":
            ctrl_state['temp'] = str(message_json['temp'])
            ctrl_state['temp_LU'] = str(datetime.datetime.now())
        if str(message_json['humidity']) != "nan":
                ctrl_state['humidity'] = str(message_json['humidity'])
                ctrl_state['humidity_LU'] = str(datetime.datetime.now())
        ctrl_state['lumens_LU'] = str(datetime.datetime.now())
        ctrl_state['lumens'] = str(message_json['lumens'])

    if message_json['Node_type'] == "Pump":
        ctrl_state['pump_state'] = str(message_json['pumpstatus'])
        if ctrl_state['pump_state'] == 'on':
            ctrl_state['pump_sw'] = 'off'

## end of New message callback




## start of function for broker thread
broker_address= "192.168.1.2"
port = 1883

client = mqttClient.Client("Python")
client.on_connect= on_connect
client.on_message= on_message
client.connect_async(broker_address, port=port)
client.loop_start()

## Start the light checking thread
threads= []
t = threading.Thread(target=light_control_thread)
threads.append(t)
t.start()

## Fall into the API code execution
if __name__ == '__main__':
    app.run("192.168.1.2","5000")
