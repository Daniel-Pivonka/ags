# importing library
import requests
import datetime

# api-endpoint
URL = "http://216.49.153.111:5000/sensors/AGS100"

# sending get request and saving the response as response object
r = requests.get(url = URL)

# extracting data in json format
data = r.json()


print '\n'
# parse data
for x in range(1,12):
    # print 'soil'+str(x)
    # print data['soil'+str(x)]
    # print data['soil'+str(x)+'_LU']


    fifthteen_ago = datetime.datetime.now() - datetime.timedelta(minutes=15)
    soil_lu = datetime.datetime.strptime(data['soil'+str(x)+'_LU'], "%Y-%m-%d %H:%M:%S.%f")

    if soil_lu < fifthteen_ago:
        print 'sensor ' + str(x) + ' has not updated in the last 15 mis'
        print 'last updated at ' + data['soil'+str(x)+'_LU']

    else:
        print 'sensor ' + str(x) + ' has updated in the last 15, the value is ' + str(data['soil'+str(x)])

    print '\n'



print '\n'
