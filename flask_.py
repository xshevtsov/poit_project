from flask import Flask, request,jsonify,render_template
from flask_socketio import SocketIO,emit
from flask_cors import CORS
import mysql.connector
from mysql.connector import Error
import json


app = Flask(__name__,template_folder='templates')
app.config['SECRET_KEY'] = 'secret!'
CORS(app,resources={r"/*":{"origins":"*"}})
socketio = SocketIO(app,cors_allowed_origins="*")

# id = 0
# temp, pressure = [], []



def insert_sensor_reading(temperature, pressure):
    try:
        # Establish a connection to the MySQL database
        connection = mysql.connector.connect(
            host='localhost',
            database='poit_project',
            user='poit_user',
            password='pass'
        )

        if connection.is_connected():
            print("MySQL connection is open")   
            cursor = connection.cursor()


            # SQL query to count the number of rows in the table
            count_query = "SELECT COUNT(*) FROM SensorReadings"
            cursor.execute(count_query)
            row_count = cursor.fetchone()[0]

            # If row count exceeds 1800, delete the oldest record
            if row_count >= 1800:
                delete_query = "DELETE FROM SensorReadings ORDER BY timestamp ASC LIMIT 1"
                cursor.execute(delete_query)
                connection.commit()
                print("Oldest record deleted to maintain limit of 100 rows")

            # SQL query to insert a new record
            sql_insert_query = """INSERT INTO SensorReadings (temperature, pressure)
                                  VALUES (%s, %s)"""
            # Tuple of values to be inserted
            record_tuple = (temperature, pressure)
            cursor.execute(sql_insert_query, record_tuple)
            # Commit the transaction
            connection.commit()
            print("Record inserted successfully into SensorReadings table")

    except Error as e:
        print(f"Error while connecting to MySQL: {e}")

    finally:
        if connection.is_connected():
            cursor.close()
            connection.close()
            print("MySQL connection is closed")

def get_sensor_readings():
    try:
        # Establish a connection to the MySQL database
        connection = mysql.connector.connect(
            host='localhost',
            database='poit_project',
            user='poit_user',
            password='pass'
        )

        if connection.is_connected():
            print("MySQL connection is open")   
            cursor = connection.cursor()
            # SQL query to retrieve data
            select_query = "SELECT * FROM SensorReadings"
            cursor.execute(select_query)
            # Fetch all rows from the executed query
            records = cursor.fetchall()
            
            # print("Total number of rows in SensorReadings is: ", cursor.rowcount)
            # print("\nPrinting each sensor reading:\n")
            # for row in records:
            #     print("Id:", row[0])
            #     print("Pressure:", row[3])
            #     print("Temperature:", row[2])
            #     print("Reading Time:", row[1], "\n")

             # Get column names from the cursor
            column_names = [i[0] for i in cursor.description]
            
            # Convert fetched data to a list of dictionaries
            readings = []
            for row in records:
                reading = dict(zip(column_names, row))
                readings.append(reading)
            
            # Convert the list of dictionaries to a JSON string
            readings_json = json.dumps(readings, default=str)  # default=str to handle datetime serialization
            return readings_json
    except Error as e:
        print(f"Error while connecting to MySQL: {e}")
        return None

    finally:
        if connection.is_connected():
            cursor.close()
            connection.close()
            print("MySQL connection is closed")       




# Example usage
# insert_sensor_reading(25.2, 1010.50)
# insert_sensor_reading(26.5, 1009.75)




# print(get_sensor_readings())


@app.route('/')
def index():
    return render_template('index.html')

@app.route('/GetGraphData', methods = ['GET'])
def getGraphData():
    return get_sensor_readings()

# @app.route('/style.css')
# def redirect_to_style():
#     with open('templates/style.css', 'r', encoding='utf-8') as f:
#         return f.read()

@app.route('/SendDisplayData',methods = ['POST'])
def sendDisplayData():
    data = request.get_json() # a multidict containing POST data
    print(data)
    socketio.emit("SendDisplayData",f"{data}")
    # emit({'displayData': data})
    return data
    # average_time = request.form.get('x')
    # print(average_time)
    # choices = request.form.get('y')
    # emit({'data': data})
@app.route('/clearDisplay', methods = ['POST'])
def sendClearDisplay():
    socketio.emit("clearDisplay")
    return "200"

@app.route("/http-call")
def http_call():
    """return JSON with string data as the value"""
    data = {'data':'This text was fetched using an HTTP call to server on render'}
    return jsonify(data)



@socketio.on("connect")
def connected():
    """event listener when client connects to the server"""
    print(request.sid)
    print("client has connected")
    emit("connect",{"data":f"id: {request.sid} is connected"})

@socketio.on('data')
def handle_message(data):
    """event listener when client types a message"""
    print("data from the front end: ",str(data))
    # emit("data",{'data':data,'id':request.sid},broadcast=True)

@socketio.on("disconnect")
def disconnected():
    """event listener when client disconnects to the server"""
    print("user disconnected")
    emit("disconnect",f"user {request.sid} disconnected",broadcast=True)

# @socketio.on("SendDisplayData")
# def handle_message(data):
#     data = request.get_json() # a multidict containing POST data
#     print(data)
#     # socketio.emit("SendDisplayData",f"{data}")


@socketio.on("graph_data")
def handle_message(data):
    print("data from the front end: ",str(data['temp']))
    insert_sensor_reading(data['temp'], data['pressure'])
    # global temp, pressure
    # temp.append(data['temp'])
    # pressure.append(data['pressure'])

    # data_to_file = [{"temp": t, "pressure": p} for t, p in zip(temp, pressure)]

    # print(json.dumps(data_to_file))
    # # global id 

    # # data['file_id'] = id 
    # # id = id+1
    # y = json.dumps(data)
    # print(y)

    with open('data.json', 'a+', encoding='utf-8') as f:
        f.write(json.dumps(data, ensure_ascii=False, indent=4)+',')


    
    # emit("data",{'data':data,'id':request.sid},broadcast=True)


if __name__ == '__main__':
    socketio.run(app, debug=True,port=5000,host='0.0.0.0')