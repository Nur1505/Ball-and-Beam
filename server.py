from flask import Flask, request, jsonify
import matplotlib.pyplot as plt
import threading
import sys
import time

app = Flask(__name__)

position_data = []
time_data = []

# Global flag to control data reception
listening = True

@app.route('/updatePosition', methods=['POST'])
def update_position():
    global position_data, time_data, listening

    if not listening:
        return jsonify({"message": "Server is paused. Please resume to accept new data."}), 403

    # Get JSON data from the POST request
    data = request.get_json()
    if 'positions' in data and 'times' in data:
        position_data.extend(data['positions'])
        time_data.extend(data['times'])
        plot_data()
        # Respond with a success message
        return jsonify({"message": "Data received successfully"}), 200
    else:
        return jsonify({"message": "Invalid data format"}), 400


def plot_data():
    global position_data, time_data

    if len(position_data) == 0 or len(time_data) == 0:
        return jsonify({"message": "No data to plot"}), 400

    # Plot data
    plt.figure()
    plt.plot(time_data, position_data, label='Pozicija (cm)')
    plt.xlabel('Vrijeme (ms)')
    plt.ylabel('Pozicija (cm)')
    plt.title('Vremenski odziv pozicije loptice')
    plt.grid(True)
    plt.legend()
    plt.show()

    return jsonify({"message": "Plot generated"}), 200

def pause_and_resume_server():
    global listening

    # Wait for user input to pause
    input("Press Enter to pause the server and generate the plot...")
    listening = False
    print("Server paused. Generating plot...")
    
    # Generate the plot
    with app.test_request_context('/plotData', method='GET'):
        app.dispatch_request()

    # Wait for user input to resume
    input("Press Enter to resume the server...")
    listening = True
    print("Server resumed.")

if __name__ == '__main__':
    # Run the pause_and_resume_server in a separate thread
    threading.Thread(target=pause_and_resume_server, daemon=True).start()
    app.run(host='0.0.0.0', port=5000)
